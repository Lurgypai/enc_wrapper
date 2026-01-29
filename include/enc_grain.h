#pragma once
#include <stdint.h>
#include "enc_wrapper.h"

// metadata for a grain
typedef struct enc_grain_ {
    // storage stats
    int64_t size;
    // encryption config stats
    enc_library lib;
    enc_algorithm alg;
} enc_grain;

typedef struct enc_grain_layout_ {
    void* meta_dest;
    void* data_src;
    void* data_dest;
} enc_grain_layout;

void enc_grain_read(enc_grain grain, enc_grain_layout layout);
void enc_grain_write(enc_grain grain, enc_grain_layout layout);
