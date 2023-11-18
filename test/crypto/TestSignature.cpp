#include <crypto/Base64EncodeDecode.h>
#include <crypto/RSAPrivateKey.h>
#include <crypto/RSAPublicKey.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace lu::crypto;


TEST(TestSignature, checkInvalidFile)
{
    RSAPrivateKey rsaPrivateKey;
    ASSERT_EQ(rsaPrivateKey.load("resource/invalipath"), false);
    RSAPublicKey rsaPublicKey;
    ASSERT_EQ(rsaPublicKey.load("resource/invalipath"), false);
}

TEST(TestSignature, checkFailureToReadFile)
{
    RSAPrivateKey rsaPrivateKey;
    ASSERT_EQ(rsaPrivateKey.load("resource/publick_key.pem"), false);
    RSAPublicKey rsaPublicKey;
    ASSERT_EQ(rsaPublicKey.load("resource/private.pem"), false);
}

TEST(TestSignature, signWithoutSalt)
{
    RSAPrivateKey rsaPrivateKey;
    ASSERT_EQ(rsaPrivateKey.load("resource/private.pem"), true);
    RSAPublicKey rsaPublicKey;
    ASSERT_EQ(rsaPublicKey.load("resource/publick_key.pem"), true);
    std::string data = "User:Stanly, Calims:10001";
    auto token = rsaPrivateKey.getBase64Signature(data);
    ASSERT_EQ(rsaPublicKey.verifyBase64Signature(data, token), true);

    std::string wrongClaim = "User:Stanly, Calims:11001";
    ASSERT_EQ(rsaPublicKey.verifyBase64Signature(wrongClaim, token), false);
}


TEST(TestSignature, singWithSalt)
{
    RSAPrivateKey rsaPrivateKey;
    ASSERT_EQ(rsaPrivateKey.load("resource/private.pem"), true);
    RSAPublicKey rsaPublicKey;
    ASSERT_EQ(rsaPublicKey.load("resource/publick_key.pem"), true);
    std::string data = "User:Stanly, Calims:10001";
    auto token = rsaPrivateKey.getBase64Signature(data, "stanly");
    ASSERT_EQ(rsaPublicKey.verifyBase64Signature(data, token, "stanly"), true);
    ASSERT_EQ(rsaPublicKey.verifyBase64Signature(data, token, "stanly1"), false);
}