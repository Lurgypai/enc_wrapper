#include <stdio.h>
#include <stdlib.h>

#include "enc_store.h"
#include "enc_wrapper.h"


#define REGION_SIZE 4096

int main(int argc, char** argv) {
    enc_config meta_cfg = {
        .alg = aes256,
        .lib = enc_lib_gcrypt
    };

    enc_grain_meta grain_template = {
        .cfg = meta_cfg,
        .size = REGION_SIZE
    };

    char data_in[REGION_SIZE];
    for(int i = 0; i != REGION_SIZE; ++i) {
        data_in[i] = i % 256;
    }
    char data_out[REGION_SIZE] = {0};

    enc_load_config(meta_cfg);
    char* key = enc_make_key();

    enc_store store = enc_store_create("test.store", meta_cfg);
    enc_store_add_object(&store, "single_grain", enc_object_layout_joined);
    enc_store_add_object(&store, "three_grains_1", enc_object_layout_joined);
    enc_store_add_object(&store, "three_grains_2", enc_object_layout_joined);

    enc_object* obj_single = enc_store_get_object(store, "single_grain");
    enc_object_add_grain(obj_single, grain_template);

    enc_object* obj_three_1 = enc_store_get_object(store, "three_grains_1");
    enc_object_add_grain(obj_three_1, grain_template);
    enc_object_add_grain(obj_three_1, grain_template);
    enc_object_add_grain(obj_three_1, grain_template);

    enc_object* obj_three_2 = enc_store_get_object(store, "three_grains_2");
    enc_object_add_grain(obj_three_2, grain_template);
    enc_object_add_grain(obj_three_2, grain_template);
    enc_object_add_grain(obj_three_2, grain_template);

    // aligned io on single
    enc_store_write(store, "single_grain", 0, REGION_SIZE, data_in, key);
    enc_store_grains_write(store, "single_grain", key);

    // aligned io in between grains
    enc_store_write(store, "three_grains_1", REGION_SIZE, REGION_SIZE, data_in, key);
    enc_store_grains_write(store, "three_grains_1", key);

    // unaligned io
    enc_store_write(store, "three_grains_2", REGION_SIZE / 2, REGION_SIZE, data_in, key);
    enc_store_grains_write(store, "three_grains_2", key);

    enc_store_close(store, key);

    store = enc_store_open("test.store", key);

    printf("Testing reading from single grain...\n");
    enc_store_grains_read(store, "single_grain", key);
    enc_store_read(store, "single_grain", 0, REGION_SIZE, data_out, key);
    for(int i = 0; i != REGION_SIZE; ++i) {
        if(data_in[i] != data_out[i]) {
            printf("data written and read don't match at %d, %d != %d\n", i, data_in[i], data_out[i]);
            break;
        }
    }

    memset(data_out, 0, REGION_SIZE);

    printf("Testing reading from aligned grain...\n");
    enc_store_grains_read(store, "three_grains_1", key);
    enc_store_read(store, "three_grains_1", REGION_SIZE, REGION_SIZE, data_out, key);
    for(int i = 0; i != REGION_SIZE; ++i) {
        if(data_in[i] != data_out[i]) {
            printf("data written and read don't match at %d, %d != %d\n", i, data_in[i], data_out[i]);
            break;
        }
    }

    memset(data_out, 0, REGION_SIZE);

    printf("Testing reading from unaligned grain...\n");
    enc_store_grains_read(store, "three_grains_2", key);
    enc_store_read(store, "three_grains_2", REGION_SIZE / 2, REGION_SIZE, data_out, key);
    for(int i = 0; i != REGION_SIZE; ++i) {
        if(data_in[i] != data_out[i]) {
            printf("data written and read don't match at %d, %d != %d\n", i, data_in[i], data_out[i]);
            break;
        }
    }

    enc_store_close(store, key);

    free(key);
    return 0;
}
