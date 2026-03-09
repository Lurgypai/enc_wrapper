#include <stdio.h>

#include "enc_wrapper.h"
#include "enc_store_test.h"

#define OBJ_REGION_SIZE 4096

int main(int argc, char** argv) {
    enc_config meta_cfg = {
        .alg = aes256,
        .lib = enc_lib_gcrypt
    };
    
    printf("Creating store\n");
    enc_store_test store = enc_store_test_create("test.store", meta_cfg);

    printf("Adding object\n");
    enc_store_test_add_object(&store, "test_object_1");

    enc_grain_meta obj_meta = {
        .cfg = {
            .alg = aes256,
            .lib = enc_lib_gcrypt
        },
        .size = OBJ_REGION_SIZE
    };

    printf("Setting object metadata\n");
    enc_store_test_set_meta(store, "test_object_1", obj_meta);

    enc_load_config(meta_cfg);
    char* key = enc_make_key();

    char data[OBJ_REGION_SIZE] = {0};

    printf("Writing object\n");
    enc_store_test_write_object(&store, "test_object_1", key, data);

    printf("Adding second object\n");
    enc_store_test_add_object(&store, "test_object_2");

    printf("Setting second object metadata\n");
    enc_store_test_set_meta(store, "test_object_2", obj_meta);

    printf("Writing second object\n");
    enc_store_test_write_object(&store, "test_object_2", key, data);

    printf("Closing\n");
    enc_store_test_close(store, key);


    printf("Opening\n");
    store = enc_store_test_open("test.store", key);

    printf("Reading first object meta\n");
    enc_grain_meta meta = enc_store_test_get_meta(store, "test_object_1", key);

    int meta_correct = meta.cfg.alg == obj_meta.cfg.alg && meta.cfg.lib == obj_meta.cfg.lib && meta.size == obj_meta.size;
    printf("\tfirst meta is correct: %d\n", meta_correct);

    for(int i = 0; i != OBJ_REGION_SIZE; ++i) {
        data[i] = 1;
    }

    printf("Reading first object\n");
    enc_store_test_read_object(store, "test_object_1", key, data);

    int obj_correct = 1;
    for(int i = 0; i != OBJ_REGION_SIZE; ++i) {
        if(data[i] != 0) {
            obj_correct = 0;
            break;
        }
    }
    
    printf("\tfirst object is correct: %d\n", obj_correct);

    printf("Reading second object meta\n");
    enc_grain_meta meta2 = enc_store_test_get_meta(store, "test_object_2", key);
    meta_correct = meta2.cfg.alg == obj_meta.cfg.alg && meta2.cfg.lib == obj_meta.cfg.lib && meta2.size == obj_meta.size;
    printf("\tsecond meta is correct: %d\n", meta_correct);

    for(int i = 0; i != OBJ_REGION_SIZE; ++i) {
        data[i] = 1;
    }
    printf("Reading second object\n");
    enc_store_test_read_object(store, "test_object_2", key, data);
    obj_correct = 1;
    for(int i = 0; i != OBJ_REGION_SIZE; ++i) {
        if(data[i] != 0) {
            obj_correct = 0;
            break;
        }
    }
    
    printf("\tsecond object is correct: %d\n", obj_correct);

    return 0;
}
