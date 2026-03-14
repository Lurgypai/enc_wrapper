#pragma once

#include <string.h>
#include "enc_object.h"

typedef enum enc_object_layout_ {
    enc_object_layout_joined,
    // enc_object_layout_split
} enc_object_layout;

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

enc_store enc_store_create(char* filename, enc_config cfg);
enc_store enc_store_open(char* filename, char* key);

void enc_store_close(enc_store store, char* key);

void enc_store_add_object(enc_store* store, char* tag, enc_object_layout layout);
enc_object* enc_store_get_object(enc_store store, char* tag);

void enc_store_grains_write(enc_store store, char* tag, char* key);
void enc_store_grains_read(enc_store store, char* tag, char* key);

void enc_store_write(enc_store store, char* tag, size_t offset, size_t size, void* in_data, char* key);
void enc_store_read(enc_store store, char* tag, size_t offset, size_t size, void* out_data, char* key);


