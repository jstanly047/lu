#include <crypto/Base64EncodeDecode.h>
#include <openssl/pem.h>

using namespace lu::crypto;


std::string Base64EncodeDecode::encode(DataWrap& data) 
{
    ::BUF_MEM *bptr = nullptr;
    auto *b64 = ::BIO_new(BIO_f_base64());
    ::BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    auto *bio = ::BIO_new(BIO_s_mem());
    bio = ::BIO_push(b64, bio);

    ::BIO_write(bio, data.getData(), data.getLength());
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bptr);
    std::string result(bptr->data, bptr->length);
    ::BIO_free_all(bio);
    return result;
}

DataWrap Base64EncodeDecode::decode(const std::string& b64message, int decodeLen) 
{
    DataWrap dataWrap(decodeLen);
    auto *bio = BIO_new_mem_buf(b64message.data(),  static_cast<int>(b64message.length()));
    auto *b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    // BIO_set_flags is needed to make sure the BIO_f_base64 BIO doesn't add newlines
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    BIO_read(bio, dataWrap.getData(), decodeLen);
    BIO_free_all(bio);
    return dataWrap;
}
