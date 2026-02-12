#pragma once

#include "canvas.hpp"
#include "encode_provider.hpp"


// =============================================================================

namespace myqro
{

// =============================================================================

class Encoder
{
public:
    static Canvas Encode(const std::string& msg, CorrectionLevel cl = CorrectionLevel::M,
                         EncodingType encoding = EncodingType::BYTES, int mask_id = 0);
};

// =============================================================================

} // namespace myqro

// =============================================================================
