#include <crypto/Base64EncodeDecode.h>
#include <openssl/pem.h>

using namespace lu::crypto;


std::string Base64EncodeDecode::encode(unsigned char *msg, size_t len) const
{
    ::BIO *bio, *b64;
    ::BUF_MEM *bptr;

    b64 = ::BIO_new(BIO_f_base64());
    ::BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = ::BIO_new(BIO_s_mem());
    bio = ::BIO_push(b64, bio);

    ::BIO_write(bio, msg, len);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bptr);

    std::string result(bptr->data, bptr->length);

    ::BIO_free_all(bio);
    return result;
}

std::pair<unsigned char*, size_t> Base64EncodeDecode::decode(const std::string& b64message, size_t decodeLen) const
{
    auto buffer = (unsigned char *)malloc(decodeLen);
    memset(buffer, 0, decodeLen);

    auto bio = BIO_new_mem_buf(b64message.data(), b64message.length());
    auto b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    // BIO_set_flags is needed to make sure the BIO_f_base64 BIO doesn't add newlines
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    auto length = BIO_read(bio, buffer, decodeLen);
    BIO_free_all(bio);
    return {buffer, length};
}
