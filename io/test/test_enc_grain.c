#include <stdlib.h>
#include <stdio.h>

#include "enc_grain.h"
#include "enc_wrapper.h"

int test_enc_grain_write() {
    enc_grain_meta meta = {
        .size = 4096,
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        }
    };

    char data_mem[4096] = {0};
    char data_store[4096] = {0};
    enc_grain_meta meta_store;

    enc_load_config(meta.cfg);
    char* key = enc_make_key();
    enc_grain_write(meta, &meta_store, data_mem, data_store, key);
    free(key);

    return 0;
}

int test_enc_grain_meta_read() {
    enc_grain_meta meta = {
        .size = 4096,
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        }
    };

    char data_mem[4096] = {0};
    char data_store[4096] = {0};
    enc_grain_meta meta_store;

    enc_load_config(meta.cfg);
    char* key = enc_make_key();
    enc_grain_write(meta, &meta_store, data_mem, data_store, key);
    free(key);

    enc_load_config(meta.cfg);
    enc_grain_meta meta2 = enc_grain_meta_read(&meta_store, key);
    return !(meta.size == meta2.size && meta.cfg.alg == meta2.cfg.alg && meta.cfg.lib == meta2.cfg.lib);
}

int test_enc_grain_data_read() {
    enc_grain_meta meta = {
        .size = 4096,
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        }
    };

    char data_mem[4096] = {0};
    char data_store[4096] = {0};
    enc_grain_meta meta_store;

    enc_load_config(meta.cfg);
    char* key = enc_make_key();
    enc_grain_write(meta, &meta_store, data_mem, data_store, key);
    free(key);

    for(int i = 0; i != 4096; ++i) {
        data_mem[i] = i;
    }
    enc_grain_data_read(meta, data_store, data_mem, key);

    int is_zero = 1;
    for(int i = 0; i != 4096; ++i) {
        if(data_mem[i] != 0) {
            is_zero = 0;
            break;
        }
    }
    return !is_zero;
}

int main(int argc, char** argv) {

    printf("test_enc_grain_write: %d\n", test_enc_grain_write());
    printf("test_enc_grain_meta_read: %d\n", test_enc_grain_meta_read());
    printf("test_enc_grain_data_read: %d\n", test_enc_grain_data_read());

    return 0;
}
