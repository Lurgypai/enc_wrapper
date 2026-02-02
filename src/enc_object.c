#include "enc_object.h"

#include <stdlib.h>

#include "enc_wrapper.h"

enc_object enc_object_make(const char* tag) {
    enc_object obj;
    obj.tag = strdup(tag);
    obj.grain_cnt = 0;
    obj.grain_reserve = 0;
    obj.grains = NULL;
    return obj;
}

void enc_object_free(enc_object obj) {
    free(obj.tag);
    if(obj.grains != NULL) free(obj.grains);
}

size_t enc_object_add_grain(enc_object* obj, enc_grain_meta grain) {
    size_t pos = obj->grain_cnt;
    ++obj->grain_cnt;

    // if null assign
    if(obj->grains == NULL) {
        obj->grains = malloc(sizeof(enc_grain_io));
        obj->grain_reserve = 1;
    }
    else if (obj->grain_cnt > obj->grain_reserve) {
        obj->grain_reserve *= 2;
        obj->grains = realloc(obj->grains, obj->grain_reserve);
    }
    
    obj->grains[pos].grain = grain;
    return pos;
}

void enc_object_set_grain_layout(enc_object* obj, size_t pos, enc_grain_layout layout) {
    obj->grains[pos].layout = layout;
}

void enc_object_grains_read(enc_object obj, enc_config meta_config, char* key) {
    for(int grain_pos = 0; grain_pos != obj.grain_cnt; ++grain_pos) {
        enc_load_config(meta_config);
        obj.grains[grain_pos].grain = enc_grain_meta_read(obj.grains[grain_pos].layout.meta_store, key);
        enc_grain_data_read(obj.grains[grain_pos].grain, obj.grains[grain_pos].layout.data_store, obj.grains[grain_pos].layout.data_mem, key);
    }
}

void enc_object_grains_write(enc_object obj, enc_config meta_config, char* key) {
    for(int grain_pos = 0; grain_pos != obj.grain_cnt; ++grain_pos) {
        enc_load_config(meta_config);
        enc_grain_write(obj.grains[grain_pos].grain, obj.grains[grain_pos].layout, key);
    }
}

// how do we encrypt a variable sized blob?
// collect it all and then encrypt
// how do we collect across objects? function to pull metadata
// function to group write object meta
//  does this make sense? changes depending on layout
// function to 
typedef struct enc_object_meta_ {
} enc_object_meta;

void enc_object_meta_read(enc_object* obj, enc_config meta_config, void* meta_store, char* key) {
    enc_load_config(meta_config);
    enc_set_key(key, enc_get_key_size());

    size_t nonce_size = enc_get_nonce_size();
    char* nonce = malloc(nonce_size);
    memcpy(nonce, meta_store, nonce_size);
    enc_set_nonce(nonce, nonce_size);



    free(nonce);
}

void enc_object_meta_write(enc_object obj, enc_config meta_config, void* meta_store, char* key) {
    enc_load_config(meta_config);
    enc_set_key(key, enc_get_key_size());

    size_t nonce_size = enc_get_nonce_size();
    char* nonce = enc_make_nonce();



    free(nonce);
}
