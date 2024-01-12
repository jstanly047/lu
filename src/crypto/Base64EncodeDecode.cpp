#include <crypto/Base64EncodeDecode.h>

using namespace lu::crypto;

namespace
{
    std::size_t calcDecodeLength(const std::string& b64message)
    {
        std::size_t len = b64message.size();
        std::size_t padding = 0;

        if (b64message[len - 1] == '=' && b64message[len - 2] == '=')
        {
            padding = 2;
        }
        else if (b64message[len - 1] == '=')
        {
            padding = 1;
        }

        return (len * 3) / 4 - padding;
    }

    ::BIO *b64 = ::BIO_new(BIO_f_base64());
}

::BIO *Base64EncodeDecode::bio = ::BIO_new(BIO_s_mem());

::BIO* Base64EncodeDecode::b64Encoder()
{
    static ::BIO *b64_instance = []
    {
        ::BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        return ::BIO_push(b64, bio);
    }();
    return b64_instance;
}

std::string Base64EncodeDecode::encode(DataWrap& data) 
{
    ::BUF_MEM *bptr = nullptr;
    //auto *b64 = ::BIO_new(BIO_f_base64());
    //::BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    //auto *bio = ::BIO_new(BIO_s_mem());
    //bio = ::BIO_push(b64, bio);
    BIO_reset(bio);
    ::BIO_write(b64Encoder(), data.getData(), static_cast<int>(data.getCapacity()));
    BIO_flush(b64Encoder());
    BIO_get_mem_ptr(b64Encoder(), &bptr);
    std::string result(bptr->data, bptr->length);
    //::BIO_free_all(bio);
    return result;
}

void Base64EncodeDecode::encode(void *ptr, int size, void *dest, std::size_t maxLength)
{
    ::BUF_MEM *bptr = nullptr;
    BIO_reset(bio);
    ::BIO_write(b64Encoder(), ptr, size);
    BIO_flush(b64Encoder());
    BIO_get_mem_ptr(b64Encoder(), &bptr);
    auto len = maxLength < bptr->length ? maxLength : bptr->length;
    std::memcpy(dest, bptr->data, len);
}

DataWrap Base64EncodeDecode::decode(const std::string& b64message) 
{
    auto decodeLen = calcDecodeLength(b64message);
    DataWrap dataWrap(decodeLen);
    auto *bio = BIO_new_mem_buf(b64message.data(),  -1);
    auto *b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    // BIO_set_flags is needed to make sure the BIO_f_base64 BIO doesn't add newlines
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    BIO_read(bio, dataWrap.getData(),static_cast<int>(b64message.length() + 1));
    BIO_free_all(bio);
    return dataWrap;
}
