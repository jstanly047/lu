#pragma once
#include <string>
#include <crypto/HashTraits.h>

namespace lu::crypto
{
    class Base64EncodeDecode;

    class RSAPublicKey
    {
    public:
        template<HashAlgo Algo=HashAlgo::SHA256>
        bool verifyBase64Signature(const std::string& data, const std::string &signature, const std::string& salt="") const;
        bool load(const std::string& filePath);
        ~RSAPublicKey();

    private:
        ::EVP_PKEY* m_publicKey{};
    };
}