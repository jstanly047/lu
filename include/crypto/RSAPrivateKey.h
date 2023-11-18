#pragma once
#include <string>

#include <openssl/types.h>


namespace lu::crypto
{
    class Base64EncodeDecode;

    class RSAPrivateKey
    {
    public:
        template<std::size_t HashSize=256>
        std::string getBase64Signature(const std::string &data, const std::string& salt="");
        bool load(const std::string& filePath);
        ~RSAPrivateKey();

    private:
        ::EVP_PKEY * m_privateKey{};
    };
}