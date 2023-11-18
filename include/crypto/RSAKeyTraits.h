#pragma once
#include <openssl/evp.h>
#include <openssl/types.h>



namespace lu::crypto
{
    template <std::size_t HashSize>
    struct RSAKeyTraits;

    using HashFunctionPtr = const EVP_MD* (*)();

    template <>
    struct RSAKeyTraits<256>
    {
        static HashFunctionPtr HASH_FUNCTION()
        {
            return EVP_sha256;
        }
    };

    template <>
    struct RSAKeyTraits<384>
    {
        static HashFunctionPtr HASH_FUNCTION()
        {
            return EVP_sha384;
        }
    };

    template <>
    struct RSAKeyTraits<512>
    {
        static HashFunctionPtr HASH_FUNCTION()
        {
            return EVP_sha512;
        }
    };
}