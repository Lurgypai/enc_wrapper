#include "enc_object.h"

#include <stdlib.h>

enc_object enc_object_make(const char* tag) {
    enc_object obj;
    obj.tag = strdup(tag);
    obj.tag_size = strlen(tag);
    obj.grain_cnt = 0;
    obj.cur_grain_offset = 0;
    obj.idx = enc_grain_index_make();
    return obj;
}

void enc_object_free(enc_object* obj) {
    enc_grain_index_free(&obj->idx);
    free(obj->tag);
}

size_t enc_object_add_grain(enc_object* obj, enc_grain_meta grain) {
    size_t pos = obj->grain_cnt;
    ++obj->grain_cnt;

    enc_grain_index_add_grain(&obj->idx, pos, obj->cur_grain_offset, grain.size);

    obj->cur_grain_offset += grain.size;
    return pos;
}

void enc_object_grain_read(enc_object obj, enc_grain_meta grain, void* data_mem, void* data_store, char* key) {
    enc_grain_data_read(grain, data_store, data_mem, key);
}

void enc_object_grain_write(enc_object obj, enc_grain_meta grain, void* data_mem, void* data_store, char* key) {
    enc_grain_data_write(grain, data_store, data_mem, key);
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
}
