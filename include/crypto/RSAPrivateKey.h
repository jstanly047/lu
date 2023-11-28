#pragma once
#include <string>
#include <crypto/HashTraits.h>

namespace lu::crypto
{
    class Base64EncodeDecode;

    class RSAPrivateKey
    {
    public:
        template<HashAlgo Algo=HashAlgo::SHA256>
        std::string getBase64Signature(const std::string &data, const std::string& salt="");
        bool load(const std::string& filePath);
        ~RSAPrivateKey();

    private:
        ::EVP_PKEY * m_privateKey{};
    };
}