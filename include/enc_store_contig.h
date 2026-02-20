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
    size_t obj_cnt;
    size_t obj_reserved;
    enc_object* objs;
    // offset to the data of each object on disk
    size_t* obj_offsets;
    // location of meta footer
} enc_store_contig;

// footer layout
// obj_offsets - object meta blob - object count
// the object meta blob is constructed by getting the object meta as a blob

// open file
enc_store_contig enc_store_contig_open(char* filename);
// close file
void enc_store_contig_close(enc_store_contig store);

// add object to file
void enc_store_contig_add_object(enc_store_contig* store, char* tag);

// get object (grain) meta from file
enc_grain_meta enc_store_contig_get_grain(enc_store_contig store, char* tag, enc_config config, char* key);
// set object (grain) meta
void enc_store_contig_set_grain(char* tag, enc_grain_meta);

// read object
void enc_store_contig_read_object(char* tag, void* data_store);
// write obejct
void enc_store_contig_write_object(char* tag, void* data);

// replicate vol connector
