#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

// =============================================================================

namespace myqro
{

// =============================================================================

enum class EncodingType : uint8_t
{
    NUNERIC         = 0b0001,
    ALPHANUMERIC    = 0b0010,
    BYTES           = 0b0100,
    KANJI           = 0b1111,
};

EncodingType EncodingTypeFromString(const std::string& type_str);
const char* EncodingTypeToString(EncodingType e);

// =============================================================================

// Maximum correction level in percents
enum class CorrectionLevel : size_t
{
    L = 7,
    M = 15,
    Q = 25,
    H = 30,
};

const char* CorrectionLevelToString(CorrectionLevel cl);
CorrectionLevel CorrectionLevelFromString(const std::string& s);

// =============================================================================

using ArrayType = std::vector<uint8_t>;

// =============================================================================

struct Block
{
    ArrayType::iterator begin;
    ArrayType::iterator end;

    size_t Size() const { return end - begin; }
};

// =============================================================================

constexpr size_t MIN_VERSION = 1;
constexpr size_t MAX_VERSION = 40;
constexpr size_t VERSION_ARRAY_SIZE = MAX_VERSION - MIN_VERSION + 1;

// =============================================================================

constexpr size_t MIN_MASK_ID = 0;
constexpr size_t MAX_MASK_ID = 7;
constexpr size_t MASK_ARRAY_SIZE = MAX_MASK_ID - MIN_MASK_ID + 1;

// =============================================================================

constexpr int SEARCH_PATTERN_SIZE = 8;

// =============================================================================

const std::unordered_map<CorrectionLevel, std::array<size_t, VERSION_ARRAY_SIZE>> VersionCorrectionMaxDataSize{
    {CorrectionLevel::L, {152, 272, 440, 640, 864, 1088, 1248, 1552, 1856, 2192, 2592, 2960, 3424, 3688, 4184, 4712, 5176, 5768, 6360, 6888, 7456, 8048, 8752, 9392, 10208, 10960, 11744, 12248, 13048, 13880, 14744, 15640, 16568, 17528, 18448, 19472, 20528, 21616, 22496, 23648}},
    {CorrectionLevel::M, {128, 224, 352, 512, 688,  864,  992, 1232, 1456, 1728, 2032, 2320, 2672, 2920, 3320, 3624, 4056, 4504, 5016, 5352, 5712, 6256, 6880, 7312,  8000,  8496,  9024,  9544, 10136, 10984, 11640, 12328, 13048, 13800, 14496, 15312, 15936, 16816, 17728, 18672}},
    {CorrectionLevel::Q, {104, 176, 272, 384, 496,  608,  704,  880, 1056, 1232, 1440, 1648, 1952, 2088, 2360, 2600, 2936, 3176, 3560, 3880, 4096, 4544, 4912, 5312,  5744,  6032,  6464,  6968,  7288,  7880,  8264,  8920,  9368,  9848, 10288, 10832, 11408, 12016, 12656, 13328}},
    {CorrectionLevel::H, { 72, 128, 208, 288, 368,  480,  528,  688,  800,  976, 1120, 1264, 1440, 1576, 1784, 2024, 2264, 2504, 2728, 3080, 3248, 3536, 3712, 4112,  4304,  4768,  5024,  5288,  5608,  5960,  6344,  6760,  7208,  7688,  7888,  8432,  8768,  9136,  9776, 10208}},
};

// =============================================================================

const std::unordered_map<EncodingType, std::unordered_map<size_t, size_t>> DataSizeFieldWidth{
    {EncodingType::NUNERIC,      {{9, 10}, {26, 12}, {MAX_VERSION, 14}}},
    {EncodingType::ALPHANUMERIC, {{9,  9}, {26, 11}, {MAX_VERSION, 13}}},
    {EncodingType::BYTES,        {{9,  8}, {26, 16}, {MAX_VERSION, 16}}},
};

// =============================================================================

extern const std::unordered_map<CorrectionLevel, std::array<size_t, VERSION_ARRAY_SIZE>> BlocksCount;
extern const std::unordered_map<CorrectionLevel, std::array<size_t, VERSION_ARRAY_SIZE>> CorrBlockBytes;
extern const std::unordered_map<size_t, std::vector<size_t>> GeneratingPolynomial;
extern const std::array<uint8_t, 256> GaloisField;
extern const std::array<uint8_t, 256> ReverseGaloisField;

// =============================================================================

extern const std::array<std::vector<size_t>, VERSION_ARRAY_SIZE> LevelingPatterns;
extern const std::array<uint32_t, VERSION_ARRAY_SIZE> VersionCode;

// =============================================================================

extern const std::array<std::function<uint8_t(size_t, size_t)>, MASK_ARRAY_SIZE> MaskFunctions;
extern const std::unordered_map<CorrectionLevel, std::array<size_t, MASK_ARRAY_SIZE>> CorrectionLevelMaskCode;

} // namespace myqro

// =============================================================================
