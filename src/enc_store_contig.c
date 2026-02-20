#include "enc_store_contig.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <enc_wrapper.h>


enc_store_contig enc_store_contig_create(char* filename) {
    enc_store_contig store;
    store.obj_cnt = 0;
    store.obj_reserved = 0;
    store.objs = NULL;
    store.obj_offsets = NULL;

    store.file = open(filename, O_CREAT | O_RDWR);

    return store;
}

enc_store_contig enc_store_contig_open(char* filename) {
    enc_store_contig store;
    store.obj_cnt = 0;
    store.obj_reserved = 0;
    store.objs = NULL;
    store.obj_offsets = NULL;

    store.file = open(filename, O_RDWR);

    // parse object count
    size_t offset = sizeof(store.obj_cnt);
    lseek(store.file, -offset, SEEK_END);
    read(store.file, &store.obj_cnt, sizeof(store.obj_cnt));

    store.obj_reserved = store.obj_cnt;
    store.objs = malloc(store.obj_cnt * sizeof(enc_object));
    store.obj_offsets = malloc(store.obj_cnt * sizeof(size_t));

    offset += store.obj_cnt * sizeof(enc_object);
    lseek(store.file, -offset, SEEK_END);
    read(store.file, store.objs, store.obj_cnt * sizeof(enc_object));

    offset += store.obj_cnt * sizeof(size_t);
    lseek(store.file, -offset, SEEK_END);
    read(store.file, store.obj_offsets, store.obj_cnt * sizeof(size_t));

    return store;
}

void enc_store_contig_close(enc_store_contig store) {
    free(store.objs);
    free(store.obj_offsets);
    close(store.file);
}

// review the interface
// the store needs to query the object for information about its grains
//
// change to get meta instead of get grain
// get meta will return the meta data for the backing grain
// to do this, we should use the object interface to read an object, and then retrieve the one grain from that
//      separate meta and data read
//
// to read data, we use the previously read metadata to pull data out

enc_grain_meta enc_store_contig_get_meta(enc_store_contig store, char* tag, enc_config config, char* key) {
    // find the object
    enc_object* obj = NULL;
    int i;
    for(i = 0; i != store.obj_cnt; ++i) {
        obj = store.objs + i;
        if(strcmp(obj->tag, tag) == 0) break;
    }

    // unable to find object, actually handle this at some point
    if( obj == NULL ) {
        fprintf(stderr, "ERROR: Unable to find object \"%s\" in store.\n", tag);
        exit(1);
    }

    enc_object_grains_meta_read(*obj, config, key);

    return obj->grains[0].grain;
}


