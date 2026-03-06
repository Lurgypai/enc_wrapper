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

    printf("Reading first object\n");
    enc_store_test_read_object(store, "test_object_1", key, data);

    printf("Reading second object\n");
    enc_store_test_read_object(store, "test_object_2", key, data);

    return 0;
}
