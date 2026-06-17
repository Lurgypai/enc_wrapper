#pragma once
#include "enc_store.h"

// mmaping utils
void* mmap_unaligned(int fid, size_t size, size_t offset);
void munmap_unaligned(void* ptr, size_t size, size_t offset);
void map_grain(enc_store* store, int obj_idx, int grain_idx,
        enc_grain_meta* grain, void** data_out, int* file_out);
void unmap_grain(enc_store* store, int obj_idx, int grain_idx, void* data, int file);

// pathing utils
// allocates memory, must be freed
char* append_path(const char* path, char* to_append);
