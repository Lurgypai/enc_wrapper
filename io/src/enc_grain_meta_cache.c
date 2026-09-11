#include "enc_grain_meta_cache.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "enc_util.h"
#include "enc_wrapper.h"

enc_grain_meta_cache enc_grain_meta_cache_make() {
    enc_grain_meta_cache cache;
    // start at the end. new elements (in the ring buffer) are inserted in front of this)
    cache.front = 0;
    cache.size = 0;
    memset(cache.data_, 0, sizeof(cache.data_));

    return cache;
}

void enc_grain_meta_cache_free(enc_grain_meta_cache* cache) {
}

void enc_grain_meta_cache_put(enc_grain_meta_cache* cache,
        size_t obj_idx, size_t grain_idx,
        enc_grain_meta* meta) {
    enc_grain_meta_cache_desc desc = {
        obj_idx,
        grain_idx,
        *meta
    };

    // rotate
    if(cache->front == 0) cache->front = ENC_GRAIN_META_CACHE_COUNT - 1;
    else --cache->front;

    cache->data_[cache->front] = desc;
    if(cache->size < ENC_GRAIN_META_CACHE_COUNT) ++cache->size;
}

void enc_grain_meta_cache_put_all(enc_grain_meta_cache* cache,
       enc_grain_meta* meta, size_t obj_idx, size_t*grain_idx, size_t size) {
    if(size > ENC_GRAIN_META_CACHE_COUNT) {
        fprintf(stderr, "enc_grain_meta_cache.c: size is too big in put all");
        exit(1);
    }
    for(int meta_idx = 0; meta_idx != size; ++ meta_idx) {
        size_t pos = cache->front + meta_idx;
        pos %= ENC_GRAIN_META_CACHE_COUNT;

        enc_grain_meta_cache_desc desc = {
            obj_idx,
            grain_idx[meta_idx],
            meta[meta_idx]
        };

        cache->data_[pos] = desc;
    }
    cache->size = size;
}

enc_grain_meta* enc_grain_meta_cache_get(enc_grain_meta_cache* cache,
        size_t obj_idx, size_t grain_idx) {
    for(int meta_idx = 0; meta_idx != cache->size; ++meta_idx) {
        size_t pos = cache->front + meta_idx;
        pos %= ENC_GRAIN_META_CACHE_COUNT;
        enc_grain_meta_cache_desc* desc = &cache->data_[pos];
        if(desc->obj_idx == obj_idx && desc->grain_idx == grain_idx) {
            return &desc->meta;
        }
    }
    return NULL;
}

enc_grain_meta* enc_grain_meta_cache_at(enc_grain_meta_cache* cache, size_t idx) {
    if(idx >= cache->size) return NULL;

    enc_grain_meta_cache_desc* desc = &cache->data_[
        (cache->front + idx) % ENC_GRAIN_META_CACHE_COUNT
    ];

    return &desc->meta;
}

static void read_grain_joined(enc_grain_meta_cache* cache, enc_store* store,
        size_t obj_idx, size_t grain_idx, char* key) {
    // TODO
    // check and evict
    //  if current obj and grain are wrong
    //      minimum rank with incorrect obj and grain needs to read the grain meta in
    //      minimum rank needs to collect the grains
    //      minimum rank needs to write

    // have rank 0 load and distribute
    // TODO
    //  change to lowest rank on node

    int my_rank = 0;
    int assignee_rank = 0;
    void * data;

#ifdef ENABLE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
#endif
    if(my_rank == assignee_rank) {
        enc_object* obj = &store->objs[obj_idx].obj;

        char* index_filename = malloc(10 + 3);
        index_filename[0] = '\0';
        sprintf(index_filename, "%lu-g", obj_idx);

        char * filename = append_path(store->name, index_filename);
        int file = open(filename, O_RDWR, 0644);
        free(index_filename);

        enc_load_config(store->cfg);
        enc_set_key(key, enc_get_key_size());

        size_t blob_size = obj->grain_cnt * sizeof(enc_grain_meta);
        data = malloc(blob_size);
        size_t encrypted_size = enc_get_encrypted_size(store->cfg, blob_size);

        void* src = mmap_unaligned(file, encrypted_size, 0);

        size_t nonce_size = enc_get_nonce_size();
        char* nonce = malloc(nonce_size);
        memcpy(nonce, src, nonce_size);
        enc_set_nonce(nonce, nonce_size);

        enc_decrypt(src + nonce_size, blob_size, data, blob_size);

        free(nonce);
        munmap_unaligned(src, encrypted_size, 0);
        free(filename);
    }
#ifdef ENABLE_MPI

#endif
}

void enc_grain_meta_cache_read_grain(enc_grain_meta_cache* cache, enc_store* store,
        size_t obj_idx, size_t grain_idx, char* key) {
    enc_object_desc* desc = &store->objs[obj_idx];
    switch(desc->layout) {
        case enc_object_layout_joined:

             break;
        case enc_object_layout_split:
             break;
    }
}
