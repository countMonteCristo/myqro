#pragma once

#include <concepts>
#include <cstdint>
#include <ostream>
#include <vector>

#include <iostream>

#include "bits.hpp"
#include "defines.hpp"


// =============================================================================

namespace myqro
{

// =============================================================================

constexpr size_t BITS_PER_BYTE = 8;

class DataStream
{
private:

public:
    DataStream() : bit_size_(0) {}
    DataStream(ArrayType&& array) :
        data_(std::move(array)),
        bit_size_(BITS_PER_BYTE * data_.size())
    {}

    DataStream(const ArrayType& array) :
        data_(array),
        bit_size_(BITS_PER_BYTE * data_.size())
    {}

    void SetBitSize(size_t nbits);
    uint8_t BitAt(size_t pos) const;
    void SetBitAt(size_t pos, uint8_t bit = 1);

    size_t Size() const { return bit_size_; }
    size_t ByteSize() const { return data_.size(); }
    uint8_t ByteAt(size_t idx) const { return data_.at(idx); }

    std::vector<Block> GenerateBlocks(size_t count);

    template<std::unsigned_integral T>
    void AppendBits(T bits, uint8_t mask_size)
    {
        size_t type_bits = sizeof(T) * BITS_PER_BYTE;
        bits <<= (type_bits - mask_size);

        size_t start = bit_size_;
        SetBitSize(bit_size_ + mask_size);
        for (size_t i = 0; i < mask_size; i++)
        {
            uint8_t bit = static_cast<uint8_t>(bits >> (type_bits - 1));
            SetBitAt(start + i, bit);
            bits <<= 1;
        }
    }

    void Print(std::ostream& stream, const std::string& sep = "") const;

private:
    ArrayType data_;
    size_t bit_size_;
};

// =============================================================================

DataStream& operator<<(DataStream& left, const DataStream& right);

// =============================================================================

void PrintArrayBits(std::ostream& os, const ArrayType& data, size_t bit_size, const std::string& sep = "");
void PrintBlockBits(std::ostream& os, const Block& block, const std::string& sep = "");

// =============================================================================

ArrayType GenerateCorrectionBlock(const Block& block, size_t n_correction_bytes);

} // namespace myqro

// =============================================================================
