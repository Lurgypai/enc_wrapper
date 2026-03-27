#include "enc_dummy.h"

#include <stdlib.h>
#include <string.h>

static int dummy_encrypt(void* source, size_t source_len, void* dest, size_t dest_len, size_t blk_len) {
    memcpy(dest, source, source_len);
    return 0;
}

static int dummy_decrypt(void* source, size_t source_len, void* dest, size_t dest_len, size_t blk_len) {
    memcpy(dest, source, source_len);
    return 0;
}

static char* dummy_make_key() {
    return malloc(1);
}

static char* dummy_make_nonce() {
    return malloc(1);
}

static size_t dummy_get_encrypted_size(enc_algorithm alg, size_t raw_size) {
    return raw_size;
}

enc_library_impl enc_get_dummy() {
    enc_library_impl ret = {
        .prepare = NULL,
        .set_key = NULL,
        .set_nonce = NULL,
        .encrypt = dummy_encrypt,
        .decrypt = dummy_decrypt,
        .reset = NULL,
        .make_key = NULL,
        .make_nonce = NULL,
        .key_size = 1,
        .nonce_size = 1,
        .block_size = 0,
        .get_encrypted_size = dummy_get_encrypted_size
    };
    return ret;
}
