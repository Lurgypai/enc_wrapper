#pragma once

#include <stdint.h>

#include "enc_config.h"

// metadata for a grain
typedef struct enc_grain_meta_ {
    int64_t size;
    enc_config cfg;
} enc_grain_meta;

// encryption configuration for metadata must be set before IO
// we read meta and data separately because users will need to know the size to allocate buffers
enc_grain_meta enc_grain_meta_read(void* meta_store, char* key);
void enc_grain_data_read(enc_grain_meta meta, void* data_store, void* data_mem, char* key);

void enc_grain_write(enc_grain_meta meta, void* meta_store, void* data_mem, void* data_store, char* key);

// in the current implementation, meta and data are written and read (and thus encrypted and unencrypted) separately. this means additional overhead for each. might be useful to add functions that read/write them together (alternate layout that supports contiguous)
// WRONG: meta and data will use different encryption configs (meta uses config for reading all meta, while data uses config defined in metadata)

// next
// separate meta read and write
