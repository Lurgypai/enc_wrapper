#include "enc_object.h"

#include <stdlib.h>

enc_object enc_object_make(const char* tag) {
    enc_object obj;
    obj.tag = strdup(tag);
    obj.tag_size = strlen(tag);
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
        obj->grains = malloc(sizeof(enc_grain_meta));
        obj->grain_reserve = 1;
    }
    else if (obj->grain_cnt > obj->grain_reserve) {
        obj->grain_reserve *= 2;
        obj->grains = realloc(obj->grains, obj->grain_reserve * sizeof(enc_grain_meta));
    }
    
    obj->grains[pos] = grain;
    return pos;
}

void enc_object_grain_meta_read(enc_object obj, size_t grain_idx, enc_config meta_conf, char* key, void* meta_store) {
    obj.grains[grain_idx] = enc_grain_meta_read(meta_store, key);
}

void enc_object_grain_data_read(enc_object obj, size_t grain_idx, char* key, void* data_mem, void* data_store) {
    enc_grain_data_read(obj.grains[grain_idx], data_store, data_mem, key);
}

void enc_object_grain_write(enc_object obj, size_t grain_idx, enc_config meta_conf, char* key, void* meta_store, void* data_mem, void* data_store) {
    enc_grain_write(meta_conf, obj.grains[grain_idx], meta_store, data_mem, data_store, key);
}

size_t enc_object_get_meta_size(enc_object obj) {
    return sizeof(obj.grain_cnt) + sizeof(obj.tag_size) + obj.tag_size;
}

void enc_object_put_meta(enc_object obj, void* meta_store) {
    memcpy(meta_store, &obj.grain_cnt, sizeof(obj.grain_cnt));
    memcpy(meta_store + sizeof(obj.grain_cnt), &obj.tag_size, sizeof(obj.tag_size));
    memcpy(meta_store + sizeof(obj.grain_cnt) + sizeof(obj.tag_size), obj.tag, obj.tag_size);

}

void enc_object_parse_meta(enc_object* obj, void* meta_store) {
    memcpy(&obj->grain_cnt, meta_store, sizeof(obj->grain_cnt));
    memcpy(&obj->tag_size, meta_store + sizeof(obj->grain_cnt), sizeof(obj->tag_size));
    obj->tag = malloc(obj->tag_size + 1);
    obj->tag[obj->tag_size] = '\0';
    memcpy(obj->tag, meta_store + sizeof(obj->grain_cnt) + sizeof(obj->tag_size), obj->tag_size);

    obj->grain_reserve = obj->grain_cnt;
    obj->grains = NULL;
}
