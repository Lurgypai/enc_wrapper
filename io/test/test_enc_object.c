#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "enc_object.h"
#include "enc_wrapper.h"

#define CHUNK_SIZE 4096

int test_enc_object_make() {
    const char* obj_tag = "test_object";
    enc_object obj = enc_object_make(obj_tag);
    int ret =  !(strcmp(obj_tag, obj.tag) == 0) && obj.grain_cnt == 0 && obj.grain_reserve == 0 && obj.tag_size == 11 && obj.grains == NULL;
    enc_object_free(obj);

    return ret;
}

int test_enc_object_free() {
    const char* obj_tag = "test_object";
    enc_object obj = enc_object_make(obj_tag);
    enc_object_free(obj);
    return 0;
}

int test_enc_object_add_grain() {
    const char* obj_tag = "test_object";
    enc_object obj = enc_object_make(obj_tag);

    enc_grain_meta grain = {
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        },
        .size = CHUNK_SIZE
    };
    size_t pos = enc_object_add_grain(&obj, grain);

    int ret = !(pos == 0 && obj.grains[pos].cfg.alg == grain.cfg.alg && obj.grains[pos].cfg.lib == grain.cfg.lib && obj.grains[pos].size == grain.size);

    enc_object_free(obj);

    return ret;
}

int test_enc_object_grain_write() {
    const char* obj_tag = "test_object";
    enc_object obj = enc_object_make(obj_tag);

    enc_grain_meta grain = {
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        },
        .size = CHUNK_SIZE
    };
    size_t pos = enc_object_add_grain(&obj, grain);

    enc_load_config(grain.cfg);
    char* key = enc_make_key();
    char data_mem[CHUNK_SIZE] = {0};
    void* meta_store = malloc(enc_get_encrypted_size(grain.cfg, sizeof(enc_grain_meta)));
    void* data_store = malloc(enc_get_encrypted_size(grain.cfg, CHUNK_SIZE));

    enc_object_grain_write(obj, pos, grain.cfg, key, meta_store, &data_mem, data_store);

    free(key);
    free(meta_store);
    free(data_store);

    enc_object_free(obj);

    return 0;
}

int test_enc_object_grain_meta_read() {
    const char* obj_tag = "test_object";
    enc_object obj = enc_object_make(obj_tag);

    enc_grain_meta grain = {
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        },
        .size = CHUNK_SIZE
    };
    size_t pos = enc_object_add_grain(&obj, grain);

    enc_load_config(grain.cfg);
    char* key = enc_make_key();
    char data_mem[CHUNK_SIZE] = {0};
    void* meta_store = malloc(enc_get_encrypted_size(grain.cfg, sizeof(enc_grain_meta)));
    void* data_store = malloc(enc_get_encrypted_size(grain.cfg, CHUNK_SIZE));

    enc_object_grain_write(obj, pos, grain.cfg, key, meta_store, &data_mem, data_store);

    // reset the grain so we can replace the metadata
    obj.grains[pos].cfg.alg = -1;
    obj.grains[pos].cfg.lib = -1;
    obj.grains[pos].size = 0;

    enc_object_grain_meta_read(obj, pos, grain.cfg, key, meta_store);

    int ret = !(obj.grains[pos].cfg.alg == grain.cfg.alg && obj.grains[pos].cfg.lib == grain.cfg.lib && obj.grains[pos].size == grain.size);

    free(key);
    free(meta_store);
    free(data_store);

    enc_object_free(obj);

    return ret;
}

int test_enc_object_grain_data_read() {
    const char* obj_tag = "test_object";
    enc_object obj = enc_object_make(obj_tag);

    enc_grain_meta grain = {
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        },
        .size = CHUNK_SIZE
    };
    size_t pos = enc_object_add_grain(&obj, grain);

    enc_load_config(grain.cfg);
    char* key = enc_make_key();
    char data_mem[CHUNK_SIZE] = {0};
    void* meta_store = malloc(enc_get_encrypted_size(grain.cfg, sizeof(enc_grain_meta)));
    void* data_store = malloc(enc_get_encrypted_size(grain.cfg, CHUNK_SIZE));

    enc_object_grain_write(obj, pos, grain.cfg, key, meta_store, &data_mem, data_store);

    for(int i = 0; i != CHUNK_SIZE; ++i) {
        data_mem[i] = i;
    }

    enc_object_grain_data_read(obj, pos, key, data_mem, data_store);

    int is_zero = 1;
    for(int i = 0; i != CHUNK_SIZE; ++i) {
        if(data_mem[i] != 0) {
            is_zero = 0;
            break;
        }
    }

    free(key);
    free(meta_store);
    free(data_store);

    enc_object_free(obj);

    return !is_zero;
}

int test_enc_object_get_meta_size() {
    const char* obj_tag = "test_object";
    enc_object obj = enc_object_make(obj_tag);

    enc_grain_meta grain = {
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        },
        .size = CHUNK_SIZE
    };
    size_t pos = enc_object_add_grain(&obj, grain);

    int ret = !(enc_object_get_meta_size(obj) == sizeof(size_t) + 11 + sizeof(size_t));

    enc_object_free(obj);

    return ret;
}

int test_enc_object_put_meta() {

    const char* obj_tag = "test_object";
    enc_object obj = enc_object_make(obj_tag);

    enc_grain_meta grain = {
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        },
        .size = CHUNK_SIZE
    };
    size_t pos = enc_object_add_grain(&obj, grain);

    void* obj_meta = malloc(enc_object_get_meta_size(obj));

    enc_object_put_meta(obj, obj_meta);

    free(obj_meta);
    return 0;
}

int test_enc_object_parse_meta() {
    const char* obj_tag = "test_object";
    enc_object obj = enc_object_make(obj_tag);

    enc_grain_meta grain = {
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        },
        .size = CHUNK_SIZE
    };
    size_t pos = enc_object_add_grain(&obj, grain);

    void* obj_meta = malloc(enc_object_get_meta_size(obj));

    enc_object_put_meta(obj, obj_meta);

    enc_object obj2;

    enc_object_parse_meta(&obj2, obj_meta);

    int ret = !(obj2.grain_cnt == obj.grain_cnt && obj2.tag_size == obj.tag_size && strcmp(obj2.tag, obj.tag) == 0 &&
            obj2.grains[0].size == obj.grains[0].size &&
            obj2.grains[0].cfg.alg == obj.grains[0].cfg.alg &&
            obj2.grains[0].cfg.lib == obj.grains[0].cfg.lib);

    free(obj_meta);
    return 0;
}

int main(int argc, char** argv) {
    printf("test_enc_object_make: %d\n", test_enc_object_make());
    printf("test_enc_object_free: %d\n", test_enc_object_free());
    printf("test_enc_object_add_grain: %d\n", test_enc_object_add_grain());
    printf("test_enc_object_grain_write: %d\n", test_enc_object_grain_write());
    printf("test_enc_object_grain_meta_read: %d\n", test_enc_object_grain_meta_read());
    printf("test_enc_object_grain_data_read: %d\n", test_enc_object_grain_data_read());
    printf("test_enc_object_get_meta_size: %d\n", test_enc_object_get_meta_size());
    printf("test_enc_object_put_meta: %d\n", test_enc_object_put_meta());
    printf("test_enc_object_parse_meta: %d\n", test_enc_object_parse_meta());

    return 0;
}
