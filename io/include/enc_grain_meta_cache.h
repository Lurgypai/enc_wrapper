#pragma once
#include "enc_grain.h"
#include "string.h"

// cache in the enc store for the grain meta of an object being accessed 

/*
 * needs to
 *  load object
 *  retrieve grain meta
 */

#define ENC_GRAIN_META_CACHE_COUNT (128)

typedef struct enc_grain_meta_cache_desc_ {
    size_t obj_idx;
    size_t grain_idx;
    enc_grain_meta meta;
} enc_grain_meta_cache_desc;

// simple rotating buffer implementation first

typedef struct enc_grain_meta_cache_ {
    size_t front;
    size_t size;
    enc_grain_meta_cache_desc data_[ENC_GRAIN_META_CACHE_COUNT];
} enc_grain_meta_cache;

enc_grain_meta_cache enc_grain_meta_cache_make();
void enc_grain_meta_cache_free(enc_grain_meta_cache* cache);

// put one element into the cache, LRU style
void enc_grain_meta_cache_put(enc_grain_meta_cache* cache, size_t obj_idx, size_t grain_idx, enc_grain_meta* meta);
// put a list of elements in
void enc_grain_meta_cache_put_all(enc_grain_meta_cache* cache, enc_grain_meta* meta, size_t obj_idx, size_t* grain_idx, size_t size);

// returns NULL if not found
enc_grain_meta* enc_grain_meta_cache_get(enc_grain_meta_cache* cache, size_t obj_idx, size_t grain_idx);

enc_grain_meta* enc_grain_meta_cache_at(enc_grain_meta_cache* cache, size_t idx);
