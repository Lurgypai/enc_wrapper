#include "enc_grain.h"

#include "enc_wrapper.h"

#include <stdlib.h>
#include <string.h>

static void read_and_set_nonce(void* buf, char** buf_out, size_t* size_out) {
    *size_out = enc_get_nonce_size();
    *buf_out = malloc(*size_out);
    memcpy(*buf_out, buf, *size_out);
    enc_set_nonce(*buf_out, *size_out);
}

enc_grain_meta enc_grain_meta_read(void* meta_store, char* key) {
    enc_grain_meta mem;

    enc_set_key(key, enc_get_key_size());

    char* nonce;
    size_t nonce_size;
    read_and_set_nonce(meta_store, &nonce, &nonce_size);

    enc_decrypt(meta_store + nonce_size, sizeof(mem), &mem, sizeof(mem));

    free(nonce);

    return mem;
}

void enc_grain_data_read(enc_grain_meta meta, void* data_store, void* data_mem, char* key) {
    // load config
    enc_load_config(meta.cfg);

    enc_set_key(key, enc_get_key_size());

    char* nonce;
    size_t nonce_size;
    read_and_set_nonce(data_store, &nonce, &nonce_size);

    enc_decrypt(data_store + nonce_size, meta.size, data_mem, meta.size);

    free(nonce);
}

void enc_grain_meta_write(enc_config meta_cfg, enc_grain_meta meta, void* meta_store, char* key) {
    enc_load_config(meta_cfg);

    enc_set_key(key, enc_get_key_size());
    char* nonce = enc_make_nonce();
    size_t nonce_size = enc_get_nonce_size();
    memcpy(meta_store, nonce, nonce_size);

    enc_set_nonce(nonce, nonce_size);
    enc_encrypt(&meta, sizeof(meta), meta_store + nonce_size, sizeof(meta));

    free(nonce);

    enc_close();
}

void enc_grain_data_write(enc_grain_meta meta, void* data_store, void* data_mem, char* key) {
    enc_load_config(meta.cfg);

    enc_set_key(key, enc_get_key_size());
    char* nonce = enc_make_nonce();
    size_t nonce_size = enc_get_nonce_size();
    memcpy(data_store, nonce, nonce_size);
    enc_set_nonce(nonce, nonce_size);
    enc_encrypt(data_mem, meta.size, data_store + nonce_size, meta.size);
    free(nonce);

    enc_close();
}
