#include <crypto/RSAPublicKey.h>
#include <crypto/Base64EncodeDecode.h>
#include <crypto/RSAKeyTraits.h>

#include <cassert>

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <glog/logging.h>

using namespace lu::crypto;

template<std::size_t HashSize>
bool RSAPublicKey::verifyBase64Signature(const std::string& data, const std::string &signature, const std::string& salt) const
{
    assert(m_publicKey != nullptr);
    Base64EncodeDecode encodeDecode;

    auto decodedSignature = encodeDecode.decode(signature, HashSize);
    const std::string dataWithSalt = salt + data;

    ::EVP_MD_CTX* rsaVerifyCtx = ::EVP_MD_CTX_create();

    if (::EVP_DigestVerifyInit(rsaVerifyCtx, NULL, RSAKeyTraits<HashSize>::HASH_FUNCTION()(), NULL, m_publicKey) <= 0) 
    {
        //LOG(ERROR) << "Singature verificatio faild at EVP_DigestVerifyInit [" << signature << "]";
        ::EVP_MD_CTX_destroy(rsaVerifyCtx);
        return false;
    }

    if (::EVP_DigestVerifyUpdate(rsaVerifyCtx, dataWithSalt.data(), dataWithSalt.length()) <= 0) 
    {
        //LOG(ERROR) << "Singature verificatio faild at EVP_DigestVerifyUpdate [" << signature << "]";
        ::EVP_MD_CTX_destroy(rsaVerifyCtx);
        return false;
    }

    int AuthStatus = ::EVP_DigestVerifyFinal(rsaVerifyCtx, decodedSignature.getData(), decodedSignature.getLength());
    ::EVP_MD_CTX_destroy(rsaVerifyCtx);
    return AuthStatus == 1;
}

bool RSAPublicKey::load(const std::string& filePath)
{
    BIO *keyFile = ::BIO_new_file(filePath.c_str(), "r");
    if (keyFile == nullptr) 
    {
        //LOG(ERROR) << "Can not open RSA Public key file " << filePath;
        return false;
    }

    m_publicKey = ::PEM_read_bio_PUBKEY(keyFile, nullptr, nullptr, nullptr);
    ::BIO_free(keyFile);

    if (!m_publicKey)
    {
        //LOG(ERROR) << "Can not read RSA Public key " << filePath;
        return false;
    }

    //LOG(INFO) << "Loaded Public key " << filePath;
    return true;
}

RSAPublicKey::~RSAPublicKey()
{
    if (m_publicKey == nullptr)
    {
        return;
    }

    ::EVP_PKEY_free(m_publicKey);
}

template bool RSAPublicKey::verifyBase64Signature<256>(const std::string& data, const std::string &signature, const std::string& salt) const;
template bool RSAPublicKey::verifyBase64Signature<384>(const std::string& data, const std::string &signature, const std::string& salt) const;
template bool RSAPublicKey::verifyBase64Signature<512>(const std::string& data, const std::string &signature, const std::string& salt) const;