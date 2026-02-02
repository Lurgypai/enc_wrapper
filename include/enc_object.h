#pragma once

#include <string.h>

#include "enc_grain.h"

typedef struct enc_grain_io_ {
    enc_grain_meta grain;
    enc_grain_layout layout;
} enc_grain_io;

typedef struct enc_object_ {
    char* tag;
    // size
    size_t grain_cnt;
    // backing size
    size_t grain_reserve;
    enc_grain_io* grains;
} enc_object;

enc_object enc_object_make(const char* tag);
void enc_object_free(enc_object obj);

size_t enc_object_add_grain(enc_object* obj, enc_grain_meta grain);
void enc_object_set_grain_layout(enc_object* obj, size_t pos, enc_grain_layout layout);

// before these calls the metadata config needs to be set
void enc_object_grains_read(enc_object obj, enc_config meta_conf, char* key);
void enc_object_grains_write(enc_object obj, enc_config meta_conf, char* key);

void enc_object_meta_read(enc_object* obj, enc_config meta_config, void* meta_store, char* key);
void enc_object_meta_write(enc_object obj, enc_config meta_config, void* meta_store, char* key);

