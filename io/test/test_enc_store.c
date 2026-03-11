#include "enc_store.h"
#include "enc_wrapper.h"

#define REGION_SIZE 4096

int main(int argc, char** argv) {
    enc_config meta_cfg = {
        .alg = aes256,
        .lib = enc_lib_gcrypt
    };
    enc_load_config(meta_cfg);
    char* key = enc_make_key();

    enc_store store = enc_store_create("test.store", meta_cfg);
    enc_store_add_object(&store, "obj1");
    enc_object* obj1 = enc_store_get_object(store, "obj1");

    enc_grain_meta grain1 = {
        .cfg = meta_cfg,
        .size = REGION_SIZE
    };

    enc_object_add_grain(obj1, grain1);

    enc_store_close(store, key);

    store = enc_store_open("test.store", key);
    enc_object* obj2 = enc_store_get_object(store, "obj1");

    enc_store_close(store, key);
    return 0;
}
