#include <crypto/Base64EncodeDecode.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>
#include <common/FixedString.h>

using namespace lu::crypto;


TEST(TestBase64EncodeDecode, testEncode)
{
    
    long long int testData = 123456789;
    DataWrap dataWrap(8);
    std::memcpy(dataWrap.getData(), &testData  , 8);
    std::string encodedString = Base64EncodeDecode::encode(dataWrap);
    lu::common::FixedString<16> encodedFixedString;
    Base64EncodeDecode::encode(&testData, 8, encodedFixedString.data(), 16);
    EXPECT_EQ(encodedString, std::string(encodedFixedString.getCString()));
    EXPECT_EQ(encodedString, "Fc1bBwAAAAA=");
}
