#include "enc_store_contig.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <enc_wrapper.h>

enc_store_contig enc_store_contig_open(char* filename) {
    enc_store_contig store;
    store.obj_cnt = 0;
    store.obj_reserved = 0;
    store.objs = NULL;
    store.obj_offsets = NULL;

    store.file = open(filename, O_CREAT | O_APPEND| O_RDWR);

    return store;
}

void enc_store_contig_close(enc_store_contig store) {
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

enc_grain_meta enc_store_contig_get_grain(enc_store_contig store, char* tag, enc_config config, char* key) {
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

    enc_load_config(config);

    // get meta + nonce as ptr
    void* meta_store = mmap(NULL, sizeof(enc_grain_meta) + enc_get_nonce_size(), PROT_READ, 0644, store.file, store.obj_offsets[i]);

    return enc_grain_meta_read(meta_store, key);
}


