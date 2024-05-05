#include <crypto/RSAPublicKey.h>
#include <crypto/Base64EncodeDecode.h>

#include <cassert>

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>

using namespace lu::crypto;

template<HashAlgo Algo>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
bool RSAPublicKey::verifyBase64Signature(const std::string& data, const std::string &signature, const std::string& salt) const
{
    assert(m_publicKey != nullptr);
    auto decodedSignature = Base64EncodeDecode::decode(signature);
    const std::string dataWithSalt = salt + data;
    ::EVP_MD_CTX* rsaVerifyCtx = ::EVP_MD_CTX_create();

    if (::EVP_DigestVerifyInit(rsaVerifyCtx, NULL, HashTraits<Algo>::HASH_FUNCTION()(), NULL, m_publicKey) <= 0) 
    {
        ::EVP_MD_CTX_destroy(rsaVerifyCtx);
        return false;
    }

    if (::EVP_DigestVerifyUpdate(rsaVerifyCtx, dataWithSalt.data(), dataWithSalt.length()) <= 0) 
    {
        ::EVP_MD_CTX_destroy(rsaVerifyCtx);
        return false;
    }

    int AuthStatus = ::EVP_DigestVerifyFinal(rsaVerifyCtx, decodedSignature.getData(), decodedSignature.getCapacity());
    ::EVP_MD_CTX_destroy(rsaVerifyCtx);
    return AuthStatus == 1;
}

bool RSAPublicKey::load(const std::string& filePath)
{
    BIO *keyFile = ::BIO_new_file(filePath.c_str(), "r");
    if (keyFile == nullptr) 
    {
        return false;
    }

    m_publicKey = ::PEM_read_bio_PUBKEY(keyFile, nullptr, nullptr, nullptr);
    ::BIO_free(keyFile);

    return  m_publicKey != nullptr;
}

bool RSAPublicKey::read(const std::string& publickey)
{
    BIO* keyBio = ::BIO_new_mem_buf((void*)publickey.data(), -1);
    if (keyBio == nullptr) 
    {
        return false;
    }

    m_publicKey = ::PEM_read_bio_PUBKEY(keyBio, nullptr, nullptr, nullptr);
    ::BIO_free(keyBio);
    return  m_publicKey != nullptr;
}

RSAPublicKey::~RSAPublicKey()
{
    if (m_publicKey == nullptr)
    {
        return;
    }

    ::EVP_PKEY_free(m_publicKey);
}

template bool RSAPublicKey::verifyBase64Signature<HashAlgo::MD5>(const std::string& data, const std::string &signature, const std::string& salt) const;
template bool RSAPublicKey::verifyBase64Signature<HashAlgo::SHA>(const std::string& data, const std::string &signature, const std::string& salt) const;
template bool RSAPublicKey::verifyBase64Signature<HashAlgo::SHA224>(const std::string& data, const std::string &signature, const std::string& salt) const;
template bool RSAPublicKey::verifyBase64Signature<HashAlgo::SHA256>(const std::string& data, const std::string &signature, const std::string& salt) const;
template bool RSAPublicKey::verifyBase64Signature<HashAlgo::SHA384>(const std::string& data, const std::string &signature, const std::string& salt) const;
template bool RSAPublicKey::verifyBase64Signature<HashAlgo::SHA512>(const std::string& data, const std::string &signature, const std::string& salt) const;