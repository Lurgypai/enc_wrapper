#include <stdlib.h>
#include <stdio.h>

#include "enc_grain.h"
#include "enc_wrapper.h"

#define CHUNK_SIZE 4096

int test_enc_grain_write() {
    enc_grain_meta meta = {
        .size = CHUNK_SIZE,
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        }
    };
    char data_mem[CHUNK_SIZE] = {0};
    size_t data_store_size = enc_get_encrypted_size(meta.cfg, CHUNK_SIZE);
    void* data_store = malloc(data_store_size);
    size_t meta_store_size = enc_get_encrypted_size(meta.cfg, sizeof(enc_grain_meta));
    void* meta_store = malloc(meta_store_size);

    enc_load_config(meta.cfg);
    char* key = enc_make_key();
    enc_close();

    enc_grain_write(meta.cfg, meta, meta_store, data_mem, data_store, key);
    free(key);

    free(data_store);
    free(meta_store);

    return 0;
}

int test_enc_grain_data_write_and_read() {
    enc_grain_meta meta = {
        .size = CHUNK_SIZE,
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        }
    };
    char data_mem[CHUNK_SIZE] = {0};

    size_t data_store_size = enc_get_encrypted_size(meta.cfg, CHUNK_SIZE);
    void* data_store = malloc(data_store_size);

    enc_load_config(meta.cfg);
    char* key = enc_make_key();
    enc_close();

    enc_grain_data_write(meta, data_store, data_mem, key);

    char data_mem_2[CHUNK_SIZE] = {0};
    for(int i = 0; i != CHUNK_SIZE; ++i) {
        data_mem_2[i] = i % 256;
    }

    enc_grain_data_read(meta, data_store, data_mem_2, key);

    int ret = 0;
    for(int i = 0; i != CHUNK_SIZE; ++i) {
        if(data_mem[i] != data_mem_2[i]) {
            printf("data_mem[%d] doesn't match data_mem_2[%d], %d != %d\n", i, i, data_mem[i], data_mem_2[i]);
            ret = 1;
        }
    }
    free(data_store);
    free(key);
    return ret;
}

int test_enc_grain_meta_read() {
    enc_grain_meta meta = {
        .size = CHUNK_SIZE,
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        }
    };

    char data_mem[CHUNK_SIZE] = {0};
    size_t data_store_size = enc_get_encrypted_size(meta.cfg, CHUNK_SIZE);
    void* data_store = malloc(data_store_size);
    size_t meta_store_size = enc_get_encrypted_size(meta.cfg, sizeof(enc_grain_meta));
    void* meta_store = malloc(meta_store_size);

    enc_load_config(meta.cfg);
    char* key = enc_make_key();
    enc_close();

    enc_grain_write(meta.cfg, meta, meta_store, data_mem, data_store, key);

    free(data_store);

    enc_load_config(meta.cfg);
    enc_grain_meta meta2 = enc_grain_meta_read(meta_store, key);

    free(meta_store);
    free(key);

    printf("\tmeta.size: %lu, %lu\n", meta.size, meta2.size);
    printf("\tmeta.cfg.alg: %d, %d\n", meta.cfg.alg, meta2.cfg.alg);
    printf("\tmeta.cfg.lib: %d, %d\n", meta.cfg.lib, meta2.cfg.lib);

    return !(meta.size == meta2.size && meta.cfg.alg == meta2.cfg.alg && meta.cfg.lib == meta2.cfg.lib);
}

int test_enc_grain_data_read() {
    enc_grain_meta meta = {
        .size = CHUNK_SIZE,
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        }
    };

    char data_mem[CHUNK_SIZE] = {0};
    size_t data_store_size = enc_get_encrypted_size(meta.cfg, CHUNK_SIZE);
    void* data_store = malloc(data_store_size);
    size_t meta_store_size = enc_get_encrypted_size(meta.cfg, sizeof(enc_grain_meta));
    void* meta_store = malloc(meta_store_size);

    enc_load_config(meta.cfg);
    char* key = enc_make_key();
    enc_close();

    enc_grain_write(meta.cfg, meta, meta_store, data_mem, data_store, key);

    free(meta_store);

    for(int i = 0; i != CHUNK_SIZE; ++i) {
        data_mem[i] = i;
    }
    enc_grain_data_read(meta, data_store, data_mem, key);

    free(data_store);
    free(key);

    int is_zero = 1;
    for(int i = 0; i != CHUNK_SIZE; ++i) {
        if(data_mem[i] != 0) {
            printf("\tvalue at index %d was %d\n", i, data_mem[i]);
            is_zero = 0;
            break;
        }
    }

    enc_close();
    return !is_zero;
}

int main(int argc, char** argv) {

    printf("test_enc_grain_write: %d\n", test_enc_grain_write());
    // printf("test_enc_grain_meta_read: %d\n", test_enc_grain_meta_read());
    // printf("test_enc_grain_data_read: %d\n", test_enc_grain_data_read());
    // printf("test_enc_grain_data_write_and_read: %d\n", test_enc_grain_data_write_and_read());

    return 0;
}
