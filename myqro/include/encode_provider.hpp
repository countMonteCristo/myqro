#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "context.hpp"


// =============================================================================

namespace myqro
{

// =============================================================================

class EncodeProvider
{
public:
    virtual ~EncodeProvider() {}

    virtual const char* GetProviderName() const = 0;
    virtual EncodingType GetEncodingType() const = 0;

    Context Encode(const std::string& data, CorrectionLevel cl = CorrectionLevel::M) const;

    virtual bool IsDataSupported(const std::string& data) const = 0;
    virtual void ConvertInput(const std::string& data, Context& context) const = 0;
    void PrepareServiceFields(Context& context) const;
    void AddTailZeros(Context& context) const;
    void AddRequiredVersionTailBytes(Context& context) const;
    void PrepareBlocks(Context& context) const;
    void PrepareOutput(Context& context) const;

private:
    static std::pair<size_t, size_t> EstimateVersion(const DataStream& stream, CorrectionLevel cl);
};

using EncodeProviderPtr = std::unique_ptr<EncodeProvider>;

// =============================================================================

class NumericEncodeProvider : public EncodeProvider
{
public:
    bool IsDataSupported(const std::string& data) const final;

    const char* GetProviderName() const final { return "NumericEncodeProvider"; }
    EncodingType GetEncodingType() const final { return EncodingType::NUNERIC; }

    void ConvertInput(const std::string& data, Context& context) const final;

private:
    static const std::unordered_set<char> digits_;
};

// =============================================================================

class AlphaNumericEncodeProvider : public EncodeProvider
{
public:
    bool IsDataSupported(const std::string& data) const final;

    const char* GetProviderName() const final { return "AlphaNumericEncodeProvider"; }
    EncodingType GetEncodingType() const final { return EncodingType::ALPHANUMERIC; }

    void ConvertInput(const std::string& data, Context& context) const final;

private:
    static const std::unordered_map<char, uint8_t> chars_;
};

// =============================================================================

class BytesEncodeProvider : public EncodeProvider
{
public:
    bool IsDataSupported(const std::string& data) const final;

    const char* GetProviderName() const final { return "BytesEncodeProvider"; }
    EncodingType GetEncodingType() const final { return EncodingType::BYTES; }

private:
    void ConvertInput(const std::string& data, Context& context) const final;
};

// =============================================================================

class EncodeProviderFactory
{
public:
    static EncodeProviderPtr GetProvider(EncodingType type);
};

// =============================================================================

} // namespace myqro

// =============================================================================
