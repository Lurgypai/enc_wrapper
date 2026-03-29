#pragma once

#include <string.h>

#include "enc_config.h"

int enc_load_library(enc_library lib);
int enc_prepare(enc_algorithm alg);
int enc_load_config(enc_config cfg);
int enc_set_key(char* key, size_t key_len);
int enc_set_nonce(char* nonce, size_t key_len);
int enc_encrypt(void* source, size_t source_size, void* dest, size_t dest_size); 
int enc_decrypt(void* source, size_t source_size, void* dest, size_t dest_size); 
int enc_reset();
int enc_close();
size_t enc_get_key_size();
size_t enc_get_nonce_size();
size_t enc_get_block_size();
size_t enc_get_encrypted_size(enc_config cfg, size_t raw_size);
char* enc_make_key();
char* enc_make_nonce();

