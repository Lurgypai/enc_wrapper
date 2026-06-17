#pragma once

#include <string.h>
#include "enc_object.h"

#define INVALID_OBJECT_ID SIZE_MAX
#define GRAIN_META_BUFFER_SIZE 128

typedef enum enc_object_layout_ {
    enc_object_layout_joined,
    enc_object_layout_split
} enc_object_layout;

// add the grain index

typedef struct enc_object_desc_ {
    enc_object obj;
    enc_object_layout layout;
} enc_object_desc;

typedef struct enc_store_ {
    int root_file;
    char* name;
    enc_config cfg;
    size_t obj_cnt;
    size_t obj_reserved;
    enc_object_desc* objs;

    char* cur_joined_obj;
    enc_grain_meta* joined_obj_grains;
    size_t joined_obj_reserved;
} enc_store;

enc_store enc_store_create(const char* filename, enc_config cfg);
enc_store enc_store_open(const char* filename, char* key);
void enc_store_close(enc_store* store, char* key);

void enc_store_add_object(enc_store* store, const char* tag, enc_object_layout layout);
enc_object* enc_store_get_object(enc_store store, const char* tag);
void enc_store_add_grain(enc_store* store, const char* tag, enc_grain_meta grain, char* key);
void enc_store_index_write(enc_store* store, const char* tag, char* key);
void enc_store_index_read(enc_store* store, const char* tag, char* key);

void enc_store_write(enc_store* store, const char* tag, size_t offset, size_t size, const void* in_data, char* key);
void enc_store_read(enc_store* store, const char* tag, size_t offset, size_t size, void* out_data, char* key);


