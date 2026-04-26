#pragma once

#include <string.h>
#include "enc_object.h"

typedef struct enc_store_grain_index_desc_ {
    size_t offset;
    size_t size;
} enc_store_grain_index_desc;

typedef struct enc_store_grain_index_ {
} enc_store_grain_index;

typedef enum enc_object_layout_ {
    enc_object_layout_joined,
    // enc_object_layout_split
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
} enc_store;

enc_store enc_store_create(const char* filename, enc_config cfg);
enc_store enc_store_open(const char* filename, char* key);

void enc_store_close(enc_store store, char* key);

void enc_store_add_object(enc_store* store, const char* tag, enc_object_layout layout);
enc_object* enc_store_get_object(enc_store store, const char* tag);

void enc_store_grains_write(enc_store store, const char* tag, char* key);
void enc_store_grains_read(enc_store store, const char* tag, char* key);

void enc_store_write(enc_store store, const char* tag, size_t offset, size_t size, const void* in_data, char* key);
void enc_store_read(enc_store store, const char* tag, size_t offset, size_t size, void* out_data, char* key);


