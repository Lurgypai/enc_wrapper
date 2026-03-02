#pragma once

#include <string.h>

#include "enc_grain.h"

/*
 * an object is a collection of grains (regions)
 * the io utilities here just write grains based on their layout, and do not optimize for co-located metadata
 * to write an object, one must
 *  somehow track the stored location of grains. this is outside the scope of the object, as it will change based on the on disk layout
 *  write the grains
 *  write the object meta
 *  write the stored location of the grains
 *
 * to read
 *  retrieve location of grains
 *  read object metadata
 *  set grain layouts to complete metadata
 *      this completes the metadata setup
 *  read grain meta using object grain meta read
 *  read grain data using objec grain data read
 */

typedef struct enc_object_ {
    char* tag;
    size_t tag_size;
    // size
    size_t grain_cnt;
    // backing size
    size_t grain_reserve;
    enc_grain_meta* grains;
} enc_object;

enc_object enc_object_make(const char* tag);
void enc_object_free(enc_object obj);

size_t enc_object_add_grain(enc_object* obj, enc_grain_meta grain);
void enc_object_set_grain_layout(enc_object* obj, size_t pos, enc_grain_layout layout);

// separate read so we can just read meta
void enc_object_grains_meta_read(enc_object obj, enc_config meta_conf, char* key);
void enc_object_grains_data_read(enc_object obj, char* key);

//write happens all at once (for now)
void enc_object_grains_write(enc_object obj, enc_config meta_conf, char* key);

size_t enc_object_get_meta_size(enc_object obj);
// puts the object metadata as a contiguous buffer into meta_store, for encryption and writing
// space allocation is the job of whoever is passing the data
void enc_object_put_meta(enc_object obj, void* meta_store);
// parses meta from blob
void enc_object_parse_meta(enc_object* obj, void* meta_store);
