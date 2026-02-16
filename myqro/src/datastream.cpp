#include "datastream.hpp"

#include <format>

#include "defines.hpp"


// =============================================================================

namespace myqro
{

// =============================================================================

void DataStream::SetBitSize(size_t nbits)
{
    bit_size_ = nbits;
    size_t nbytes = bit_size_ / BITS_PER_BYTE;
    if (bit_size_ % BITS_PER_BYTE > 0)
        nbytes++;
    data_.resize(nbytes);
}

// =============================================================================

uint8_t DataStream::BitAt(size_t pos) const
{
    size_t byte_pos = pos / BITS_PER_BYTE;
    uint8_t rem = static_cast<uint8_t>(pos % BITS_PER_BYTE);
    uint8_t byte = data_.at(byte_pos);
    uint8_t shift = static_cast<uint8_t>(BITS_PER_BYTE - 1);
    return static_cast<uint8_t>(byte << rem) >> shift;
}

// =============================================================================

void DataStream::SetBitAt(size_t pos, uint8_t bit)
{
    if (bit > 1)
        throw std::runtime_error("Only values 0 and 1 are suported as bit value");

    size_t byte_pos = pos / BITS_PER_BYTE;
    size_t rem = pos % BITS_PER_BYTE;

    uint8_t& byte = data_.at(byte_pos);
    uint8_t mask = (1 << (BITS_PER_BYTE - 1 - rem));
    if (bit == 1)
    {
        byte |= mask;
    }
    else
    {
        mask = ~mask;
        byte &= mask;
    }
}

// =============================================================================

std::vector<Block> DataStream::GenerateBlocks(size_t count)
{
    size_t n_extended = ByteSize() % count;
    size_t n_ordinary = count - n_extended;
    size_t n_bytes_per_block = ByteSize() / count;

    std::vector<Block> result;
    result.reserve(count);

    ArrayType::iterator it = data_.begin();
    for (size_t i = 0; i < count; i++)
    {
        if (i < n_ordinary)
        {
            result.emplace_back(it, it + n_bytes_per_block);
            it += n_bytes_per_block;
        }
        else
        {
            result.emplace_back(it, it + n_bytes_per_block + 1);
            it += n_bytes_per_block + 1;
        }
    }

    return result;
}

// =============================================================================

void DataStream::Print(std::ostream& stream, const std::string& sep) const
{
    PrintArrayBits(stream, data_, bit_size_, sep);
}

// =============================================================================

void PrintArrayBits(std::ostream& os, const ArrayType& data, size_t bit_size, const std::string& sep)
{
    static const uint8_t bitmask = (1 << (BITS_PER_BYTE - 1));
    size_t bit_index = 0;
    for (uint8_t byte: data)
    {
        for (size_t i = 0; (i < BITS_PER_BYTE) && (bit_index < bit_size); bit_index++, i++)
        {
            uint8_t bit = (byte & bitmask) >> (BITS_PER_BYTE - 1);
            os << "01"[bit];
            byte <<= 1;
        }
        if (bit_index < bit_size)
           os << sep;
    }
}

// =============================================================================

void PrintBlockBits(std::ostream& os, const Block& block, const std::string& sep)
{
    static const uint8_t bitmask = (1 << (BITS_PER_BYTE - 1));
    for (auto it = block.begin; it != block.end; ++it)
    {
        uint8_t byte = *it;
        for (size_t i = 0; i < BITS_PER_BYTE; i++)
        {
            uint8_t bit = (byte & bitmask) >> (BITS_PER_BYTE - 1);
            os << "01"[bit];
            byte <<= 1;
        }
        if ((it + 1) != block.end)
           os << sep;
    }
}

// =============================================================================

DataStream& operator<<(DataStream& left, const DataStream& right)
{
    for (size_t i = 0; i < right.ByteSize(); i++)
    {
        uint8_t byte = right.ByteAt(i);
        if (i < right.ByteSize() - 1)
        {
            left.AppendBits(byte, BITS_PER_BYTE);
        }
        else
        {
            size_t mask_size = right.Size() % BITS_PER_BYTE;
            if (mask_size == 0)
            {
                left.AppendBits(byte, BITS_PER_BYTE);
            }
            else
            {
                byte >>= (BITS_PER_BYTE - mask_size);
                left.AppendBits(byte, mask_size);
            }

        }
    }

    return left;
}

// =============================================================================

void DebugArray(const ArrayType& array, std::ostream& s)
{
    for (const auto& x: array)
        s << static_cast<size_t>(x) << ' ';
    s << std::endl;
}

// =============================================================================

// 1. Берём первый элемент массива, сохраняем его значение в переменной А и удаляем его из массива
//    (все следующие значения сдвигаются на одну ячейку влево, последний элемент заполняется нулём).
//
// 2. Если А равно нулю, то пропустить следующие действия (до конца списка) и перейти к следующей итерации цикла.
//
// 3. Находим соответствующее числу А число в таблице 8, заносим его в переменную Б.
//
// 4. Далее для N первых элементов, где N — количество байтов коррекции, i — счётчик цикла:
//    * К i-му значению генерирующего многочлена надо прибавить значение Б и записать эту сумму
//      в переменную В (сам многочлен не изменять).
//    * Если В больше 254, надо использовать её остаток при делении на 255 (именно 255, а не 256).
//    * Найти соответствующее В значение в таблице 7 и произвести опеацию побитового сложения
//      по модулю 2 (XOR, во многих языках программирования оператор ^) с i-м значением
//      подготовленного массива и записать полученное значение в i-ю ячейку подготовленного массива.

// TODO: use cyclic buffer instead of erase+push_back?
ArrayType GenerateCorrectionBlock(const Block& block, size_t n_correction_bytes)
{
    const auto& poly = GeneratingPolynomial.at(n_correction_bytes);

    ArrayType result;
    result.resize(std::max(block.Size(), n_correction_bytes));
    std::copy(block.begin, block.end, result.begin());
    for (size_t j = 0; j < block.Size(); j++)
    {
        uint8_t A = result.front();
        result.erase(result.begin());
        result.push_back(0);

        if (A == 0) continue;

        size_t B = static_cast<size_t>(ReverseGaloisField[A]);
        for (size_t i = 0; i < n_correction_bytes; i++)
        {
            uint8_t C = (poly[i] + B) % 255;
            result[i] ^= GaloisField[C];
        }
    }

    return result;
}

// =============================================================================

} // namespace myqro

// =============================================================================
