#include "enc_util.h"

#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "enc_wrapper.h"

void* mmap_unaligned(int fd, size_t size, size_t offset) {
    long page_size = sysconf(_SC_PAGESIZE);

    // get page aligned offset
    size_t page_offset = (offset / page_size) * page_size;
    // get offset into output ptr
    size_t sub_offset = offset % page_size;

    void* raw_map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_offset);
    if(raw_map == MAP_FAILED) perror("ERROR MMAPING");
    return raw_map + sub_offset;
}

void munmap_unaligned(void* ptr, size_t size, size_t offset) {
    long page_size = sysconf(_SC_PAGESIZE);
    size_t sub_offset = offset % page_size;

    void* raw_map = ptr - sub_offset;
    int err = munmap(raw_map, size);
    if(err < 0) perror("ERROR UNMAPPING");
}

void map_grain(enc_store* store, int obj_idx, int grain_idx, enc_grain_meta* grain, void** data_out, int* file_out) {
    enc_object_desc* obj = store->objs + obj_idx;
    // enc_grain_meta* grain = obj->obj.grains + grain_idx;
    // 10 digits per integer, - and null terminator
    char* grain_filename = malloc(22);
    sprintf(grain_filename, "%d-%d", obj_idx, grain_idx);
    char* filename = append_path(store->name, grain_filename);
    free(grain_filename);

    *file_out = open(filename, O_RDWR | O_CREAT, 0644);
    if(*file_out < 0) { 
        perror("FAILED TO OPEN GRAIN FILE");
        exit(1);
    }

    size_t encrypted_size = enc_get_encrypted_size(grain->cfg, grain->size);
    ftruncate(*file_out, encrypted_size);

    *data_out = mmap(NULL, encrypted_size, PROT_READ | PROT_WRITE, MAP_SHARED, *file_out, 0);
    if(*data_out == MAP_FAILED) {
        perror("FAILED TO MAP GRAIN");
        exit(1);
    }

    free(filename);
}

// TODO
// needs to be fixed, passed grain meta
void unmap_grain(enc_store* store, int obj_idx, int grain_idx, enc_grain_meta* grain, void* data, int file) {
    enc_object_desc* obj = store->objs + obj_idx;
    munmap(data, grain->size);
    close(file);
}

char* append_path(const char* path, char* to_append) {
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
