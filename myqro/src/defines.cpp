#include "defines.hpp"

#include <format>
// #include <stdexcept>

#include "error.hpp"


// =============================================================================

namespace myqro
{

// =============================================================================

EncodingType EncodingTypeFromString(const std::string& type_str)
{
    if (type_str == "num")   return EncodingType::NUNERIC;
    if (type_str == "alnum") return EncodingType::ALPHANUMERIC;
    if (type_str == "bytes") return EncodingType::BYTES;
    if (type_str == "kanji") return EncodingType::KANJI;
    throw Error("Unknown encoding type parsed: " + type_str);
}

// =============================================================================

const char* EncodingTypeToString(EncodingType e)
{
    switch (e)
    {
        case EncodingType::NUNERIC: return "num";
        case EncodingType::ALPHANUMERIC: return "alnum";
        case EncodingType::BYTES: return "bytes";
        case EncodingType::KANJI: return "kanji";
    }
    throw Error("Can not convert unknown encoding type to string");
}

// =============================================================================

const char* CorrectionLevelToString(CorrectionLevel cl)
{
    switch (cl)
    {
        case CorrectionLevel::L: return "L";
        case CorrectionLevel::M: return "M";
        case CorrectionLevel::Q: return "Q";
        case CorrectionLevel::H: return "H";
    }
    throw Error("Can not convert unknown correction level to string");
}

// =============================================================================

CorrectionLevel CorrectionLevelFromString(const std::string& s)
{
    if (s == "L") return CorrectionLevel::L;
    if (s == "M") return CorrectionLevel::M;
    if (s == "Q") return CorrectionLevel::Q;
    if (s == "H") return CorrectionLevel::H;
    throw Error(std::format("Unknown correction level: {}", s));
}

// =============================================================================

} // namespace myqro

// =============================================================================
