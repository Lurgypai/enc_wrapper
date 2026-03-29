#include "enc_gcrypt.h"

#include "gcrypt.h"

#include <stdio.h>

static gcry_cipher_hd_t handle;

static void print_gcrypt_err(gcry_error_t err) {
    if(!err) return;
    fprintf(stderr, "ERROR FROM GCRYPT: %s\n", gcry_strerror(err));
}

static int gcrypt_prepare(enc_algorithm alg, size_t* key_len, size_t* nonce_len, size_t* blk_len) {
    int cipher;
    int mode;

    switch(alg) {
        case aes256:
            cipher = GCRY_CIPHER_AES256;
            mode = GCRY_CIPHER_MODE_CBC;
            *nonce_len = gcry_cipher_get_algo_blklen(cipher);
            break;
        case chacha20:
            cipher = GCRY_CIPHER_CHACHA20;
            mode = GCRY_CIPHER_MODE_STREAM;
            *nonce_len = 12;
            break;
        default:
            break;
    }

    *blk_len = gcry_cipher_get_algo_blklen(cipher);
    print_gcrypt_err(gcry_cipher_open(&handle, cipher, mode, 0));
    *key_len = gcry_cipher_get_algo_keylen(cipher);

    return 0;
}

static int gcrypt_set_key(char* key, size_t key_len) {
    print_gcrypt_err(gcry_cipher_setkey(handle, key, key_len));
    return 0;
}

static int gcrypt_set_nonce(char* nonce, size_t nonce_len) {
    gcry_cipher_setiv(handle, nonce, nonce_len);
    return 0;
}

static int gcrypt_encrypt(void* source, size_t source_len, void* dest, size_t dest_len, size_t blk_len) {
    if(source_len % blk_len != 0 || dest_len % blk_len != 0) {
        fprintf(stderr, "ERROR: encrypt, source or destination sizes are not divisible by block size\n");
    }

    print_gcrypt_err(gcry_cipher_encrypt(handle, dest, dest_len, source, source_len));
    return 0;
}

static int gcrypt_decrypt(void* source, size_t source_len, void* dest, size_t dest_len, size_t blk_len) {
    if(source_len % blk_len != 0 || dest_len % blk_len != 0) {
        fprintf(stderr, "ERROR: decrypt, source or destination sizes are not divisible by block size\n");
    }

    gcry_cipher_decrypt(handle, dest, dest_len, source, source_len);
    return 0;
}

static int gcrypt_reset() {
    gcry_cipher_reset(handle);
    return 0;
}

static int gcrypt_close() {
    gcry_cipher_close(handle);
    return 0;
}

static char* make_random(size_t size) {
    char* data = malloc(size);
    gcry_randomize(data, size, GCRY_VERY_STRONG_RANDOM);
    return data;
}

char* gcrypt_make_key(size_t key_size) {
    return make_random(key_size);
}

char* gcrypt_make_nonce(size_t nonce_size) {
    return make_random(nonce_size);
}

static size_t gcrypt_get_encrypted_size(enc_algorithm alg, size_t raw_size) {
    // calculate padding
    int cipher;
    int nonce_len;
    switch(alg) {
        case aes256:
            cipher = GCRY_CIPHER_AES256;
            nonce_len = gcry_cipher_get_algo_blklen(cipher);
            break;
        case chacha20:
            cipher = GCRY_CIPHER_CHACHA20;
            nonce_len = 12;
            break;
        default:
            break;
    }
    unsigned int block_size = gcry_cipher_get_algo_blklen(cipher);
    size_t padding = 0;
    size_t remain = raw_size % block_size;
    if(remain != 0) padding = block_size - remain;
    return raw_size + nonce_len + padding;
}

enc_library_impl enc_get_gcrypt() {
    enc_library_impl ret = {
        .prepare = gcrypt_prepare,
        .set_key = gcrypt_set_key,
        .set_nonce = gcrypt_set_nonce,
        .encrypt = gcrypt_encrypt,
        .decrypt = gcrypt_decrypt,
        .reset = gcrypt_reset,
        .close = gcrypt_close,
        .make_key = gcrypt_make_key,
        .make_nonce = gcrypt_make_nonce,
        .key_size = 0,
        .nonce_size = 0,
        .block_size = 0,
        .get_encrypted_size = gcrypt_get_encrypted_size
    };
    return ret;
}
