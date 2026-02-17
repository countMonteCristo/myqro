#pragma once

#include <concepts>
#include <cstdint>


// =============================================================================

namespace myqro
{

// =============================================================================

template <std::integral T>
uint8_t GetBit(T value, uint8_t pos)
{
    static const T one = 1;
    return static_cast<uint8_t>(((value & (one << pos)) >> pos) & one);
}

// =============================================================================

} // namespace myqro

// =============================================================================
