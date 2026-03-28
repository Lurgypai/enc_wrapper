#pragma once

typedef enum enc_algorithm {
    aes256,
    chacha20,
    camellia256,
    twofish
} enc_algorithm;

typedef enum enc_library_ {
    enc_lib_none,
    enc_lib_dummy,
    enc_lib_gcrypt,
    enc_lib_nettle
} enc_library;

typedef struct enc_config_ {
    enc_algorithm alg;
    enc_library lib;
} enc_config;
