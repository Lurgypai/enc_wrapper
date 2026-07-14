#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "enc_object.h"
#include "enc_wrapper.h"

#define CHUNK_SIZE 4096

int test_enc_object_make() {
    const char* obj_tag = "test_object";
    enc_object obj = enc_object_make(obj_tag);
    int ret =  !(strcmp(obj_tag, obj.tag) == 0) && obj.grain_cnt == 0 && obj.tag_size == 11;
    enc_object_free(&obj);

    return ret;
}

int test_enc_object_free() {
    const char* obj_tag = "test_object";
    enc_object obj = enc_object_make(obj_tag);
    enc_object_free(&obj);
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

    enc_grain_index_desc desc = obj.idx.grains[0];

    int ret = !(obj.grain_cnt == 1 && obj.idx.cnt == 1 && obj.idx.reserved == 1 && desc.id == 0 && desc.offset == 0 && desc.size == CHUNK_SIZE);

    enc_object_free(&obj);

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
    void* data_store = malloc(enc_get_encrypted_size(grain.cfg, CHUNK_SIZE));

    enc_object_grain_write(obj, grain, data_mem, data_store, key);

    free(key);
    free(data_store);

    enc_object_free(&obj);

    return 0;
}

int test_enc_object_grain_read() {
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

    enc_object_grain_write(obj, grain, data_mem, data_store, key);

    for(int i = 0; i != CHUNK_SIZE; ++i) {
        data_mem[i] = i;
    }

    enc_object_grain_read(obj, grain, data_mem, data_store, key);

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

    enc_object_free(&obj);

    return !is_zero;
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

typedef struct dummy_meta_ {
    int i;
    char c;
} dummy_meta;

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
    dummy_meta o_meta = { 13, 'x' };
    enc_object_opaque_meta_put(&obj, &o_meta, sizeof(o_meta));

    void* obj_meta = malloc(enc_object_get_meta_size(obj));

    enc_object_put_meta(obj, obj_meta);

    enc_object obj2;

    enc_object_parse_meta(&obj2, obj_meta);
    dummy_meta* o_meta2 = enc_object_opaque_meta_get(&obj2);

    int ret = !(obj2.grain_cnt == obj.grain_cnt && obj2.tag_size == obj.tag_size && strcmp(obj2.tag, obj.tag) == 0 &&
            o_meta.c == o_meta2->c && o_meta.i == o_meta2->i);

    free(obj_meta);
    return 0;
}

int main(int argc, char** argv) {
    printf("test_enc_object_make: %d\n", test_enc_object_make());
    printf("test_enc_object_free: %d\n", test_enc_object_free());
    printf("test_enc_object_add_grain: %d\n", test_enc_object_add_grain());
    printf("test_enc_object_grain_write: %d\n", test_enc_object_grain_write());
    printf("test_enc_object_grain_data_read: %d\n", test_enc_object_grain_read());
    printf("test_enc_object_put_meta: %d\n", test_enc_object_put_meta());
    printf("test_enc_object_parse_meta: %d\n", test_enc_object_parse_meta());

    return 0;
}
