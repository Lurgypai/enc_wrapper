#include "enc_grain_index.h"

#include <stdlib.h>

// DEBUG
#include <stdio.h>

enc_grain_index enc_grain_index_make() {
    enc_grain_index idx = {
        .grains = NULL,
        .cnt = 0,
        .reserved = 0
    };

    return idx;
}

void enc_grain_index_free(enc_grain_index* idx) {
    if(idx->grains != NULL) free(idx->grains);
    idx->grains = NULL;
    idx->cnt = 0;
    idx->reserved = 0;
}

void enc_grain_index_add_grain(enc_grain_index* idx, int id, size_t offset, size_t size) {
    // allocate first
    if(idx->cnt == 0) {
        idx->reserved = 1;
        idx->grains = malloc(sizeof(enc_grain_index_desc));
    }

    // realloc if full
    else if(idx->cnt == idx->reserved) {
        idx->reserved *= 2;
        idx->grains = realloc(idx->grains, sizeof(enc_grain_index_desc) * idx->reserved);
    }

    // insert at end
    idx->grains[idx->cnt].id = id;
    idx->grains[idx->cnt].offset = offset;
    idx->grains[idx->cnt].size = size;

    // update count
    ++idx->cnt;
}

void enc_grain_index_select_grains(enc_grain_index* idx, size_t offset, size_t size, size_t** ids_, size_t* id_cnt_) {
    // tradeoff for speed over memory efficiency here
    // consider dynamically sizing
    size_t* ids = malloc(sizeof(size_t) * idx->cnt);
    size_t id_cnt = 0; 

    for(size_t pos = 0; pos != idx->cnt; ++pos) {
        enc_grain_index_desc* cur_grain = &idx->grains[pos];
        if(offset < cur_grain->offset + cur_grain->size && cur_grain->offset < offset + size) {
            ids[id_cnt] = pos;
            ++id_cnt;
        }
        else if(offset + size <= cur_grain->offset) break;
    } 

    // set return values
    *ids_ = ids;
    *id_cnt_ = id_cnt;
}

size_t enc_grain_index_get_size(enc_grain_index* idx) {
    size_t size = sizeof(idx->cnt) + sizeof(idx->reserved) + sizeof(enc_grain_index_desc) * idx->cnt;
    return size;
}

void enc_grain_index_put(enc_grain_index* idx, void* store) {
    memcpy(store, &idx->cnt, sizeof(idx->cnt));
    memcpy(store + sizeof(idx->cnt), &idx->reserved, sizeof(idx->reserved));
    memcpy(store + sizeof(idx->cnt) + sizeof(idx->reserved), idx->grains, sizeof(enc_grain_index_desc) * idx->cnt);
}

void enc_grain_index_parse(enc_grain_index* idx, void* store) {
    memcpy(&idx->cnt, store, sizeof(idx->cnt));
    memcpy(&idx->reserved, store + sizeof(idx->cnt), sizeof(idx->reserved));

    idx->grains = malloc(sizeof(enc_grain_index_desc) * idx->cnt);
    memcpy(idx->grains, store + sizeof(idx->cnt) + sizeof(idx->reserved), sizeof(enc_grain_index_desc) * idx->cnt);
}

