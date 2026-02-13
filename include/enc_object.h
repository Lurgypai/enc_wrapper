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
 */

typedef struct enc_grain_io_ {
    enc_grain_meta grain;
    enc_grain_layout layout;
} enc_grain_io;

typedef struct enc_object_ {
    char* tag;
    size_t tag_size;
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

// these automatically write the grain and the config to their respective locations as defined by their layouts
// this means the meta and raw data are encrypted separately, ie, no collective meta optimization
void enc_object_grains_read(enc_object obj, enc_config meta_conf, char* key);
void enc_object_grains_write(enc_object obj, enc_config meta_conf, char* key);

size_t enc_object_get_meta_size(enc_object obj);
// puts the object metadata as a contiguous buffer into meta_store, for encryption and writing
// space allocation is the job of whoever is passing the data
void enc_object_put_meta(enc_object obj, void* meta_store);
// parses meta from blob
void enc_object_parse_meta(enc_object* obj, void* meta_store);
