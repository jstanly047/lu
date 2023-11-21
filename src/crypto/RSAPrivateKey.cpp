#include <crypto/RSAPrivateKey.h>
#include <crypto/Base64EncodeDecode.h>
#include <crypto/RSAKeyTraits.h>

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <cassert>

#include <glog/logging.h>

using namespace lu::crypto;

template<std::size_t HashSize>
std::string RSAPrivateKey::getBase64Signature(const std::string &data, const std::string& salt)
{  
    assert(m_privateKey != nullptr);
    const std::string dataWithSalt = salt + data;
    ::EVP_MD_CTX* rsaSignCtx = ::EVP_MD_CTX_create();

    if (::EVP_DigestSignInit(rsaSignCtx, NULL, RSAKeyTraits<HashSize>::HASH_FUNCTION()(), NULL, m_privateKey)<=0) 
    {
        //LOG(ERROR) << "Signing failed in EVP_DigestSignInit!";
        ::EVP_MD_CTX_destroy(rsaSignCtx);
        return std::string();
    }

    if (::EVP_DigestSignUpdate(rsaSignCtx, dataWithSalt.data(), dataWithSalt.length()) <= 0) 
    {
        //LOG(ERROR) << "Signing failed in EVP_DigestSignUpdate!";
        ::EVP_MD_CTX_destroy(rsaSignCtx);
        return std::string();
    }

    std::size_t encMessageLength = HashSize;
    DataWrap dataWrap(HashSize);

    if (::EVP_DigestSignFinal(rsaSignCtx, dataWrap.getData(), &encMessageLength) <= 0) 
    {
        //LOG(ERROR) << "Signing failed in EVP_DigestSignFinal!";
        ::EVP_MD_CTX_destroy(rsaSignCtx);
        return std::string();
    }

    ::EVP_MD_CTX_destroy(rsaSignCtx);
    return Base64EncodeDecode::encode(dataWrap);
}

bool RSAPrivateKey::load(const std::string& filePath)
{
    ::BIO *keyFile = ::BIO_new_file(filePath.c_str(), "r");

    if (keyFile == nullptr) 
    {
        //LOG(ERROR) << "Can not open RSA Private key file " << filePath.c_str();
        return false;
    }

    m_privateKey = ::PEM_read_bio_PrivateKey(keyFile, nullptr, nullptr, nullptr);
    ::BIO_free(keyFile);
    
    return m_privateKey != nullptr;
}

RSAPrivateKey::~RSAPrivateKey()
{
    if (m_privateKey == nullptr)
    {
        return;
    }

    ::EVP_PKEY_free(m_privateKey);
}


template std::string lu::crypto::RSAPrivateKey::getBase64Signature<256>(const std::string &data, const std::string& salt);
template std::string lu::crypto::RSAPrivateKey::getBase64Signature<384>(const std::string &data, const std::string& salt);
template std::string lu::crypto::RSAPrivateKey::getBase64Signature<512>(const std::string &data, const std::string& salte);