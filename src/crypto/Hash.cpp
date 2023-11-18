#include <crypto/Hash.h>

using namespace lu::crypto;

template <HashAlgo Algo>
bool Hash<Algo>::init()
{
    m_md = ::EVP_get_digestbyname(HashTraits<Algo>::ALGO_NAME);

    if (m_md == nullptr)
    {
        return false;
    }

    m_mdctx = EVP_MD_CTX_new();
    return true;
}

template <HashAlgo Algo>
std::string Hash<Algo>::getBase64Hash(const std::string &data)
{
    DataWrap dataWrap(HashTraits<Algo>::HASH_SIZE);
    ::EVP_DigestInit_ex(m_mdctx, m_md, NULL);
    ::EVP_DigestUpdate(m_mdctx, data.data(), data.length());
    unsigned int md_len;
    ::EVP_DigestFinal_ex(m_mdctx, dataWrap.getData(), &md_len);
    return base64EncodeDecode.encode(dataWrap);
}

template <HashAlgo Algo>
Hash<Algo>::~Hash()
{
    if (m_mdctx == nullptr)
    {
        return;
    }

    ::EVP_MD_CTX_free(m_mdctx);
}

template class Hash<HashAlgo::SHA>;
template class Hash<HashAlgo::SHA224>;
template class Hash<HashAlgo::SHA256>;
template class Hash<HashAlgo::SHA384>;
template class Hash<HashAlgo::SHA512>;
//template class Hash<HashAlgo::MD4>;
template class Hash<HashAlgo::MD5>;
