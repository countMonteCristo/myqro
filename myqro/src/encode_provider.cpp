#include "encode_provider.hpp"

#include <algorithm>
#include <format>

#include "logger.hpp"
#include "utils.hpp"


// =============================================================================

namespace myqro
{

// =============================================================================

// TODO: implement mixed encoding strategy (split string into chunks and encode them separately)
Context EncodeProvider::Encode(const std::string& data, CorrectionLevel cl) const
{
    if (!IsDataSupported(data))
        throw Error(std::format("Unsupported data for {}: {}", GetProviderName(), data));

    Context context(data, cl);
    ConvertInput(data, context);
    PrepareServiceFields(context);
    AddTailZeros(context);
    AddRequiredVersionTailBytes(context);
    PrepareBlocks(context);
    PrepareOutput(context);

    return context;
}

void EncodeProvider::PrepareServiceFields(Context& context) const
{
    auto [version, max_data_size] = EstimateVersion(context.stream, context.cl);
    LogDebug("Estimated version={} max_data_size={}", version, max_data_size);

    size_t max_version = (version <= 9) ? 9 : (version <= 26) ? 26 : MAX_VERSION;
    EncodingType encoding = GetEncodingType();

    context.data_size_field_width = DataSizeFieldWidth.at(encoding).at(max_version);

    if (context.stream.Size() + context.data_size_field_width + context.encoding_field_width > max_data_size)
    {
        version += 1;
        if (version > MAX_VERSION)
            throw Error(std::format("Data stream ({}) with service fields ({}, {}) "
                                    "is too big for correction level {}",
                                    context.stream.Size(), context.encoding_field_width,
                                    context.data_size_field_width,
                                    CorrectionLevelToString(context.cl)));
        max_data_size = VersionCorrectionMaxDataSize.at(context.cl).at(version - 1);
    }

    DataStream result;
    result.AppendBits(static_cast<uint8_t>(encoding), context.encoding_field_width);
    result.AppendBits(context.input_data_size, context.data_size_field_width);
    result << context.stream;

    context.version = version;
    context.stream = result;
    context.max_data_size = max_data_size;
}

void EncodeProvider::AddTailZeros(Context& context) const
{
    size_t rem = context.stream.Size() % BITS_PER_BYTE;
    if (rem > 0)
    {
        LogDebug("Add {} tailing zero bits", BITS_PER_BYTE - rem);
        context.stream.SetBitSize(context.stream.Size() + BITS_PER_BYTE - rem);
    }
}

void EncodeProvider::AddRequiredVersionTailBytes(Context& context) const
{
    uint8_t required_tail_bytes[] = {0b11101100, 0b00010001};
    size_t required_bytes_count = context.max_data_size / BITS_PER_BYTE;
    uint8_t idx = 0;
    while (context.stream.ByteSize() < required_bytes_count)
    {
        context.stream.AppendBits(required_tail_bytes[idx], BITS_PER_BYTE);
        idx = (idx + 1) % std::size(required_tail_bytes);
    }
}

void EncodeProvider::PrepareBlocks(Context& context) const
{
    size_t blocks_count = context.GetBlocksCount();
    size_t n_correction_bytes = context.GetCorrectionBytesCount();

    LogDebug("# of blocks: {}", blocks_count);
    LogDebug("# of corr bytes: {}", n_correction_bytes);

    context.data_blocks = context.stream.GenerateBlocks(blocks_count);
    context.correction_blocks.reserve(blocks_count);
    for (const Block& block: context.data_blocks)
        context.correction_blocks.push_back(GenerateCorrectionBlock(block, n_correction_bytes));
}

void EncodeProvider::PrepareOutput(Context& context) const
{
    context.output.reserve(context.stream.Size() + context.correction_blocks.size() * context.GetBlocksCount());
    size_t n_bytes_per_block = context.GetBytesPerBlock();

    for (size_t byte_idx = 0; byte_idx < n_bytes_per_block + 1; byte_idx++)
    {
        for (const auto& block: context.data_blocks)
        {
            if (byte_idx >= block.Size()) continue;
            context.output.push_back(*(block.begin + byte_idx));
        }
    }

    size_t n_correction_bytes = context.GetCorrectionBytesCount();
    for (size_t byte_idx = 0; byte_idx < n_correction_bytes; byte_idx++)
    {
        for (const auto& block: context.correction_blocks)
        {
            context.output.push_back(block.at(byte_idx));
        }
    }
}

std::pair<size_t, size_t> EncodeProvider::EstimateVersion(const DataStream& stream, CorrectionLevel cl)
{
    const auto& sizes = VersionCorrectionMaxDataSize.at(cl);
    size_t version = 1;
    for (; version <= MAX_VERSION; version++)
    {
        if (sizes[version - 1] > stream.Size())
            return {version, sizes[version - 1]};
    }
    throw Error(std::format("No versions available for correction level {} and data bit size {}",
                            CorrectionLevelToString(cl) ,stream.Size()));
};

// =============================================================================

const std::unordered_map<char, uint8_t> AlphaNumericEncodeProvider::chars_ = {
    {'0', 0},  {'1', 1},  {'2', 2},  {'3', 3},  {'4', 4},  {'5', 5},  {'6', 6},  {'7', 7},  {'8', 8},  {'9', 9},
    {'A', 10}, {'B', 11}, {'C', 12}, {'D', 13}, {'E', 14}, {'F', 15}, {'G', 16}, {'H', 17}, {'I', 18}, {'J', 19},
    {'K', 20}, {'L', 21}, {'M', 22}, {'N', 23}, {'O', 24}, {'P', 25}, {'Q', 26}, {'R', 27}, {'S', 28}, {'T', 29},
    {'U', 30}, {'V', 31}, {'W', 32}, {'X', 33}, {'Y', 34}, {'Z', 35}, {' ', 36}, {'$', 37}, {'%', 38}, {'*', 39},
    {'+', 40}, {'-', 41}, {'.', 42}, {'/', 43}, {':', 44}
};

// =============================================================================

bool AlphaNumericEncodeProvider::IsDataSupported(const std::string& data) const
{
    return std::all_of(data.begin(), data.end(), [](char c){ return chars_.count(c) > 0;});
}

// =============================================================================

void AlphaNumericEncodeProvider::ConvertInput(const std::string& data, Context& context) const
{
    for (size_t i = 0; i < data.size(); i += 2)
    {
        size_t tail = data.size() - i;
        uint8_t mask_size = 0;
        uint16_t result = 0;

        if (tail == 1)
        {
            result = chars_.at(data[i]);
            mask_size = 6;
        }
        else
        {
            size_t code1 = chars_.at(data[i]);
            size_t code2 = chars_.at(data[i+1]);
            result = code1 * 45 + code2;
            mask_size = 11;
        }
        context.stream.AppendBits(result, mask_size);
    }
}

// =============================================================================

const std::unordered_set<char> NumericEncodeProvider::digits_ = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

// =============================================================================

bool NumericEncodeProvider::IsDataSupported(const std::string& data) const
{
    return std::all_of(data.begin(), data.end(), [](char c){ return digits_.count(c) > 0;});
}

// =============================================================================

void NumericEncodeProvider::ConvertInput(const std::string& data, Context& context) const
{
    for (size_t i = 0; i < data.size(); i += 3)
    {
        size_t tail = data.size() - i;
        uint8_t mask_size = 0;
        uint16_t result = 0;

        if (tail == 1)
        {
            result = data[i] - '0';
            mask_size = 4;
        }
        else if (tail == 2)
        {
            result = (data[i] - '0') * 10 + (data[i+1] - '0');
            mask_size = 7;
        }
        else
        {
            result = (data[i] - '0') * 100 + (data[i+1] - '0') * 10 + (data[i+2] - '0');
            mask_size = 10;
        }
        context.stream.AppendBits(result, mask_size);
    }
}

// =============================================================================

bool BytesEncodeProvider::IsDataSupported(const std::string&) const
{
    return true;
}

// =============================================================================

void BytesEncodeProvider::ConvertInput(const std::string& data, Context& context) const
{
    // TODO: check if string is UTF-8
    for (char c : data)
        context.stream.AppendBits(static_cast<uint8_t>(c), BITS_PER_BYTE);
}

// =============================================================================

EncodeProviderPtr EncodeProviderFactory::GetProvider(EncodingType type)
{
    switch (type)
    {
        case EncodingType::ALPHANUMERIC : return std::make_unique<AlphaNumericEncodeProvider>();
        case EncodingType::NUNERIC      : return std::make_unique<NumericEncodeProvider>();
        case EncodingType::BYTES        : return std::make_unique<BytesEncodeProvider>();
        case EncodingType::KANJI        : throw Error("Kanji encoder is not supported");
        default                         : throw Error("Unknown encoding type");
    }
}

// =============================================================================

} // namespace myqro

// =============================================================================
