#include "enc_store_test.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <enc_wrapper.h>

// footer layout
// offsets
// object metadata blob
// object count
// encrypted size
// enc config

static void* mmap_unaligned(int fd, size_t size, size_t offset) {
    long page_size = sysconf(_SC_PAGESIZE);

    // get page aligned offset
    size_t page_offset = (offset / page_size) * page_size;
    // get offset into output ptr
    size_t sub_offset = offset % page_size;

    void* raw_map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_offset);
    if(raw_map == MAP_FAILED) perror("ERROR MMAPING");
    return raw_map + sub_offset;
}

static void munmap_unaligned(void* ptr, size_t size, size_t offset) {
    long page_size = sysconf(_SC_PAGESIZE);
    size_t sub_offset = offset % page_size;

    void* raw_map = ptr - sub_offset;
    int err = munmap(raw_map, size);
    if(err < 0) perror("ERROR UNMAPPING");
}


enc_store_test enc_store_test_create(char* filename, enc_config cfg) {
    enc_store_test store;
    store.file = open(filename, O_CREAT | O_RDWR, 0644);
    store.cfg = cfg;
    store.obj_cnt = 0;
    store.obj_reserved = 0;
    store.objs = NULL;
    store.obj_offsets = NULL;
    store.cur_offset = 0;

    return store;
}

enc_store_test enc_store_test_open(char* filename, char* key) {
    enc_store_test store;
    store.file = open(filename, O_RDWR, 0644);
    store.obj_cnt = 0;
    store.obj_reserved = 0;
    store.objs = NULL;
    store.obj_offsets = NULL;

    // WARNING: as this is 0, writing to an opened file will start overwriting from the front.
    // needs to be fixed for complex io patterns
    store.cur_offset = 0;

    size_t cur_offset = 0;

    // parse the encryption config for the metadata
    size_t offset = sizeof(enc_config);
    lseek(store.file, -offset, SEEK_END);
    read(store.file, &store.cfg, sizeof(store.obj_cnt));

    // parse the size of the metadata blob
    size_t encrypted_size;
    offset += sizeof(encrypted_size);
    lseek(store.file, -offset, SEEK_END);
    read(store.file, &encrypted_size, sizeof(encrypted_size));
    
    // parse the number of objects
    offset += sizeof(store.obj_cnt);
    lseek(store.file, -offset, SEEK_END);
    read(store.file, &store.obj_cnt, sizeof(store.obj_cnt));

    //  as part of parsing the object count, prepare space for the objects and offsets
    store.obj_reserved = store.obj_cnt;
    store.objs = malloc(sizeof(enc_object) * store.obj_cnt);
    store.obj_offsets = malloc(sizeof(size_t) * store.obj_cnt);

    //  decrypt the meta blob
    offset += encrypted_size;
    size_t file_size = lseek(store.file, 0, SEEK_END);
    void* meta_blob = mmap_unaligned(store.file, encrypted_size, file_size - offset);
    char* nonce = meta_blob;
    enc_load_config(store.cfg);
    size_t nonce_size = enc_get_nonce_size();
    enc_set_nonce(nonce, nonce_size);
    enc_set_key(key, enc_get_key_size());

    size_t meta_size = encrypted_size - nonce_size;
    void* meta_buf = malloc(meta_size);
    enc_decrypt(meta_blob + nonce_size, meta_size, meta_buf, meta_size);

    size_t meta_buf_offset = 0;
    for(size_t obj_idx = 0; obj_idx != store.obj_cnt; ++obj_idx) {
        enc_object_parse_meta(store.objs + obj_idx, meta_buf + meta_buf_offset);
        meta_buf_offset += enc_object_get_meta_size(store.objs[obj_idx]);
    }

    free(meta_buf);
    munmap_unaligned(meta_blob, encrypted_size, file_size - offset);

    size_t obj_offsets_size = sizeof(size_t) * store.obj_cnt;
    offset += obj_offsets_size;
    read(store.file, store.obj_offsets, obj_offsets_size);

    return store;
}

void enc_store_test_close(enc_store_test store, char* key) {

    // ------- write object offsets
    size_t file_end = lseek(store.file, 0, SEEK_END);
    size_t offsets_size = store.obj_cnt * sizeof(size_t);
    write(store.file, store.obj_offsets, offsets_size);
    file_end += offsets_size;

    // ------- write object metadata
    // collect object meta with total size
    // get output ptr
    // encrypt and write object metadata
    size_t cur_size = 0; 
    void* meta_mem = NULL;
    for(int i = 0; i != store.obj_cnt; ++i) {
        // get metadata size
        size_t meta_size = enc_object_get_meta_size(store.objs[1]);
        // allocate space to put
        meta_mem = realloc(meta_mem, cur_size + meta_size);
        // put meta in, cur_size hasn't been updated, so its the old end offset
        enc_object_put_meta(store.objs[1], meta_mem + cur_size);
        // update cur_size
        cur_size += meta_size;

        enc_object_free(store.objs[i]);
    }

    size_t encrypted_size = enc_get_encrypted_size(store.cfg, cur_size);

    // allocate footer space
    ftruncate(store.file, file_end + encrypted_size);
    void* meta_store = mmap_unaligned(store.file, encrypted_size, store.cur_offset);

    // write nonce and encrypted data
    enc_load_config(store.cfg);
    size_t nonce_size = enc_get_nonce_size();
    char* nonce = enc_make_nonce();
    enc_set_nonce(nonce, nonce_size);
    memcpy(meta_store, nonce, nonce_size);
    free(nonce);
    enc_encrypt(meta_mem, cur_size, meta_store + nonce_size, cur_size);

    // cleanup
    munmap_unaligned(meta_store, encrypted_size, store.cur_offset);
    free(meta_mem);
    file_end += encrypted_size;

    // ------ write object count
    file_end = lseek(store.file, 0, SEEK_END);
    write(store.file, &store.obj_cnt, sizeof(store.obj_cnt));
    
    // ------ write footer encryption config
    file_end = lseek(store.file, 0, SEEK_END);
    write(store.file, &encrypted_size, sizeof(encrypted_size));
    write(store.file, &store.cfg, sizeof(store.cfg));

    // ------ cleanup
    free(store.objs);
    free(store.obj_offsets);
    close(store.file);
}

void enc_store_test_add_object(enc_store_test* store, char* tag) {
    size_t pos = store->obj_cnt;
    ++store->obj_cnt;

    if(store->objs == NULL) {
        store->objs = malloc(sizeof(enc_object));
        store->obj_offsets = malloc(sizeof(size_t));
        store->obj_reserved = 1;
    }
    else if (store->obj_cnt > store->obj_reserved) {
        store->obj_reserved *= 2;
        store->objs = realloc(store->objs, store->obj_reserved * sizeof(enc_object));
        store->obj_offsets = realloc(store->obj_offsets, store->obj_reserved * sizeof(size_t));
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

static size_t get_object_idx(enc_store_test store, char* tag) {
    enc_object* obj = NULL;
    size_t i;
    for(i = 0; i != store.obj_cnt; ++i) {
        obj = store.objs + i;
        if(strcmp(obj->tag, tag) == 0) break;
    }

    // unable to find object, actually handle this at some point
    if( obj == NULL ) {
        fprintf(stderr, "ERROR: Unable to find object \"%s\" in store.\n", tag);
        exit(1);
    }

    return i;
}

enc_grain_meta enc_store_test_get_meta(enc_store_test store, char* tag, char* key) {
    size_t obj_idx = get_object_idx(store, tag);
    enc_object* obj = store.objs + obj_idx;
    size_t offset = store.obj_offsets[obj_idx];

    int grain_pos = 0;
    // map the grain metadata
    // decrypt the grain metadata
    // return
    size_t encrypted_size = enc_get_encrypted_size(store.cfg, sizeof(enc_grain_meta));
    void* grain_meta_store = mmap_unaligned(store.file, encrypted_size, offset);
    enc_object_grain_meta_read(*obj, grain_pos, store.cfg, key, grain_meta_store);
    munmap_unaligned(grain_meta_store, encrypted_size, offset);

    return obj->grains[0];
}

void enc_store_test_set_meta(enc_store_test store, char* tag, enc_grain_meta meta) {
    size_t obj_idx = get_object_idx(store, tag);
    enc_object* obj = store.objs + obj_idx;
    // we already have a representative grain, oh no!
    if(obj->grain_cnt > 0) {
        fprintf(stderr, "ERROR: contig store object \"%s\" already has a backing grain.\n", tag);
        exit(1);
    }
    enc_object_add_grain(obj, meta);
}

void enc_store_test_read_object(enc_store_test store, char * tag, char* key, void* data) {
    size_t obj_idx = get_object_idx(store, tag);
    enc_object* obj = store.objs + obj_idx;
    size_t offset = store.obj_offsets[obj_idx];

    // TODO the sizes are incorrect, missing nonce/padding
    //      this also always reads into the data, won't merge multiple grains
    for(int grain_pos = 0; grain_pos != obj->grain_cnt; ++grain_pos) {
        // parse the grain meta
        offset += enc_get_encrypted_size(store.cfg, sizeof(enc_grain_meta));

        // use grain meta to read grain
        size_t data_size = enc_get_encrypted_size(store.cfg, obj->grains[grain_pos].size);
        void* data_store = mmap_unaligned(store.file, data_size, offset);
        enc_object_grain_data_read(*obj, grain_pos, key, data, data_store); 
        munmap_unaligned(data_store, data_size, offset);
    }
}

void enc_store_test_write_object(enc_store_test* store, char* tag, char* key, void* data) {
    size_t obj_idx = get_object_idx(*store, tag);
    enc_object* obj = store->objs + obj_idx;

    // for writing use and update the offset position
    size_t offset = store->cur_offset;
    store->obj_offsets[obj_idx] = offset;

    for(int grain_pos = 0; grain_pos != obj->grain_cnt; ++grain_pos) {
        // get sizes for write
        size_t meta_size = enc_get_encrypted_size(store->cfg, sizeof(enc_grain_meta));
        size_t total_size = meta_size + enc_get_encrypted_size(obj->grains[grain_pos].cfg, obj->grains[grain_pos].size);

        // allocate space in file
        size_t cur_file_size = lseek(store->file, 0, SEEK_END);
        ftruncate(store->file, cur_file_size + total_size);

        // map
        void* full_store = mmap_unaligned(store->file, total_size, offset);

        void* meta_store = full_store;
        void* data_store = full_store + meta_size;

        // write
        enc_object_grain_write(*obj, grain_pos, store->cfg, key, meta_store, data, data_store);

        //cleanup
        munmap_unaligned(full_store, total_size, offset);
        offset += total_size;
    }
    store->cur_offset = offset;
}

// nonce? lower layer
// how do we tell the size?
//  store in metadata? <- YES!
// but how do we get it? its straightforward enough to write it
// we need the size at the high levels to allocate
// where is the size known?
//  at the writing point after the encryption config is set
// to get the size we need
//  to know the configuration
//  to know how much we're writing
// these are known. want to avoid fully setting
// add function to query the encrypted size

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
