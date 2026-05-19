#include "enc_store.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>

#ifdef ENABLE_MPI
#include <mpi.h>
#endif

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
static char* append_path(const char* path, char* to_append) {
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

enc_store enc_store_create(const char* filename, enc_config cfg) {
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
        .objs = NULL
    };

#ifdef ENABLE_MPI
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    if(my_rank == 0) {
#endif

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

    store.objs = malloc(sizeof(enc_object_desc) * store.obj_cnt);
    for(int obj_idx = 0; obj_idx != store.obj_cnt; ++obj_idx) {
        enc_object* obj = &store.objs[obj_idx].obj;
        // parse object meta
        enc_object_parse_meta(obj, blob_mem + offset);
        offset += enc_object_get_meta_size(*obj);
        memcpy(&store.objs[obj_idx].layout, blob_mem + offset, sizeof(enc_object_layout));
        offset += sizeof(enc_object_layout);

        // allocate empty idex
        obj->idx = enc_grain_index_make();
    }

    free(blob_mem);

#ifdef ENABLE_MPI
    }
    // basic store metadata
    MPI_Bcast(&store, sizeof(store), MPI_BYTE, 0, MPI_COMM_WORLD);

    // alocate space for objects
    size_t objs_size = sizeof(enc_object_desc) * store.obj_cnt;
    if(my_rank != 0) {
        store.name = strdup(filename);
        store.objs = malloc(objs_size);
    }

    // retrieve basic object meta
    MPI_Bcast(store.objs, objs_size, MPI_BYTE, 0, MPI_COMM_WORLD);

    // allocate space for grains and tag
    if(my_rank != 0) {
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
#endif

    return store;
}

void enc_store_close(enc_store store, char* key) {
#ifdef ENABLE_MPI
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    if (my_rank != 0) return;
#endif

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
        size_t obj_size = enc_object_get_meta_size(store.objs[obj_idx].obj);
        // alocate space for object and layout
        obj_blob = realloc(obj_blob, obj_blob_size + obj_size + sizeof(enc_object_layout));
        // copy obj
        enc_object_put_meta(store.objs[obj_idx].obj, obj_blob + obj_blob_size);
        obj_blob_size += obj_size;
        // copy layout
        memcpy(obj_blob + obj_blob_size, &store.objs[obj_idx].layout, sizeof(enc_object_layout));
        obj_blob_size += sizeof(enc_object_layout);
        // free object
        enc_object_free(&store.objs[obj_idx].obj);
    }

    enc_load_config(store.cfg);
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

enc_object* enc_store_get_object(enc_store store, const char* tag) {
    size_t obj_idx = get_object_idx(store, tag);
    return &store.objs[obj_idx].obj;
}

static void write_index_joined(enc_config cfg, char* name, int object_idx, enc_object obj, char* key) {
    // get filename
    // generate meta blob
    // mmap and encrypt into
    // 10 digit integer + "-g\0"
    char* grains_filename = malloc(10 + 3);
    grains_filename[0] = '\0';
    sprintf(grains_filename, "%d-g", object_idx);

    char* filename = append_path(name, grains_filename);
    int file = open(filename, O_RDWR | O_CREAT, 0644);
    free(grains_filename);

    enc_load_config(cfg);
    enc_set_key(key, enc_get_key_size());

    size_t blob_size = sizeof(enc_grain_index_desc) * obj.grain_cnt;
    size_t encrypted_size = enc_get_encrypted_size(cfg, blob_size);

    ftruncate(file, encrypted_size);

    void* dest = mmap_unaligned(file, encrypted_size, 0);

    size_t nonce_size = enc_get_nonce_size();
    char* nonce = enc_make_nonce();
    enc_set_nonce(nonce, enc_get_nonce_size());
    memcpy(dest, nonce, nonce_size);
    enc_encrypt(obj.idx.grains, blob_size, dest + nonce_size, blob_size);

    free(nonce);
    munmap_unaligned(dest, encrypted_size, 0);
    free(filename);
}

void enc_store_index_write(enc_store store, const char* tag, char* key) {
#ifdef ENABLE_MPI
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    if(my_rank != 0) return;
#endif

    size_t obj_idx = get_object_idx(store, tag);
    enc_object_desc* obj = &store.objs[obj_idx];
    switch(obj->layout) {
        case enc_object_layout_joined:
            write_index_joined(store.cfg, store.name, obj_idx, obj->obj, key);
            break;
    }
}

static void read_index_joined(enc_config cfg, const char* name, int object_idx, enc_object obj, char* key) {
    char* grains_filename = malloc(10 + 3);
    grains_filename[0] = '\0';
    sprintf(grains_filename, "%d-g", object_idx);

    char * filename = append_path(name, grains_filename);
    int file = open(filename, O_RDWR, 0644);
    free(grains_filename);

    enc_load_config(cfg);
    enc_set_key(key, enc_get_key_size());

    size_t blob_size = sizeof(enc_grain_index_desc) * obj.grain_cnt;
    size_t encrypted_size = enc_get_encrypted_size(cfg, blob_size);

    void* src = mmap_unaligned(file, encrypted_size, 0);

    size_t nonce_size = enc_get_nonce_size();
    char* nonce = malloc(nonce_size);
    memcpy(nonce, src, nonce_size);
    enc_set_nonce(nonce, nonce_size);

    enc_decrypt(src + nonce_size, blob_size, obj.idx.grains, blob_size);

    // generate index
    size_t cur_offset;
    for(int grain_idx = 0; grain_idx != obj.grain_cnt; ++grain_idx) {
        // enc_grain_index_add_grain(&obj.idx, grain_idx, cur_offset, obj.grains[grain_idx].size);
    } 

    free(nonce);
    munmap_unaligned(src, encrypted_size, 0);
    free(filename);
}

void enc_store_index_read(enc_store store, const char* tag, char* key) {
    size_t obj_idx = get_object_idx(store, tag);
    enc_object_desc* obj = &store.objs[obj_idx];

#ifdef ENABLE_MPI
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    if(my_rank == 0) {
#endif

    switch(obj->layout) {
        case enc_object_layout_joined:
            read_index_joined(store.cfg, store.name, obj_idx, obj->obj, key);
            break;
    }

#ifdef ENABLE_MPI
    }
    // set the count (and reserve) alloc, and load
    MPI_Bcast(&obj->obj.idx.cnt, sizeof(obj->obj.idx.cnt), MPI_BYTE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&obj->obj.idx.reserved, sizeof(obj->obj.idx.reserved), MPI_BYTE, 0, MPI_COMM_WORLD);
    size_t index_size = sizeof(enc_grain_index_desc) * obj->obj.idx.cnt;
    if(my_rank != 0) obj->obj.idx.grains = malloc(index_size);
    MPI_Bcast(&obj->obj.idx.grains, sizeof(index_size), MPI_BYTE, 0, MPI_COMM_WORLD);
#endif
}

static void map_grain(enc_store store, int obj_idx, int grain_idx, enc_grain_meta grain, void** data_out, int* file_out) {
    enc_object_desc* obj = store.objs + obj_idx;
    // enc_grain_meta* grain = obj->obj.grains + grain_idx;
    // 10 digits per integer, - and null terminator
    char* grain_filename = malloc(22);
    sprintf(grain_filename, "%d-%d", obj_idx, grain_idx);
    char* filename = append_path(store.name, grain_filename);
    free(grain_filename);

    *file_out = open(filename, O_RDWR | O_CREAT, 0644);
    if(*file_out < 0) { 
        perror("FAILED TO OPEN GRAIN FILE");
        exit(1);
    }

    // size_t encrypted_size = enc_get_encrypted_size(grain->cfg, grain->size);
    // ftruncate(*file_out, encrypted_size);

    // *data_out = mmap(NULL, encrypted_size, PROT_READ | PROT_WRITE, MAP_SHARED, *file_out, 0);
    if(*data_out == MAP_FAILED) {
        perror("FAILED TO MAP GRAIN");
        exit(1);
    }

    free(filename);
}

// TODO
// needs to be fixed, passed grain meta
static void unmap_grain(enc_store store, int obj_idx, int grain_idx, void* data, int file) {
    enc_object_desc* obj = store.objs + obj_idx;
    // enc_grain_meta* grain = obj->obj.grains + grain_idx;
    // munmap(data, grain->size);
    close(file);
}

// returns local io size
// in_data has to be the data we want to write to this grain
static size_t write_to_grain(enc_store store, int obj_idx, int grain_idx,
                            size_t offset, size_t grain_offset, size_t remaining_size,
                            const void* in_data, char* key) {
    enc_object* obj = &store.objs[obj_idx].obj;
    enc_grain_meta* grain = &obj->grains[grain_idx];
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
    map_grain(store, obj_idx, grain_idx, &data_store, &file);

    // cast away the const to silence errors.
    // we don't modify this, but we might replace it, and need to free it
    void* data = (void*)in_data;
    int unaligned = local_offset != 0 || local_remaining_size != grain->size;
    if(unaligned) {
            data = malloc(grain->size);
            enc_object_grain_data_read(*obj, grain_idx, key, data, data_store);
            memcpy(data + local_offset, in_data, local_remaining_size);
    }

    enc_grain_data_write(*grain, data_store, data, key);

    unmap_grain(store, obj_idx, grain_idx, data_store, file);
    if(unaligned) free(data);

    return local_remaining_size;
}


static size_t read_from_grain(enc_store store, int obj_idx, int grain_idx,
                            size_t offset, size_t grain_offset, size_t remaining_size,
                            void* out_data, char* key) {
    enc_object* obj = &store.objs[obj_idx].obj;
    enc_grain_meta* grain = &obj->grains[grain_idx];
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
    map_grain(store, obj_idx, grain_idx, &data_store, &file);
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

    unmap_grain(store, obj_idx, grain_idx, data_store, file);
    return local_remaining_size;
}


void do_io(int io_dir, enc_store store, const char* tag, size_t offset, size_t size, void* data, char* key) {
    if(size == 0) return;
    size_t obj_idx = get_object_idx(store, tag);
    enc_object_desc* obj = store.objs + obj_idx;

    size_t* selected_grains;
    size_t selected_grains_size;
    enc_grain_index_select_grains(&obj->obj.idx, offset, size, &selected_grains, &selected_grains_size);

    size_t remaining_size = size;

    for(int selected_grain_idx = 0; selected_grain_idx != selected_grains_size; ++selected_grain_idx) {
        int grain_idx = selected_grains[selected_grain_idx];
        enc_grain_meta* cur_grain = obj->obj.grains + grain_idx;

        size_t io_performed = 0;
        if(io_dir == 0) {
            io_performed = write_to_grain(store, obj_idx, grain_idx,
                    offset, obj->obj.idx.grains[grain_idx].offset, remaining_size, data, key);
        } else if (io_dir == 1) {
            io_performed = read_from_grain(store, obj_idx, grain_idx,
                    offset, obj->obj.idx.grains[grain_idx].offset, remaining_size, data, key);
        }

        remaining_size -= io_performed;
        if(remaining_size == 0) break;
        data += io_performed;
    }
}

void enc_store_write(enc_store store, const char* tag, size_t offset, size_t size, const void* in_data, char* key) {
    do_io(0, store, tag, offset, size, (void*)in_data, key);
}

void enc_store_read(enc_store store, const char* tag, size_t offset, size_t size, void* out_data, char* key) {
    do_io(1, store, tag, offset, size, out_data, key);
}
