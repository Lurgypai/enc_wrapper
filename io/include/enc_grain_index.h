#pragma once

#include <string.h>

/*
 *  grain index is used in the objects
 *  one is created for each object to track that objects grains
 */


typedef struct enc_grain_index_desc_ {
    // pos in the objects grain array
    int id;
    // pos in "file"
    size_t offset;
    // size in "file"
    size_t size;
} enc_grain_index_desc;

typedef struct enc_grain_index_ {
    size_t cnt;
    size_t reserved;
    enc_grain_index_desc* grains;
} enc_grain_index;

enc_grain_index enc_grain_index_make();
void enc_grain_index_free(enc_grain_index* idx);

void enc_grain_index_add_grain(enc_grain_index* idx, int id, size_t offset, size_t size);

// selects grains that overlap with input selection
void enc_grain_index_select_grains(enc_grain_index* idx, size_t offset, size_t size, size_t** ids, size_t* id_cnt);

size_t enc_grain_index_get_size(enc_grain_index* idx);
void enc_grain_index_put(enc_grain_index* idx, void* store);
void enc_grain_index_parse(enc_grain_index* idx, void* store);
