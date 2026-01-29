#pragma once
#include "enc_grain.h"

typedef struct enc_grain_io_ {
    enc_grain grain;
    enc_grain_layout layout;
} enc_grain_io;

typedef struct enc_object_ {
    char* tag;
    size_t grain_cnt;
    enc_grain_io* grains;
} enc_object;

enc_object enc_object_make(const char* tag);
void enc_object_free(enc_object obj);

void enc_object_add_grain(enc_object* obj, enc_grain grain);
void enc_object_set_grain_layout(enc_object* obj, size_t pos, enc_grain_layout);

enc_object enc_object_read(char* key);
void enc_object_write(enc_object obj, char* key);

