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
    for(int i = 0; i != store.obj_cnt; ++i) {
        enc_object_free(store.objs[i]);
    }
    free(store.objs);
    free(store.obj_offsets);
    close(store.file);
}

void enc_store_contig_add_object(enc_store_contig* store, char* tag) {
    size_t pos = store->obj_cnt;
    ++store->obj_cnt;

    if(store->objs == NULL) {
        store->objs = malloc(sizeof(enc_object));
        store->obj_reserved = 1;
    }
    else if (store->obj_cnt > store->obj_reserved) {
        store->obj_reserved *= 2;
        store->objs = realloc(store->objs, store->obj_reserved);
    }

    store->objs[pos] = enc_object_make(tag);
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

static enc_object* get_object(enc_store_contig store, char* tag) {
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
    return obj;
}

enc_grain_meta enc_store_contig_get_meta(enc_store_contig store, char* tag, enc_config config, char* key) {
    enc_object* obj = get_object(store, tag);
    enc_object_grains_meta_read(*obj, config, key);
    return obj->grains[0].grain;
}

void enc_store_contig_set_meta(enc_store_contig store, char* tag, enc_grain_meta meta) {
    enc_object* obj = get_object(store, tag);
    // we already have a representative grain, oh no!
    if(obj->grain_cnt > 0) {
        fprintf(stderr, "ERROR: contig store object \"%s\" already has a backing grain.\n", tag);
        exit(1);
    }
    enc_object_add_grain(obj, meta);
}

void enc_store_contig_read_object(enc_store_contig store, char * tag, void* data) {
    // do we know where the object is?
    //      yes, we should have the file and the offset
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

    size_t offset = store.obj_offsets[i];
    for(int grain_pos = 0; grain_pos != obj->grain_cnt; ++grain_pos) {
        // need to set the layout
        // set pointer to meta_store, data_store, and data
        // set layout
        // mmap entire grain
        // read grain
        // -------> NEED TO ADD FUNCTION TO READ INDIVIDUAL GRAIN

        enc_grain_layout layout;
        layout.data_mem = data;
        // grain is a contiguous meta + data blob
        layout.meta_store = mmap(NULL, sizeof(enc_grain_meta), PROT_READ, MAP_SHARED, store.file, offset);
    }
}

void enc_store_contig_write_object(char* tag, void* data) {

}

// need to whiteboard out the io processes
//  contig store
//  read
//      open the store
//          this should load the objects (offset to each object (meta + data), object meta (tag mostly)
//      read an object
//          lookup object by meta
//          for each grain
//              set grain layout
//              read metadata for encryption config
//              read data
//  
//  write
//      create the store
//      add an object
//      add a grain to the object, with the metadata for that grain
//      write the object
//          for each grain
//              set the grain layout
//          write all grains
