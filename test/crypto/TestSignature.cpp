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
    ASSERT_EQ(token, "sN73AvMOJ+YCakTCONIQyRBJH0hjYZ9LYigK1MwnfV2TcKz9yukq8avwJjoP5cm9stm+Al3TPWwcqjLmFSIuTlI7I44xHMg9R+APc/cTpSA+n16k2tHRqbq9B1TLaSBexRnYJX4cg6dUD/z7C93RYe2uCGseQOU1hrZBrOI/K7ZiZEplBIV3oo7NW/0adrZcToOLE5vTylsbulAS22/stl3CNQ0s7d7Wz9s7EIxuJCE8QnnFnzS3uxO79t35RBlal2T8OdaU+Bvpk6WU21mhgjlt0CFCdTJF3U2MNF0uPCI5rnwSgRh328meDwSBOfYznR5NaZuPGb6AOlmjBFu00g==");
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

TEST(TestSignature, singMD5WithSalt)
{
    RSAPrivateKey rsaPrivateKey;
    ASSERT_EQ(rsaPrivateKey.load("resource/private.pem"), true);
    RSAPublicKey rsaPublicKey;
    ASSERT_EQ(rsaPublicKey.load("resource/publick_key.pem"), true);
    std::string data = "User:Stanly, Calims:10001";
    auto token = rsaPrivateKey.getBase64Signature<HashAlgo::MD5>(data, "stanly");
    ASSERT_EQ(rsaPublicKey.verifyBase64Signature<HashAlgo::MD5>(data, token, "stanly"), true);
    ASSERT_EQ(rsaPublicKey.verifyBase64Signature<HashAlgo::MD5>(data, token, "stanly1"), false);
}

TEST(TestSignature, singSHAWithSalt)
{
    RSAPrivateKey rsaPrivateKey;
    ASSERT_EQ(rsaPrivateKey.load("resource/private.pem"), true);
    RSAPublicKey rsaPublicKey;
    ASSERT_EQ(rsaPublicKey.load("resource/publick_key.pem"), true);
    std::string data = "User:Stanly, Calims:10001";
    auto token = rsaPrivateKey.getBase64Signature<HashAlgo::SHA>(data, "stanly");
    ASSERT_EQ(rsaPublicKey.verifyBase64Signature<HashAlgo::SHA>(data, token, "stanly"), true);
    ASSERT_EQ(rsaPublicKey.verifyBase64Signature<HashAlgo::SHA>(data, token, "stanly1"), false);
}

TEST(TestSignature, sing224WithSalt)
{
    RSAPrivateKey rsaPrivateKey;
    ASSERT_EQ(rsaPrivateKey.load("resource/private.pem"), true);
    RSAPublicKey rsaPublicKey;
    ASSERT_EQ(rsaPublicKey.load("resource/publick_key.pem"), true);
    std::string data = "User:Stanly, Calims:10001";
    auto token = rsaPrivateKey.getBase64Signature<HashAlgo::SHA224>(data, "stanly");
    ASSERT_EQ(rsaPublicKey.verifyBase64Signature<HashAlgo::SHA224>(data, token, "stanly"), true);
    ASSERT_EQ(rsaPublicKey.verifyBase64Signature<HashAlgo::SHA224>(data, token, "stanly1"), false);
}

TEST(TestSignature, sing384WithSalt)
{
    RSAPrivateKey rsaPrivateKey;
    ASSERT_EQ(rsaPrivateKey.load("resource/private.pem"), true);
    RSAPublicKey rsaPublicKey;
    ASSERT_EQ(rsaPublicKey.load("resource/publick_key.pem"), true);
    std::string data = "User:Stanly, Calims:10001";
    auto token = rsaPrivateKey.getBase64Signature<HashAlgo::SHA384>(data, "stanly");
    ASSERT_EQ(rsaPublicKey.verifyBase64Signature<HashAlgo::SHA384>(data, token, "stanly"), true);
    ASSERT_EQ(rsaPublicKey.verifyBase64Signature<HashAlgo::SHA384>(data, token, "stanly1"), false);
}

TEST(TestSignature, sing512WithSalt)
{
    RSAPrivateKey rsaPrivateKey;
    ASSERT_EQ(rsaPrivateKey.load("resource/private.pem"), true);
    RSAPublicKey rsaPublicKey;
    ASSERT_EQ(rsaPublicKey.load("resource/publick_key.pem"), true);
    std::string data = "User:Stanly, Calims:10001";
    auto token = rsaPrivateKey.getBase64Signature<HashAlgo::SHA512>(data, "stanly");
    ASSERT_EQ(rsaPublicKey.verifyBase64Signature<HashAlgo::SHA512>(data, token, "stanly"), true);
    ASSERT_EQ(rsaPublicKey.verifyBase64Signature<HashAlgo::SHA512>(data, token, "stanly1"), false);
}

