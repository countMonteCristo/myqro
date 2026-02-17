#pragma once

#include "datastream.hpp"
#include "defines.hpp"
#include "error.hpp"


// =============================================================================

namespace myqro
{

// =============================================================================

struct Context
{
    DataStream stream;
    EncodingType encoding;
    CorrectionLevel cl;
    size_t version;
    size_t input_data_size;
    size_t max_data_size;   // in bits
    size_t data_size_field_width;
    std::vector<Block> data_blocks;
    std::vector<ArrayType> correction_blocks;
    ArrayType output;
    const size_t encoding_field_width = 4;

    Context() :
        encoding(EncodingType::BYTES),
        cl(CorrectionLevel::M),
        version(1),
        input_data_size(0),
        max_data_size(0),
        data_size_field_width(0)
    {}

    Context(const std::string& data, CorrectionLevel cl) :
        encoding(EncodingType::BYTES),
        cl(cl),
        version(1),
        input_data_size(data.size()),
        max_data_size(0),
        data_size_field_width(0)
    {}

    size_t GetBlocksCount() const
    {
        if (version < 1)
            throw Error("Can't get blocks count: invalid version");
        return BlocksCount.at(cl)[version - 1];
    }

    size_t GetCorrectionBytesCount() const
    {
        if (version < 1)
            throw Error("Can't get correction bytes count: invalid version");
        return CorrBlockBytes.at(cl)[version - 1];
    }

    size_t GetBytesPerBlock() const
    {
        if (version < 1)
            throw Error("Can't get bytes per block count: invalid version");
        return stream.Size() / GetBlocksCount();
    }
};

// =============================================================================

} // namespace myqro

// =============================================================================
