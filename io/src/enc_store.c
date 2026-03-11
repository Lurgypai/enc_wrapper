#include "enc_store.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>

#include "enc_wrapper.h"


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

// allocates memory, must be freed
static char* append_path(char* path, char* to_append) {
    size_t path_len = strlen(path);
    size_t append_len = strlen(to_append);

    // 2 for the "/" and "\0"
    char* ret = malloc(path_len + append_len + 2);
    strcpy(ret, path);
    ret[path_len] = '/';
    ret[path_len + 1] = '\0';
    strcat(ret, to_append);

    return ret;
}

enc_store enc_store_create(char* filename, enc_config cfg) {
    enc_store store = {
        .root_file = -1,
        .name = NULL,
        .cfg = cfg,
        .obj_cnt = 0,
        .obj_reserved = 0,
        .objs = NULL
    };

    mkdir(filename, 0777);
    char* root_file_name = append_path(filename, "root");
    store.root_file = open(root_file_name, O_RDWR | O_CREAT | O_TRUNC, 0644);
    free(root_file_name);

    if(store.root_file < 0) {
        perror("ERROR OPENING FILE IN enc_store_create");
        exit(1);
    }

    store.name = strdup(filename);

    return store;
}

enc_store enc_store_open(char* filename, char* key) {
    enc_store store = {
        .root_file = -1,
        .name = NULL,
        .cfg = {},
        .obj_cnt = 0,
        .obj_reserved = 0,
        .objs = NULL
    };

    // open file
    char* root_file_name = append_path(filename, "root");
    store.root_file = open(root_file_name, O_RDWR, 0644);
    free(root_file_name);

    store.name = strdup(filename);

    size_t offset = 0;
    // get encryption config
    read(store.root_file, &store.cfg, sizeof(store.cfg));
    offset += sizeof(store.cfg);

    // decrypt remaining blob
    enc_load_config(store.cfg);

    size_t file_size = lseek(store.root_file, 0, SEEK_END);
    size_t blob_size = file_size - offset - enc_get_nonce_size();
    void* blob_mem = malloc(blob_size);
    void* blob_store = mmap_unaligned(store.root_file, blob_size + enc_get_nonce_size(), offset);
    enc_set_key(key, enc_get_key_size());
    enc_set_nonce(blob_store, enc_get_nonce_size());
    enc_decrypt(blob_store + enc_get_nonce_size(), blob_size, blob_mem, blob_size);
    munmap_unaligned(blob_store, blob_size, offset);

    offset = 0;
    memcpy(&store.obj_cnt, blob_mem, sizeof(store.obj_cnt));
    offset += sizeof(store.obj_cnt);

    store.obj_reserved = store.obj_cnt;

    store.objs = malloc(sizeof(enc_object) * store.obj_cnt);
    for(int obj_idx = 0; obj_idx != store.obj_cnt; ++obj_idx) {
        enc_object_parse_meta(store.objs + obj_idx, blob_mem + offset);
        offset += enc_object_get_meta_size(store.objs[obj_idx]);
    }

    free(blob_mem);

    return store;
}

void enc_store_close(enc_store store, char* key) {
    // write cfg
    size_t offset = 0;
    lseek(store.root_file, 0, SEEK_SET);
    write(store.root_file, &store.cfg, sizeof(store.cfg));
    offset += sizeof(store.cfg);

    // add count to blob
    size_t obj_blob_size = sizeof(store.obj_cnt);
    void* obj_blob = malloc(obj_blob_size);
    memcpy(obj_blob, &store.obj_cnt, obj_blob_size);

    // add obj metadatas to blob
    for(int obj_idx = 0; obj_idx != store.obj_cnt; ++obj_idx) {
        size_t obj_size = enc_object_get_meta_size(store.objs[obj_idx]);
        obj_blob = realloc(obj_blob, obj_blob_size + obj_size);
        enc_object_put_meta(store.objs[obj_idx], obj_blob + obj_blob_size);
        obj_blob_size += obj_size;
    }

    size_t blk_size = enc_get_block_size();
    size_t remainder = obj_blob_size % blk_size;
    if(remainder != 0) {
        obj_blob_size += blk_size - remainder;
        obj_blob = realloc(obj_blob, obj_blob_size);
    }

    // allocate space in file for object metadatas
    size_t encrypted_size = enc_get_encrypted_size(store.cfg, obj_blob_size);
    ftruncate(store.root_file, offset + encrypted_size);
    void* meta_store = mmap_unaligned(store.root_file, encrypted_size, offset);

    // encrypt into file
    enc_load_config(store.cfg);
    enc_set_key(key, enc_get_key_size());
    char* nonce = enc_make_nonce();
    size_t nonce_size = enc_get_nonce_size();
    memcpy(meta_store, nonce, nonce_size);
    enc_set_nonce(nonce, nonce_size);
    enc_encrypt(obj_blob, obj_blob_size, meta_store + nonce_size, obj_blob_size);

    free(nonce);
    munmap_unaligned(meta_store, encrypted_size, offset);
    free(obj_blob);

    free(store.name);
    free(store.objs);
}

void enc_store_add_object(enc_store* store, char* tag) {
    size_t pos = store->obj_cnt;
    ++store->obj_cnt;

    if(store->objs == NULL) {
        store->objs = malloc(sizeof(enc_object));
        store->obj_reserved = 1;
    }
    else if (store->obj_cnt > store->obj_reserved) {
        store->obj_reserved *= 2;
        store->objs = realloc(store->objs, store->obj_reserved * sizeof(enc_object));
    }

    store->objs[pos] = enc_object_make(tag);
}

static size_t get_object_idx(enc_store store, char* tag) {
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

enc_object* enc_store_get_object(enc_store store, char* tag) {
    size_t obj_idx = get_object_idx(store, tag);
    return store.objs + obj_idx;
}
