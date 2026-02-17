#include "canvas.hpp"

#include <iostream>
#include <format>
#include <fstream>

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
    uint8_t value = 1;
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
    if (version_ >= 7)
    {
        uint32_t code = VersionCode[version_ - 1];
        uint32_t mask[3] = {
            (code & 0b111111000000000000) >> (2*6),
            (code & 0b000000111111000000) >> (1*6),
            (code & 0b000000000000111111) >> (0*6),
        };

        int start = size_ - SEARCH_PATTERN_SIZE - 3;
        for (int r = 0; r < 3; r++)
        {
            uint32_t m = mask[r];
            for (int c = 0; c < 6; c++)
            {
                uint8_t value = (m & (1 << (6 - c - 1))) >> (6 - c - 1);

                Cell& c1 = At(start + r, c);
                c1.kind = Pattern::VERSION;
                c1.value = value;

                Cell& c2 = At(c, start + r);
                c2.kind = Pattern::VERSION;
                c2.value = value;
            }
        }
    }
}

// =============================================================================

void Canvas::FillData(CorrectionLevel cl, size_t mask_id, const DataStream& stream)
{
    if (mask_id >= MaskFunctions.size())
        throw Error(std::format("No such mask_id: {}", mask_id));

    static const std::array<Dir, 2> dd[2]{
        {Dir{0, -1}, Dir{-1, 1}},   // up
        {Dir{0, -1}, Dir{1,  1}},   // down
    };

    auto mask = MaskFunctions[mask_id];
    PlaceCorrectionMaskCode(cl, mask_id);

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
            const Dir& dir = dd[down][d];

            if (At(r, c).kind == Pattern::UNKNOWN)
            {
                uint8_t value = (index >= stream.Size()) ? 0 : stream.BitAt(index);
                uint8_t module_value = 1 - (value ^ (mask(c, r) != 0));
                At(r, c) = {Pattern::DATA, module_value};
                index++;
            }

            r += dir.dr;
            c += dir.dc;
            d = 1 - d;
        }
    }
}

// =============================================================================

// TODO: add indent
// TODO: add scale
void Canvas::DebugOutput(std::ostream& os) const
{
    for (size_t row = 0; row < size_; row++)
    {
        for (size_t col = 0; col < size_; col++)
        {
            const Cell& c = At(row, col);
            os << " #"[c.value];
            // if (c.kind == Pattern::UNKNOWN)
            //     os << "•";
            // else if (c.value == 1) os << "□";
            // else os << "■";
        }
        os << std::endl;
    }
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

// TODO: rewrite it using pattern values
void Canvas::DebugOutputFillDataOrder(std::ostream& os, CorrectionLevel cl, size_t mask_id)
{
    static const std::array<Dir, 2> dd[2]{
        {Dir{0, -1}, Dir{-1, 1}},   // up
        {Dir{0, -1}, Dir{1,  1}},   // down
    };

    PlaceCorrectionMaskCode(cl, mask_id);

    std::vector<int> modules(size_*size_, 0);

    size_t index = 0;
    size_t n_strip = size_ / 2;
    for (size_t i = 0; i < n_strip; i++)
    {
        uint8_t down = i % 2;
        int r_start = down ? 0 : size_ - 1;
        int c_start = size_ - 1 - 2*i;
        if (c_start <= SEARCH_PATTERN_SIZE - 2)
            c_start -= 1;

        int dr = down ? 1 : -1;
        // find first empty module
        int r;
        for (r = r_start; At(r, c_start).kind != Pattern::UNKNOWN; r += dr)
        {
            modules[Index(r, c_start)] = -1;
            modules[Index(r, c_start) - 1] = -1;
        }

        int c = c_start;
        uint8_t d = 0;
        while (IsInside(r, c))
        {
            const Dir& dir = dd[down][d];
            if (At(r, c).kind != Pattern::UNKNOWN)
            {
                modules[Index(r, c)] = -1;
            }
            else
            {
                modules[Index(r, c)] = index;
                index++;
            }

            r += dir.dr;
            c += dir.dc;
            d = 1 - d;
        }
    }

    for (size_t row = 0; row < size_; row++)
    {
        for (size_t col = 0; col < size_; col++)
        {
            int v = modules[row*size_+col];
            os << std::format("{:>4}", v);
            if (col + 1 != size_) os << " ";
        }
        os << std::endl;
    }
}

// =============================================================================

void Canvas::DebugPBM(const std::string& fn) const
{
    std::ofstream os(fn);

    if (!os.is_open())
        throw Error(std::format("Error opening file {}", fn));

    // TODO: customize this
    static const uint8_t indent = 4;

    os << "P1" << std::endl;
    os << size_ + 2*indent << " " << size_ + 2*indent << std::endl;

    int s = static_cast<int>(size_);

    for (int row = -indent; row < s + indent; row++)
    {
        for (int col = -indent; col < s + indent; col++)
        {
            if (!IsInside(row, col))
                os << "0";
            else
                os << "01"[At(row, col).value];
            if (col < s - 1) os << ' ';
        }
        os << std::endl;
    }
    os.close();
}

// =============================================================================

std::string Canvas::Imprint() const
{
    std::string result(cells_.size(), '0');
    for (size_t i = 0; i < cells_.size(); ++i)
    {
        result[i] = " #"[cells_[i].value];
    }
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
                cell.value = 0;
                continue;
            }

            // внешняя чёрная граница
            if (r == row + 1 || r == row + SEARCH_PATTERN_SIZE - 1 || c == col + 1 || c == col + SEARCH_PATTERN_SIZE - 1)
            {
                cell.value = 1;
                continue;
            }

            // внутренняя белая рамка
            if (r == row + 2 || r == row + SEARCH_PATTERN_SIZE - 2 || c == col + 2 || c == col + SEARCH_PATTERN_SIZE - 2)
            {
                cell.value = 0;
                continue;
            }

            // внутренний чёрный квадрат 3x3
            cell.value = 1;
        }
    }
}

// =============================================================================

void Canvas::PlaceLevelingPattern(int row, int col)
{
    // check if levelin pattern intersects with search pattern
    for (int r = row - 2; r <= row + 2; r++)
    {
        for (int c = col - 2; c <= col + 2; c++)
        {
            if (!IsInside(r, c)) continue;
            Cell& cell = At(r, c);
            if (cell.kind == Pattern::SEARCH)
            {
                LogDebug("Can't place leveling pattern module at ({},{}): module is occupied with {}",
                         r, c, PatternNameToString(cell.kind));
                return;
            }
        }
    }

    for (int r = row - 2; r <= row + 2; r++)
    {
        for (int c = col - 2; c <= col + 2; c++)
        {
            if (!IsInside(r, c)) continue;
            Cell& cell = At(r, c);
            if (cell.kind != Pattern::UNKNOWN)
            {
                LogDebug("Can't place leveling pattern module at ({},{}): module is occupied with {}",
                         r, c, PatternNameToString(cell.kind));
                continue;
            }

            cell.kind = Pattern::LEVELING;
            if (r == row - 2 || r == row + 2 || c == col - 2 || c == col + 2 || (r == row && c == col))
                cell.value = 1;
            else
                cell.value = 0;
        }
    }
}

// =============================================================================

void Canvas::PlaceCorrectionMaskCode(CorrectionLevel cl, size_t mask_id)
{
    size_t code = CorrectionLevelMaskCode.at(cl)[mask_id];

    // vertical part of code right to the bottom left search square
    for (int r = 0; r < SEARCH_PATTERN_SIZE - 1; r++)
    {
        size_t shift  = 15 - r - 1;
        int row = size_ - 1 - r;
        uint8_t value = (code & (1 << shift)) >> shift;

        Cell& cell = At(row, SEARCH_PATTERN_SIZE);
        cell.kind = Pattern::MASK_CORRECTION;
        cell.value = value;
    }

    // this square must always be black
    At(size_ - SEARCH_PATTERN_SIZE, SEARCH_PATTERN_SIZE) = Cell(Pattern::MASK_CORRECTION, 1);

    // horisontal part of code below the top right search square
    for (int c = 0; c < SEARCH_PATTERN_SIZE; c++)
    {
        size_t shift  = 15 - c - SEARCH_PATTERN_SIZE;
        int col = size_ - SEARCH_PATTERN_SIZE + c;
        uint8_t value = (code & (1 << shift)) >> shift;

        Cell& cell = At(SEARCH_PATTERN_SIZE, col);
        cell.kind = Pattern::MASK_CORRECTION;
        cell.value = value;
    }

    // horisontal part of code below the top left search square
    for (int c = 0; c < SEARCH_PATTERN_SIZE - 1; c++)
    {
        size_t shift  = 15 - c - 1;
        int col = c;
        if (col >= SEARCH_PATTERN_SIZE - 2)
            col += 1;
        uint8_t value = (code & (1 << shift)) >> shift;

        Cell& cell = At(SEARCH_PATTERN_SIZE, col);
        cell.kind = Pattern::MASK_CORRECTION;
        cell.value = value;
    }

    // vertical part of code right to the top left search square
    for (int r = 0; r < SEARCH_PATTERN_SIZE; r++)
    {
        size_t shift  = 15 - r - SEARCH_PATTERN_SIZE;
        int row = SEARCH_PATTERN_SIZE - r;
        if (row <= SEARCH_PATTERN_SIZE - 2)
            row -= 1;
        uint8_t value = (code & (1 << shift)) >> shift;

        Cell& cell = At(row, SEARCH_PATTERN_SIZE);
        cell.kind = Pattern::MASK_CORRECTION;
        cell.value = value;
    }
}

// =============================================================================

} // namespace myqro

// =============================================================================
