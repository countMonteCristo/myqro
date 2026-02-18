#include "canvas.hpp"

#include <cmath>
#include <iostream>
#include <numeric>
#include <format>
#include <fstream>

#include "bits.hpp"
#include "defines.hpp"
#include "error.hpp"
#include "logger.hpp"
#include "utils.hpp"


// =============================================================================

namespace myqro
{

// =============================================================================

const char* PatternNameToString(Pattern p)
{
    switch (p)
    {
        case Pattern::UNKNOWN:          return "UNKNOWN";
        case Pattern::INDENT:           return "INDENT";
        case Pattern::SEARCH:           return "SEARCH";
        case Pattern::LEVELING:         return "LEVELING";
        case Pattern::SYNC:             return "SYNC";
        case Pattern::MASK_CORRECTION:  return "MASK_CORRECTION";
        case Pattern::VERSION:          return "VERSION";
        case Pattern::DATA:             return "DATA";
    }
    throw Error("Unknown Pattern");
}

// =============================================================================

Canvas::Canvas(size_t version) :
    version_(version),
    size_(21 + (version - 1) * 4)
{
    cells_.resize(size_*size_);
}

// =============================================================================

void Canvas::SetupSearchPatterns()
{
    PlaceSearchPattern(-1, -1);
    PlaceSearchPattern(-1, size_ - SEARCH_PATTERN_SIZE);
    PlaceSearchPattern(size_ - SEARCH_PATTERN_SIZE, -1);
}

// =============================================================================

void Canvas::SetupLevelingPatterns()
{
    if (version_ >= 2)
    {
        const auto& centers = LevelingPatterns[version_ - 1];
        for (size_t p: centers)
            for (size_t q: centers)
                PlaceLevelingPattern(p, q);
    }
}

// =============================================================================

void Canvas::SetupSyncLines()
{
    uint8_t value = BLACK;
    static const int b = SEARCH_PATTERN_SIZE - 2;
    for (int a = size_ - SEARCH_PATTERN_SIZE + 1; a > b; a--)
    {
        Cell& c1 = At(a, b);
        if (c1.kind == Pattern::UNKNOWN)
        {
            c1.kind = Pattern::SYNC;
            c1.value = value;
        }

        Cell& c2 = At(b, a);
        if (c2.kind == Pattern::UNKNOWN)
        {
            c2.kind = Pattern::SYNC;
            c2.value = value;
        }

        value = 1 - value;
    }
}

// =============================================================================

void Canvas::SetupVersionCode()
{
    static constexpr int mask_size = 3;
    static constexpr int bit_size = 6;
    if (version_ >= 7)
    {
        uint32_t code = VersionCode[version_ - 1];
        uint32_t mask[mask_size] = {
            (code & 0b111111000000000000) >> (2*bit_size),
            (code & 0b000000111111000000) >> (1*bit_size),
            (code & 0b000000000000111111) >> (0*bit_size),
        };

        int start = size_ - SEARCH_PATTERN_SIZE - mask_size;
        for (int r = 0; r < mask_size; r++)
        {
            uint32_t m = mask[r];
            for (int c = 0; c < bit_size; c++)
            {
                uint8_t value = GetBit(m, bit_size - c - 1);
                At(start + r, c) = {Pattern::VERSION, value};
                At(c, start + r) = {Pattern::VERSION, value};
            }
        }
    }
}

// =============================================================================

void Canvas::FillData(CorrectionLevel cl, size_t mask_id, const DataStream& stream)
{
    if (mask_id >= MaskFunctions.size())
        throw Error(std::format("No such mask_id: {}", mask_id));

    auto mask = MaskFunctions[mask_id];
    PlaceCorrectionMaskCode(cl, mask_id);

    auto fun = [&stream, mask, this](size_t index, size_t r, size_t c)
    {
        uint8_t value = (index >= stream.Size()) ? 0 : stream.BitAt(index);
        uint8_t module_value = 1 - (value ^ (mask(c, r) != 0));
        At(r, c) = {Pattern::DATA, module_value};
        index++;
    };

    IterateDataModules(Pattern::UNKNOWN, fun);
}


// =============================================================================

void Canvas::DebugPatterns(std::ostream& os) const
{
    for (size_t row = 0; row < size_; row++)
    {
        for (size_t col = 0; col < size_; col++)
        {
            const Cell& c = At(row, col);
            os << std::format("{:>2}", static_cast<size_t>(c.kind));
            if (col + 1 < size_) os << " ";
        }
        os << std::endl;
    }
}

// =============================================================================

void Canvas::DebugOutputFillDataOrder(std::ostream& os)
{
    std::vector<int> modules(size_*size_, 0);
    for (size_t i = 0; i < cells_.size(); ++i)
    {
        const Cell& cell = cells_[i];
        modules[i] = (cell.kind == Pattern::DATA) ? 0 : -static_cast<int>(cell.kind);
    }

    IterateDataModules(Pattern::DATA, [&modules, this](size_t i, size_t r, size_t c){
        modules[Index(r, c)] = i;
    });

    for (size_t row = 0; row < size_; row++)
    {
        for (size_t col = 0; col < size_; col++)
        {
            int v = modules[Index(row, col)];
            os << std::format("{:>4}", v);
            if (col + 1 != size_) os << " ";
        }
        os << std::endl;
    }
}

// =============================================================================

size_t Canvas::Penalty(size_t mask_id) const
{
    size_t result = 0;

    static const size_t square_penalty = 3;
    static const size_t pattern_penalty = 120;

    // Find horizontal bars 5+ units long
    static const size_t min_len = 5;
    for (size_t row = 0; row < size_; ++row)
    {
        for (size_t col = 0; col < size_;)
        {
            uint8_t color = At(row, col).value;
            size_t i = 1;
            while (col + i < size_ && At(row, col + i).value == color)
                ++i;

            if (i >= min_len) result += (i - 2);
            col += i;
        }
    }
    // Find vertical bars 5+ units long
    for (size_t col = 0; col < size_; ++col)
    {
        for (size_t row = 0; row < size_;)
        {
            uint8_t color = At(row, col).value;
            size_t i = 1;
            while (row + i < size_ && At(row + i, col).value == color)
                ++i;

            if (i >= min_len) result += (i - 2);
            row += i;
        }
    }

    // Find 2x2 squares of same color
    const size_t sq = 2;
    for (size_t row = 0; row + sq < size_ + 1; ++row)
    {
        for (size_t col = 0; col + sq < size_ + 1; ++col)
            if (HasSameColorSquare(row, col, sq)) result += square_penalty;
    }

    // Find horizontal "# ### #" with at least 4 possible white modules at one of the sides (or both)
    static const size_t pat_len = 7;
    static const size_t strip_len = 4;
    for (size_t row = 0; row < size_; ++row)
    {
        for (size_t col = 0; col + pat_len < size_ + 1;)
        {
            if (At(row, col+0).value == BLACK &&
                At(row, col+1).value == WHITE &&
                At(row, col+2).value == BLACK &&
                At(row, col+3).value == BLACK &&
                At(row, col+4).value == BLACK &&
                At(row, col+5).value == WHITE &&
                At(row, col+6).value == BLACK)
            {
                bool has_before = (col > strip_len) ? HasColorStripe(row, col - 1, 0, -1, strip_len) : false;
                bool has_after = (col + pat_len < size_) ? HasColorStripe(row, col + pat_len, 0, 1, strip_len) : false;

                if (has_before || has_after)
                    result += pattern_penalty;

                if (has_after)
                    col += pat_len + strip_len;
                else if (has_before)
                    col += pat_len;
                else
                    col += 1;
            }
            else
                ++col;
        }
    }

    // Find vertical "# ### #" with at least 4 possible white modules at one of the sides (or both)
    for (size_t col = 0; col < size_; ++col)
    {
        for (size_t row = 0; row + pat_len < size_ + 1;)
        {
            if (At(row+0, col).value == BLACK &&
                At(row+1, col).value == WHITE &&
                At(row+2, col).value == BLACK &&
                At(row+3, col).value == BLACK &&
                At(row+4, col).value == BLACK &&
                At(row+5, col).value == WHITE &&
                At(row+6, col).value == BLACK)
            {
                bool has_before = (row > strip_len) ? HasColorStripe(row-1, col, -1, 0, strip_len) : false;
                bool has_after = (row + pat_len < size_) ? HasColorStripe(row+pat_len, col, 1, 0, strip_len) : false;

                if (has_before || has_after)
                    result += pattern_penalty;

                if (has_after)
                    row += pat_len + strip_len;
                else if (has_before)
                    row += pat_len;
                else
                    row += 1;
            }
            else
                ++row;
        }
    }

    size_t count_black = std::accumulate(cells_.begin(), cells_.end(), 0ULL,
                                         [](size_t acc, const Cell& c) { return acc + c.value; });

    result += static_cast<size_t>(std::fabs(100 * static_cast<float>(count_black) / (size_ * size_) - 50)) * 2;

    LogDebug("Penalty: mask={} result={}", mask_id, result);
    return result;
}

// =============================================================================

void Canvas::PlaceSearchPattern(int row, int col)
{
    for (int r = row; r <= row + SEARCH_PATTERN_SIZE; r++)
    {
        for (int c = col; c <= col + SEARCH_PATTERN_SIZE; c++)
        {
            if (!IsInside(r, c)) continue;

            Cell& cell = At(r, c);
            cell.kind = Pattern::SEARCH;

            // самая внешняя белая граница
            if (r == row || r == row + SEARCH_PATTERN_SIZE || c == col || c == col + SEARCH_PATTERN_SIZE)
            {
                cell.value = WHITE;
                continue;
            }

            // внешняя чёрная граница
            if (r == row + 1 || r == row + SEARCH_PATTERN_SIZE - 1 || c == col + 1 || c == col + SEARCH_PATTERN_SIZE - 1)
            {
                cell.value = BLACK;
                continue;
            }

            // внутренняя белая рамка
            if (r == row + 2 || r == row + SEARCH_PATTERN_SIZE - 2 || c == col + 2 || c == col + SEARCH_PATTERN_SIZE - 2)
            {
                cell.value = WHITE;
                continue;
            }

            // внутренний чёрный квадрат 3x3
            cell.value = BLACK;
        }
    }
}

// =============================================================================

void Canvas::PlaceLevelingPattern(int row, int col)
{
    static const int half_size = 2;
    // check if leveling pattern intersects with search pattern
    for (int r = row - half_size; r <= row + half_size; r++)
    {
        for (int c = col - half_size; c <= col + half_size; c++)
        {
            if (!IsInside(r, c))
                throw Error("Trying to place leveling pattern outside of the code canvas");
            Cell& cell = At(r, c);
            if (cell.kind == Pattern::SEARCH)
            {
                LogDebug("Can't place leveling pattern module at ({},{}): module is occupied with {}",
                         r, c, PatternNameToString(cell.kind));
                return;
            }
        }
    }

    for (int r = row - half_size; r <= row + half_size; r++)
    {
        for (int c = col - half_size; c <= col + half_size; c++)
        {
            Cell& cell = At(r, c);
            cell.kind = Pattern::LEVELING;
            if ((r == row - half_size) || (r == row + half_size) ||
                (c == col - half_size) || (c == col + half_size) ||
                (r == row && c == col))
                cell.value = BLACK;
            else
                cell.value = WHITE;
        }
    }
}

// =============================================================================

void Canvas::PlaceCorrectionMaskCode(CorrectionLevel cl, size_t mask_id)
{
    static constexpr size_t code_size = 2*SEARCH_PATTERN_SIZE - 1;
    size_t code = CorrectionLevelMaskCode.at(cl)[mask_id];

    // vertical part of code right to the bottom left search square
    for (int r = 0; r < SEARCH_PATTERN_SIZE - 1; r++)
    {
        uint8_t value = GetBit(code, code_size - r - 1);
        At(size_ - 1 - r, SEARCH_PATTERN_SIZE) = {Pattern::MASK_CORRECTION, value};
    }

    // this square must always be black
    At(size_ - SEARCH_PATTERN_SIZE, SEARCH_PATTERN_SIZE) = {Pattern::MASK_CORRECTION, 1};

    // horisontal part of code below the top right search square
    for (int c = 0; c < SEARCH_PATTERN_SIZE; c++)
    {
        uint8_t value = GetBit(code, code_size - c - SEARCH_PATTERN_SIZE);
        At(SEARCH_PATTERN_SIZE, size_ - SEARCH_PATTERN_SIZE + c) = {Pattern::MASK_CORRECTION, value};
    }

    // horisontal part of code below the top left search square
    for (int c = 0; c < SEARCH_PATTERN_SIZE - 1; c++)
    {
        uint8_t value = GetBit(code, code_size - c - 1);
        int col = c;
        if (col >= SEARCH_PATTERN_SIZE - 2)
            col += 1;

        At(SEARCH_PATTERN_SIZE, col) = {Pattern::MASK_CORRECTION, value};
    }

    // vertical part of code right to the top left search square
    for (int r = 0; r < SEARCH_PATTERN_SIZE; r++)
    {
        uint8_t value = GetBit(code, code_size - r - SEARCH_PATTERN_SIZE);
        int row = SEARCH_PATTERN_SIZE - r;
        if (row <= SEARCH_PATTERN_SIZE - 2)
            row -= 1;

        At(row, SEARCH_PATTERN_SIZE) = {Pattern::MASK_CORRECTION, value};
    }
}

// =============================================================================

bool Canvas::HasSameColorSquare(size_t row, size_t col, size_t sq) const
{
    uint8_t color = At(row, col).value;
    for (size_t dr = 0; dr < sq; ++dr)
    {
        for (size_t dc = 0; dc < sq; ++dc)
        {
            if (!IsInside(row+dr, col+dc) || At(row+dr, col+dc).value != color)
                return false;
        }
    }
    return true;
}

// =============================================================================

bool Canvas::HasColorStripe(size_t row, size_t col, int dr, int dc, size_t len, uint8_t color) const
{
    int r = static_cast<int>(row);
    int c = static_cast<int>(col);
    for (size_t i = 0; i < len; (r+=dr, c+=dc, ++i))
    {
        if (!IsInside(r, c) || At(r, c).value != color) return false;
    }
    return true;
}

// =============================================================================

void Canvas::IterateDataModules(Pattern pattern, std::function<void(size_t, size_t, size_t)> f)
{
    static const std::array<Dir, 2> dd[2]{
        {Dir{0, -1}, Dir{-1, 1}},   // up
        {Dir{0, -1}, Dir{1,  1}},   // down
    };

    size_t index = 0;
    size_t n_strip = size_ / 2;
    for (size_t i = 0; i < n_strip; i++)
    {
        uint8_t down = i % 2;
        int r = down ? 0 : size_ - 1;
        int c = size_ - 1 - 2 * i;
        if (c <= SEARCH_PATTERN_SIZE - 2)
            c--;

        uint8_t d = 0;
        while (IsInside(r, c))
        {
            if (At(r, c).kind == pattern)
            {
                f(index, r, c);
                index++;
            }

            const Dir& dir = dd[down][d];
            r += dir.dr;
            c += dir.dc;
            d = 1 - d;
        }
    }
}

// =============================================================================

} // namespace myqro

// =============================================================================
