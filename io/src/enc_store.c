#include "enc_store.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>

#include <mpi.h>

#include "enc_init.h"
#include "enc_util.h"
#include "enc_wrapper.h"


static size_t get_object_idx(enc_store store, const char* tag);

static void add_grain_joined(enc_store* store, size_t obj_idx, enc_grain_meta grain, char* key);
static void flush_cached_grains(enc_store* store, char* key);
static void cache_grains(enc_store* store, size_t obj_idx, char* key);
static void write_joined_obj_grain_meta(enc_store* store, char* key);

enc_store enc_store_create(const char* filename, enc_config cfg) {
    enc_store store = {
        .root_file = -1,
        .name = NULL,
        .cfg = cfg,
        .obj_cnt = 0,
        .obj_reserved = 0,
        .objs = NULL,
        .cur_joined_obj = NULL,
        .joined_obj_grains = NULL,
        .joined_obj_reserved = 0
    };

    mkdir(filename, 0777);
    char* root_file_name = append_path(filename, "root");
    store.root_file = open(root_file_name, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if(store.root_file < 0) {
        perror("OPEN");
        fprintf(stderr, "enc_store_create\n");
        fprintf(stderr, "Filename: %s\n", root_file_name);
    }
    free(root_file_name);

    if(store.root_file < 0) {
        perror("ERROR OPENING FILE IN enc_store_create");
        exit(1);
        exit(1);
    }

    store.name = strdup(filename);

    return store;
}

enc_store enc_store_open(const char* filename, char* key) {
    enc_store store = {
        .root_file = -1,
        .name = NULL,
        .cfg = {},
        .obj_cnt = 0,
        .obj_reserved = 0,
        .objs = NULL,
        .cur_joined_obj = NULL,
        .joined_obj_grains = NULL,
        .joined_obj_reserved = 0
    };

    if(ENC_RANK_G == 0) {
        // open file
        char* root_file_name = append_path(filename, "root");
        store.root_file = open(root_file_name, O_RDWR, 0644);
        if(store.root_file < 0) {
            perror("OPEN");
            fprintf(stderr, "enc_store_open\n");
            fprintf(stderr, "Filename: %s\n", root_file_name);
        }
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

        store.objs = malloc(sizeof(enc_object_desc) * store.obj_cnt);
        for(int obj_idx = 0; obj_idx != store.obj_cnt; ++obj_idx) {
            enc_object_desc* desc = &store.objs[obj_idx];
            enc_object* obj = &desc->obj;
            // parse object meta
            enc_object_parse_meta(obj, blob_mem + offset);
            offset += enc_object_get_meta_size(*obj);
            memcpy(&desc->layout, blob_mem + offset, sizeof(enc_object_layout));
            offset += sizeof(enc_object_layout);

            // allocate empty idex
            obj->idx = enc_grain_index_make();
        }

        free(blob_mem);

    }
    // basic store metadata
    MPI_Bcast(&store, sizeof(store), MPI_BYTE, 0, MPI_COMM_WORLD);

    // alocate space for objects
    size_t objs_size = sizeof(enc_object_desc) * store.obj_cnt;
    if(ENC_RANK_G != 0) {
        store.name = strdup(filename);
        store.objs = malloc(objs_size);
    }

    // retrieve basic object meta
    MPI_Bcast(store.objs, objs_size, MPI_BYTE, 0, MPI_COMM_WORLD);

    // allocate space for grains and tag
    if(ENC_RANK_G != 0) {
        for(int obj_idx = 0; obj_idx != store.obj_cnt; ++obj_idx) {
            enc_object* obj = &store.objs[obj_idx].obj;
            obj->tag = malloc(obj->tag_size + 1);
        }
    }

    // set tags
    for(int obj_idx = 0; obj_idx != store.obj_cnt; ++obj_idx) {
        enc_object* obj = &store.objs[obj_idx].obj;
        MPI_Bcast(obj->tag, obj->tag_size, MPI_BYTE, 0, MPI_COMM_WORLD);
        obj->tag[obj->tag_size] = '\0';
    }

    return store;
}

void enc_store_close(enc_store* store, char* key) {
    if (ENC_RANK_G != 0) return;

    // flush cached grains
    flush_cached_grains(store, key);

    // write cfg
    size_t offset = 0;
    lseek(store->root_file, 0, SEEK_SET);
    write(store->root_file, &store->cfg, sizeof(store->cfg));
    offset += sizeof(store->cfg);

    // add count to blob
    size_t obj_blob_size = sizeof(store->obj_cnt);
    void* obj_blob = malloc(obj_blob_size);
    memcpy(obj_blob, &store->obj_cnt, obj_blob_size);

    // add obj metadatas to blob
    for(int obj_idx = 0; obj_idx != store->obj_cnt; ++obj_idx) {
        size_t obj_size = enc_object_get_meta_size(store->objs[obj_idx].obj);
        // alocate space for object and layout
        obj_blob = realloc(obj_blob, obj_blob_size + obj_size + sizeof(enc_object_layout));
        // copy obj
        enc_object_put_meta(store->objs[obj_idx].obj, obj_blob + obj_blob_size);
        obj_blob_size += obj_size;
        // copy layout
        memcpy(obj_blob + obj_blob_size, &store->objs[obj_idx].layout, sizeof(enc_object_layout));
        obj_blob_size += sizeof(enc_object_layout);
        // free object
    }

    enc_load_config(store->cfg);
    size_t blk_size = enc_get_block_size();
    size_t remainder = obj_blob_size % blk_size;
    if(remainder != 0) {
        obj_blob_size += blk_size - remainder;
        obj_blob = realloc(obj_blob, obj_blob_size);
    }

    // allocate space in file for object metadatas
    size_t encrypted_size = enc_get_encrypted_size(store->cfg, obj_blob_size);
    ftruncate(store->root_file, offset + encrypted_size);
    void* meta_store = mmap_unaligned(store->root_file, encrypted_size, offset);

    // encrypt into file
    enc_set_key(key, enc_get_key_size());
    char* nonce = enc_make_nonce();
    size_t nonce_size = enc_get_nonce_size();
    memcpy(meta_store, nonce, nonce_size);
    enc_set_nonce(nonce, nonce_size);
    enc_encrypt(obj_blob, obj_blob_size, meta_store + nonce_size, obj_blob_size);

    free(nonce);
    munmap_unaligned(meta_store, encrypted_size, offset);
    free(obj_blob);

    free(store->name);
    free(store->objs);

    free(store->joined_obj_grains);
}

void enc_store_add_object(enc_store* store, const char* tag, enc_object_layout layout) {
    size_t pos = store->obj_cnt;
    ++store->obj_cnt;

    if(store->objs == NULL) {
        store->objs = malloc(sizeof(enc_object_desc));
        store->obj_reserved = 1;
    }
    else if (store->obj_cnt > store->obj_reserved) {
        store->obj_reserved *= 2;
        store->objs = realloc(store->objs, store->obj_reserved * sizeof(enc_object_desc));
    }

    store->objs[pos].obj = enc_object_make(tag);
    store->objs[pos].layout = layout;
}


enc_object* enc_store_get_object(enc_store store, const char* tag) {
    size_t obj_idx = get_object_idx(store, tag);
    return &store.objs[obj_idx].obj;
}


void enc_store_add_grain(enc_store* store, const char* tag, enc_grain_meta grain, char* key) {
    size_t obj_idx = get_object_idx(*store, tag);
    enc_object_desc* desc = &store->objs[obj_idx];
    enc_object* obj = &desc->obj;

    switch(desc->layout) {
        case enc_object_layout_joined:
            add_grain_joined(store, obj_idx, grain, key);
            break;
        case enc_object_layout_split:
            break;
    }

    enc_object_add_grain(obj, grain);
}

void enc_store_index_write(enc_store* store, const char* tag, char* key) {
    if(ENC_RANK_G != 0) return;

    size_t object_idx = get_object_idx(*store, tag);
    enc_object_desc* obj = &store->objs[object_idx];

    char* index_filename = malloc(10 + 3);
    index_filename[0] = '\0';
    sprintf(index_filename, "%lu-i", object_idx);

    char* filename = append_path(store->name, index_filename);
    int file = open(filename, O_RDWR | O_CREAT, 0644);
    if(file < 0) {
        perror("OPEN");
        fprintf(stderr, "enc_store_index_write\n");
        fprintf(stderr, "Filename: %s\n", filename);
    }
    free(index_filename);

    enc_load_config(store->cfg);
    enc_set_key(key, enc_get_key_size());

    size_t blob_size = sizeof(enc_grain_index_desc) * obj->obj.grain_cnt;
    size_t encrypted_size = enc_get_encrypted_size(store->cfg, blob_size);

    ftruncate(file, encrypted_size);

    void* dest = mmap_unaligned(file, encrypted_size, 0);

    size_t nonce_size = enc_get_nonce_size();
    char* nonce = enc_make_nonce();
    enc_set_nonce(nonce, enc_get_nonce_size());
    memcpy(dest, nonce, nonce_size);

    enc_encrypt(obj->obj.idx.grains, blob_size, dest + nonce_size, blob_size);

    free(nonce);
    munmap_unaligned(dest, encrypted_size, 0);
    free(filename);
}

void enc_store_index_read(enc_store* store, const char* tag, char* key) {
    size_t object_idx = get_object_idx(*store, tag);
    enc_object_desc* obj = &store->objs[object_idx];

    if(ENC_RANK_G == 0) {
        char* index_filename = malloc(10 + 3);
        index_filename[0] = '\0';
        sprintf(index_filename, "%lu-i", object_idx);

        char * filename = append_path(store->name, index_filename);
        int file = open(filename, O_RDWR, 0644);
        if(file < 0) {
            perror("OPEN");
            fprintf(stderr, "Rank: %d\n", ENC_RANK_G);
            fprintf(stderr, "enc_store_index_read\n");
            fprintf(stderr, "Filename: %s\n", filename);
        }
        free(index_filename);

        enc_load_config(store->cfg);
        enc_set_key(key, enc_get_key_size());

        size_t blob_size = sizeof(enc_grain_index_desc) * obj->obj.grain_cnt;
        obj->obj.idx.cnt = obj->obj.grain_cnt;
        obj->obj.idx.grains = malloc(blob_size);
        obj->obj.idx.reserved = obj->obj.grain_cnt;
        size_t encrypted_size = enc_get_encrypted_size(store->cfg, blob_size);

        void* src = mmap_unaligned(file, encrypted_size, 0);

        size_t nonce_size = enc_get_nonce_size();
        char* nonce = malloc(nonce_size);
        memcpy(nonce, src, nonce_size);
        enc_set_nonce(nonce, nonce_size);

        enc_decrypt(src + nonce_size, blob_size, obj->obj.idx.grains, blob_size);

        free(nonce);
        munmap_unaligned(src, encrypted_size, 0);
        free(filename);
    }
    // set the count (and reserve) alloc, and load
    MPI_Bcast(&obj->obj.idx.cnt, sizeof(obj->obj.idx.cnt), MPI_BYTE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&obj->obj.idx.reserved, sizeof(obj->obj.idx.reserved), MPI_BYTE, 0, MPI_COMM_WORLD);
    size_t index_size = sizeof(enc_grain_index_desc) * obj->obj.idx.cnt;
    if(ENC_RANK_G != 0) obj->obj.idx.grains = malloc(index_size);
    MPI_Bcast(&obj->obj.idx.grains, sizeof(index_size), MPI_BYTE, 0, MPI_COMM_WORLD);
}




// returns local io size
// in_data has to be the data we want to write to this grain
static size_t write_to_grain(enc_store* store, int obj_idx, int grain_idx, enc_grain_meta* grain,
                            size_t offset, size_t grain_offset, size_t remaining_size,
                            const void* in_data, char* key) {
    enc_object* obj = &store->objs[obj_idx].obj;
    // offset into grain
    // if the offset is before this grain, write at start of grain
    // else set to how far into the grain the offset is
    size_t local_offset = offset < grain_offset ? 0 : offset - grain_offset;
    // the local remaining is either the rest of the grain or however much io remains
    size_t local_remaining_size = remaining_size < grain->size - local_offset ?
        remaining_size : grain->size - local_offset;

    /*
    printf("DEBUG: writing to grain\n");
    printf("\tobj_idx: %d, grain_idx: %d\n", obj_idx, grain_idx);
    printf("\toffset: %lu, grain_offset: %lu, remaining_size: %lu\n", offset, grain_offset, remaining_size);
    printf("\tlocal_offset: %lu, local_remaining_size: %lu\n", local_offset, local_remaining_size);
    */

    void* data_store = NULL;
    int file = 0;
    map_grain(store, obj_idx, grain_idx, grain, &data_store, &file);

    // cast away the const to silence errors.
    // we don't modify this, but we might replace it, and need to free it
    void* data = (void*)in_data;
    int unaligned = local_offset != 0 || local_remaining_size != grain->size;
    if(unaligned) {
            data = malloc(grain->size);
            enc_object_grain_read(*obj, *grain, data, data_store, key);
            memcpy(data + local_offset, in_data, local_remaining_size);
    }

    enc_grain_data_write(*grain, data_store, data, key);

    unmap_grain(store, obj_idx, grain_idx, grain, data_store, file);
    if(unaligned) free(data);

    return local_remaining_size;
}


static size_t read_from_grain(enc_store* store, int obj_idx, int grain_idx, enc_grain_meta* grain,
                            size_t offset, size_t grain_offset, size_t remaining_size,
                            void* out_data, char* key) {
    enc_object* obj = &store->objs[obj_idx].obj;
    size_t local_offset = offset < grain_offset ? 0 : offset - grain_offset;
    size_t local_remaining_size = remaining_size < grain->size - local_offset ?
        remaining_size : grain->size - local_offset;
    /*
    printf("DEBUG: reading from grain\n");
    printf("\tobj_idx: %d, grain_idx: %d\n", obj_idx, grain_idx);
    printf("\toffset: %lu, grain_offset: %lu, remaining_size: %lu\n", offset, grain_offset, remaining_size);
    printf("\tlocal_offset: %lu, local_remaining_size: %lu\n", local_offset, local_remaining_size);
    */

    void* data_store = NULL;
    int file = 0;
    map_grain(store, obj_idx, grain_idx, grain, &data_store, &file);
    int unaligned = local_offset != 0 || local_remaining_size != grain->size;

    if(unaligned) {
        // read locally, copy necessary
        void* data_mem = malloc(grain->size);
        enc_grain_data_read(*grain, data_store, data_mem, key);
        memcpy(out_data, data_mem + local_offset, local_remaining_size);
        free(data_mem);
    }
    else {
        enc_grain_data_read(*grain, data_store, out_data, key);
    }

    unmap_grain(store, obj_idx, grain_idx, grain, data_store, file);
    return local_remaining_size;
}

static enc_grain_meta read_grain_meta_joined(enc_store* store, size_t obj_idx, enc_object* obj,
        size_t grain_idx, char* key) {

    if(store->cur_joined_obj != NULL &&
            strcmp(store->cur_joined_obj, obj->tag) == 0) {
        return store->joined_obj_grains[grain_idx];
    }

    cache_grains(store, obj_idx, key);

    enc_grain_meta grain;
    memcpy(&grain, store->joined_obj_grains + grain_idx, sizeof(grain));
    return grain;
}

// TODO
// complete
static enc_grain_meta read_grain_meta_split(enc_store* store, size_t obj_idx, enc_object* obj, size_t grain_idx, char* key) {
    enc_grain_meta ret;
    return ret;
}

static enc_grain_meta read_grain_meta(enc_store* store, enc_object_layout layout, size_t obj_idx, enc_object* obj, size_t grain_idx, char* key) {
    switch(layout) {
        case enc_object_layout_joined:
            return read_grain_meta_joined(store, obj_idx, obj, grain_idx, key);
        case enc_object_layout_split:
            return read_grain_meta_split(store, obj_idx, obj, grain_idx, key);
    }
}


void do_io(int io_dir, enc_store* store, const char* tag, size_t offset, size_t size, void* data, char* key) {
    if(size == 0) return;
    size_t obj_idx = get_object_idx(*store, tag);
    enc_object_desc* obj = store->objs + obj_idx;

    size_t* selected_grains;
    size_t selected_grains_size;
    enc_grain_index_select_grains(&obj->obj.idx, offset, size, &selected_grains, &selected_grains_size);

    size_t remaining_size = size;

    for(int selected_grain_idx = 0; selected_grain_idx != selected_grains_size; ++selected_grain_idx) {
        int grain_idx = selected_grains[selected_grain_idx];
        // read grain meta
         enc_grain_meta cur_grain = read_grain_meta(store, obj->layout, obj_idx, &obj->obj, grain_idx, key);

        // perform io
        size_t io_performed = 0;
        if(io_dir == 0) {
            io_performed = write_to_grain(store, obj_idx, grain_idx, &cur_grain,
                    offset, obj->obj.idx.grains[grain_idx].offset, remaining_size, data, key);
        } else if (io_dir == 1) {
            io_performed = read_from_grain(store, obj_idx, grain_idx, &cur_grain,
                    offset, obj->obj.idx.grains[grain_idx].offset, remaining_size, data, key);
        }

        remaining_size -= io_performed;
        if(remaining_size == 0) break;
        data += io_performed;
    }
}

void enc_store_write(enc_store* store, const char* tag, size_t offset, size_t size, const void* in_data, char* key) {
    do_io(0, store, tag, offset, size, (void*)in_data, key);
}

void enc_store_read(enc_store* store, const char* tag, size_t offset, size_t size, void* out_data, char* key) {
    do_io(1, store, tag, offset, size, out_data, key);
}



static size_t get_object_idx(enc_store store, const char* tag) {
    enc_object* obj = NULL;
    size_t i;
    for(i = 0; i != store.obj_cnt; ++i) {
        obj = &store.objs[i].obj;
        if(strcmp(obj->tag, tag) == 0) break;
    }

    // unable to find object, actually handle this at some point
    if( obj == NULL ) {
        fprintf(stderr, "ERROR: Unable to find object \"%s\" in store.\n", tag);
        exit(1);
    }

    return i;
}

static void add_grain_joined(enc_store* store, size_t obj_idx, enc_grain_meta grain, char * key) {
    enc_object_desc* desc = &store->objs[obj_idx];
    enc_object* obj = &desc->obj;

    // incorrect object loaded, retrieve (will flush)
    if(store->cur_joined_obj != NULL &&
            strcmp(store->cur_joined_obj, obj->tag) != 0) {
        cache_grains(store, obj_idx, key);
    }

    store->cur_joined_obj = obj->tag;

    if(store->joined_obj_reserved == 0) {
        store->joined_obj_reserved = 1;
        store->joined_obj_grains = malloc(sizeof(enc_grain_meta));
    }
    else if (obj->grain_cnt == store->joined_obj_reserved) {
        store->joined_obj_reserved *= 2;
        store->joined_obj_grains =
            realloc(store->joined_obj_grains, sizeof(enc_grain_meta) * store->joined_obj_reserved);
    }
    size_t grain_idx = obj->grain_cnt;
    store->joined_obj_grains[grain_idx].cfg = grain.cfg;
    store->joined_obj_grains[grain_idx].size = grain.size;
}


static void flush_cached_grains(enc_store* store, char* key) {
    if(ENC_RANK_G != 0) return;

    if(store->cur_joined_obj != NULL) write_joined_obj_grain_meta(store, key);
}

static void cache_grains(enc_store* store, size_t obj_idx, char* key) {
    /*
     * all ranks
     *  check if currently loaded object is us
     *  if it is return early
     *  else
     *      flush if necessary
     * rank 0
     *  load
     * all ranks
     *  bcast
     */

    enc_object* obj = &store->objs[obj_idx].obj;

    if(store->cur_joined_obj != NULL) {
        if(strcmp(obj->tag, store->cur_joined_obj) == 0) {
            // object is cached return
            return;
        }
        else {
            // unload current object
            flush_cached_grains(store, key);

            store->cur_joined_obj = obj->tag;
            size_t new_reserved = sizeof(enc_grain_meta) * obj->grain_cnt;
            if(new_reserved > store->joined_obj_reserved * sizeof(enc_grain_meta)) {
                store->joined_obj_grains = realloc(store->joined_obj_grains, new_reserved);
                store->joined_obj_reserved = obj->grain_cnt;
            }
        }
    }
    else {
        //allocate
        store->cur_joined_obj = obj->tag;
        store->joined_obj_reserved = obj->grain_cnt;
        store->joined_obj_grains = malloc(store->joined_obj_reserved * sizeof(enc_grain_meta));
    }

    // load the grains into the cache
    size_t blob_size = 0;
    if(ENC_RANK_G == 0) {
        char* index_filename = malloc(10 + 3);
        index_filename[0] = '\0';
        sprintf(index_filename, "%lu-g", obj_idx);

        char * filename = append_path(store->name, index_filename);
        int file = open(filename, O_RDWR, 0644);
        free(index_filename);
        free(filename);

        if(file > 0) {
            enc_load_config(store->cfg);
            enc_set_key(key, enc_get_key_size());

            blob_size = obj->grain_cnt * sizeof(enc_grain_meta);
            size_t encrypted_size = enc_get_encrypted_size(store->cfg, blob_size);

            void* src = mmap_unaligned(file, encrypted_size, 0);

            size_t nonce_size = enc_get_nonce_size();
            char* nonce = malloc(nonce_size);
            memcpy(nonce, src, nonce_size);
            enc_set_nonce(nonce, nonce_size);

            enc_decrypt(src + nonce_size, blob_size, store->joined_obj_grains, blob_size);

            free(nonce);
            munmap_unaligned(src, encrypted_size, 0);
        }
    }
    if(blob_size > 0) MPI_Bcast(&store->joined_obj_grains, blob_size, MPI_BYTE, 0, MPI_COMM_WORLD);
}

static void write_joined_obj_grain_meta(enc_store* store, char* key) {
    if(ENC_RANK_G != 0) return;

    size_t object_idx = get_object_idx(*store, store->cur_joined_obj);
    enc_object* obj = &store->objs[object_idx].obj;
    
    char* grains_filename = malloc(10 + 3);
    grains_filename[0] = '\0';
    sprintf(grains_filename, "%lu-g", object_idx);

    char* filename = append_path(store->name, grains_filename);
    int file = open(filename, O_RDWR | O_CREAT, 0644);
    if(file < 0) {
        perror("OPEN");
        fprintf(stderr, "Filename: %s\n", filename);
    }
    free(grains_filename);

    enc_load_config(store->cfg);
    enc_set_key(key, enc_get_key_size());

    size_t blob_size = sizeof(enc_grain_meta) * obj->grain_cnt;
    size_t encrypted_size = enc_get_encrypted_size(store->cfg, blob_size);

    ftruncate(file, encrypted_size);

    void* dest = mmap_unaligned(file, encrypted_size, 0);

    size_t nonce_size = enc_get_nonce_size();
    char* nonce = enc_make_nonce();
    enc_set_nonce(nonce, enc_get_nonce_size());
    memcpy(dest, nonce, nonce_size);
    enc_encrypt(store->joined_obj_grains, blob_size, dest + nonce_size, blob_size);

    free(nonce);
    munmap_unaligned(dest, encrypted_size, 0);
    free(filename);
}

