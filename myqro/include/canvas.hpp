#pragma once

#include <cstdint>
#include <functional>
#include <ostream>
#include <vector>

#include "datastream.hpp"


// =============================================================================

namespace myqro
{

// =============================================================================

struct Dir
{
    int dr;
    int dc;
};

// =============================================================================

enum class Pattern : size_t
{
    UNKNOWN = 0,
    INDENT = 1,
    SEARCH = 2,
    LEVELING = 3,
    SYNC = 4,
    MASK_CORRECTION = 5,
    VERSION = 6,
    DATA = 7,
};

const char* PatternNameToString(Pattern p);

// =============================================================================

struct Cell
{
    Pattern kind;
    uint8_t value;

    Cell() : kind(Pattern::UNKNOWN) {}
    Cell(Pattern k, uint8_t v) : kind(k), value(v) {}
};

// =============================================================================

class Canvas
{
public:
    Canvas(size_t version);

    void SetupSearchPatterns();
    void SetupLevelingPatterns();
    void SetupSyncLines();
    void SetupVersionCode();

    void FillData(CorrectionLevel cl, size_t mask_id, const DataStream& stream);

    void DebugPatterns(std::ostream& os) const;
    void DebugOutputFillDataOrder(std::ostream& os);

    Cell& At(size_t row, size_t col) { return cells_[Index(row, col)]; }
    const Cell& At(size_t row, size_t col) const { return cells_[Index(row, col)]; }
    bool IsInside(int row, int col) const { return row >= 0 && row < static_cast<int>(size_) && col >= 0 && col < static_cast<int>(size_); }

    size_t Penalty(size_t mask_id) const;

    size_t Version() const { return version_; }
    size_t Size() const { return size_; }

private:
    void PlaceSearchPattern(int row, int col);
    void PlaceLevelingPattern(int row, int col);
    void PlaceCorrectionMaskCode(CorrectionLevel cl, size_t mask_id);
    size_t Index(int row, int col) const { return row*size_ + col; }

    bool HasSameColorSquare(size_t row, size_t col, size_t sq) const;
    bool HasColorStripe(size_t row, size_t col, int dr, int dc, size_t len, uint8_t color=WHITE) const;

    void IterateDataModules(Pattern pattern, std::function<void(size_t, size_t, size_t)> f);

private:
    size_t version_;
    size_t size_;
    std::vector<Cell> cells_;
};

// =============================================================================

} // namespace myqro

// =============================================================================
