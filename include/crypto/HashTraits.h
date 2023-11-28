#pragma once
#include <concepts>
#include <string>
#include <openssl/sha.h>
#include <openssl/md4.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/types.h>



namespace lu::crypto
{
    enum struct HashAlgo
    {
        SHA,
        SHA224,
        SHA256,
        SHA384,
        SHA512,
        MD4,
        MD5
    };

    using HashFunctionPtr = const EVP_MD* (*)();

    template <HashAlgo Algo>
    struct HashTraits;

    template <>
    struct HashTraits<HashAlgo::SHA>
    {
        static constexpr const char* ALGO_NAME = "SHA1";
        static constexpr int HASH_SIZE = SHA_DIGEST_LENGTH;

        static HashFunctionPtr HASH_FUNCTION()
        {
            return EVP_sha1;
        }
    };

    template <>
    struct HashTraits<HashAlgo::SHA224>
    {
        static constexpr const char* ALGO_NAME = "SHA224";
        static constexpr int HASH_SIZE = SHA224_DIGEST_LENGTH;

        static HashFunctionPtr HASH_FUNCTION()
        {
            return EVP_sha256;
        }
    };

    template <>
    struct HashTraits<HashAlgo::SHA256>
    {
        static constexpr const char* ALGO_NAME = "SHA256";
        static constexpr int HASH_SIZE = SHA256_DIGEST_LENGTH;

        static HashFunctionPtr HASH_FUNCTION()
        {
            return EVP_sha256;
        }
    };

    template <>
    struct HashTraits<HashAlgo::SHA384>
    {
        static constexpr const char* ALGO_NAME = "SHA384";
        static constexpr int HASH_SIZE = SHA384_DIGEST_LENGTH;

        static HashFunctionPtr HASH_FUNCTION()
        {
            return EVP_sha384;
        }
    };

    template <>
    struct HashTraits<HashAlgo::SHA512>
    {
        static constexpr const char* ALGO_NAME = "SHA512";
        static constexpr int HASH_SIZE = SHA512_DIGEST_LENGTH;

        static HashFunctionPtr HASH_FUNCTION()
        {
            return EVP_sha512;
        }
    };

    /*template <>
    struct HashTraits<HashAlgo::MD4>
    {
        static constexpr const char* ALGO_NAME = "MD4";
        static constexpr int HASH_SIZE = MD4_DIGEST_LENGTH;
    };*/

    template <>
    struct HashTraits<HashAlgo::MD5>
    {
        static constexpr const char* ALGO_NAME = "MD5";
        static constexpr int HASH_SIZE = MD5_DIGEST_LENGTH;

        static HashFunctionPtr HASH_FUNCTION()
        {
            return EVP_md5;
        }
    };
}