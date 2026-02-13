#include "enc_object.h"

#include <stdlib.h>

#include "enc_wrapper.h"

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

size_t enc_object_get_meta_size(enc_object obj) {
    return sizeof(obj.grain_cnt) + sizeof(obj.tag_size) + obj.tag_size;
}

void enc_object_get_meta(enc_object obj, void* meta_store) {
    memcpy(meta_store, &obj.grain_cnt, sizeof(obj.grain_cnt));
    memcpy(meta_store + sizeof(obj.grain_cnt), &obj.tag_size, sizeof(obj.tag_size));
    memcpy(meta_store + sizeof(obj.grain_cnt) + sizeof(obj.tag_size), obj.tag, obj.tag_size);

}

void enc_object_parse_meta(enc_object* obj, void* meta_store) {
    memcpy(&obj->grain_cnt, meta_store, sizeof(obj->grain_cnt));
    memcpy(&obj->tag_size, meta_store + sizeof(obj->grain_cnt), sizeof(obj->tag_size));
    memcpy(obj->tag, meta_store + sizeof(obj->grain_cnt) + sizeof(obj->tag_size), obj->tag_size);
}
