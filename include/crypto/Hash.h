#pragma once
#include <crypto/HashTraits.h>
#include <crypto/Base64EncodeDecode.h>
#include <openssl/types.h>
#include <openssl/evp.h>

namespace lu::crypto
{
    template <HashAlgo Algo>
    class Hash
    {
    public:
        bool init();
        std::string getBase64Hash(const std::string& data);
        DataWrap getHash(const void*, std::size_t);
        ~Hash();

    private:
        ::EVP_MD_CTX *m_mdctx{};
        const ::EVP_MD *m_md{};
        Base64EncodeDecode base64EncodeDecode;
    };
}