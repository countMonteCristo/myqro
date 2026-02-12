#include <cppunit/extensions/HelperMacros.h>

#include "encode_provider.hpp"


// =============================================================================

namespace myqro::test
{

// =============================================================================

class TestEncoder : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestEncoder);

    CPPUNIT_TEST(TestEncodingTypeNumeric);
    CPPUNIT_TEST(TestEncodingAlphaNumeric);
    CPPUNIT_TEST(TestConvertInput);
    CPPUNIT_TEST(TestAddTailZeros);
    CPPUNIT_TEST(TestAddRequiredVersionTailBytes);
    CPPUNIT_TEST(TestGenCorrBlock);

    CPPUNIT_TEST_SUITE_END();

protected:
    void TestEncodingTypeNumeric();
    void TestEncodingAlphaNumeric();
    void TestConvertInput();
    void TestAddTailZeros();
    void TestAddRequiredVersionTailBytes();
    void TestGenCorrBlock();
};

// =============================================================================

CPPUNIT_TEST_SUITE_REGISTRATION(TestEncoder);

// =============================================================================

void TestEncoder::TestEncodingTypeNumeric()
{
    auto p = myqro::EncodeProviderFactory::GetProvider(myqro::EncodingType::NUNERIC);
    for (const auto& data: {"12345", "910239471298467812", "29863812"})
        CPPUNIT_ASSERT_EQUAL(true, p->IsDataSupported(data));
    for (const auto& data: {"12345 ", "910s239471298467812", "298.63812"})
        CPPUNIT_ASSERT_EQUAL(false, p->IsDataSupported(data));
}

// =============================================================================

void TestEncoder::TestEncodingAlphaNumeric()
{
    auto p = myqro::EncodeProviderFactory::GetProvider(myqro::EncodingType::ALPHANUMERIC);
    for (const auto& data: {"12%34-5", "QW9LAK SH+4HJQW  VS678:12", "QH$WN*KD$ ."})
        CPPUNIT_ASSERT_EQUAL(true, p->IsDataSupported(data));
    for (const auto& data: {"12345q", "a@b", "x^y"})
        CPPUNIT_ASSERT_EQUAL(false, p->IsDataSupported(data));
}

void TestEncoder::TestConvertInput()
{
    using Case = std::tuple<myqro::EncodingType, const char*, const char*>;
    for (auto [e, input, expected]: {Case{myqro::EncodingType::NUNERIC,      "12345678", "000111101101110010001001110"},
                                     Case{myqro::EncodingType::ALPHANUMERIC, "HELLO",    "0110000101101111000110011000"},
                                     Case{myqro::EncodingType::BYTES,        "Хабр",     "1101000010100101110100001011000011010000101100011101000110000000"},})
    {
        Context ctx(input, CorrectionLevel::M);
        auto p = myqro::EncodeProviderFactory::GetProvider(e);
        p->ConvertInput(input, ctx);

        std::stringstream ss;
        ctx.stream.Print(ss);

        CPPUNIT_ASSERT_EQUAL(std::string(expected), ss.str());
    }
}

void TestEncoder::TestAddTailZeros()
{
    Context ctx("", CorrectionLevel::M);
    auto p = myqro::EncodeProviderFactory::GetProvider(myqro::EncodingType::NUNERIC);
    ctx.stream.SetBitSize(13);
    ctx.stream.SetBitAt(12, 1);

    std::stringstream s1, s2;
    ctx.stream.Print(s1);
    CPPUNIT_ASSERT_EQUAL(std::string("0000000000001"), s1.str());

    p->AddTailZeros(ctx);
    ctx.stream.Print(s2);
    CPPUNIT_ASSERT_EQUAL(std::string("0000000000001000"), s2.str());
}

void TestEncoder::TestAddRequiredVersionTailBytes()
{
    Context ctx("", CorrectionLevel::M);
    ctx.max_data_size = 32;
    auto p = myqro::EncodeProviderFactory::GetProvider(myqro::EncodingType::NUNERIC);
    ctx.stream.SetBitSize(8);
    ctx.stream.SetBitAt(7, 1);

    std::stringstream s1, s2;
    ctx.stream.Print(s1, "-");
    CPPUNIT_ASSERT_EQUAL(std::string("00000001"), s1.str());

    p->AddRequiredVersionTailBytes(ctx);
    ctx.stream.Print(s2, "-");
    CPPUNIT_ASSERT_EQUAL(std::string("00000001-11101100-00010001-11101100"), s2.str());
}

void TestEncoder::TestGenCorrBlock()
{
    size_t n_corr_bytes = 28;
    myqro::ArrayType a{64, 196, 132, 84, 196, 196, 242, 194, 4, 132, 20, 37, 34, 16, 236, 17};
    myqro::ArrayType expected{16,  85, 12, 231,  54, 54, 140, 70, 118,  84,  10, 174, 235, 197,
                              99, 218, 12, 254, 246,  4, 190, 56,  39, 217, 115, 189, 193,  24};
    myqro::Block block{a.begin(), a.end()};

    myqro::ArrayType x = myqro::GenerateCorrectionBlock(block, n_corr_bytes);
}

} // namespace myqro::test

// =============================================================================
