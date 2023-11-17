#include <string>
#include <openssl/types.h>

namespace lu::crypto
{
    class Base64EncodeDecode;

    class RSAPublicKey
    {
    public:
        template<std::size_t HashSize=256>
        bool verify(const std::string& data, const std::string &signature, const std::string& salt="") const;
        bool load(const std::string& filePath);
        ~RSAPublicKey();

    private:
        ::EVP_PKEY* m_publicKey{};
    };
}