#pragma once
#include "enc_grain.h"

#include <string.h>

#include "enc_object.h"

// open
// close
// add object
// read object
// write object

typedef struct enc_store_contig_ {
    int file;
    enc_config cfg;
    size_t obj_cnt;
    size_t obj_reserved;
    enc_object* objs;
    // offset to the data of each object on disk
    size_t* obj_offsets;
    size_t cur_offset;
} enc_store_contig;

// footer layout
// obj_offsets - object meta blob - object count
// the object meta blob is constructed by getting the object meta as a blob

enc_store_contig enc_store_contig_open(char* filename, char* key);

// open file
enc_store_contig enc_store_contig_create(char* filename, enc_config cfg);
// close file
void enc_store_contig_close(enc_store_contig store, char* key);

// add object to file
void enc_store_contig_add_object(enc_store_contig* store, char* tag);

// get object and grain meta from file, return the grain information
enc_grain_meta enc_store_contig_get_meta(enc_store_contig store, char* tag, char* key);

// set object meta
void enc_store_contig_set_meta(enc_store_contig store, char* tag, enc_grain_meta meta);

// read object
void enc_store_contig_read_object(enc_store_contig store, char* tag, char* key, void* data);
// write obejct
void enc_store_contig_write_object(enc_store_contig* store, char* tag, char* key, void* data);

// replicate vol connector
