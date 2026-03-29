#include "enc_wrapper.h"
#include "enc_gcrypt.h"
#include "enc_nettle.h"
#include "enc_dummy.h"

#include <stdio.h>
#include <stdlib.h>

static int is_inited = 0;
static enc_library_impl Impls[4];
static enc_library Cur_Library;
static enc_library_impl* Enc_Library_Impl;

static void init_libraries() {
    if(!is_inited) {
        Cur_Library = enc_lib_none;
        Impls[enc_lib_dummy] = enc_get_dummy();
        Impls[enc_lib_gcrypt] = enc_get_gcrypt();
        Impls[enc_lib_nettle] = enc_get_nettle();
        is_inited = 1;
    }
}

int enc_load_library(enc_library enc_lib) {
    init_libraries();
    if(Cur_Library != enc_lib_none) enc_close();
    Enc_Library_Impl = Impls + enc_lib;
    Cur_Library = enc_lib;
    return 0;
}

int enc_prepare(enc_algorithm alg) {
    if(Enc_Library_Impl->prepare == NULL) return 0;

    (*Enc_Library_Impl->prepare)(alg, &Enc_Library_Impl->key_size, &Enc_Library_Impl->nonce_size, &Enc_Library_Impl->block_size);
    return 0;
}

int enc_load_config(enc_config cfg) {
    enc_load_library(cfg.lib);
    enc_prepare(cfg.alg);
    return 0;
}

int enc_set_key(char* key, size_t key_len) {
    if(Enc_Library_Impl->set_key == NULL) return 0;
    (*Enc_Library_Impl->set_key)(key, key_len);
    return 0;
}

int enc_set_nonce(char* nonce, size_t nonce_len) {
    if(Enc_Library_Impl->set_nonce == NULL) return 0;
    (*Enc_Library_Impl->set_nonce)(nonce, nonce_len);
    return 0;
}

int enc_encrypt(void* source, size_t source_size, void* dest, size_t dest_size) {
    (*Enc_Library_Impl->encrypt)(source, source_size, dest, dest_size, Enc_Library_Impl->block_size);
    return 0;
}

int enc_decrypt(void* source, size_t source_size, void* dest, size_t dest_size) {
    (*Enc_Library_Impl->decrypt)(source, source_size, dest, dest_size, Enc_Library_Impl->block_size);
    return 0;
}

int enc_reset() {
    if(Enc_Library_Impl->reset == NULL) return 0;
    (*Enc_Library_Impl->reset)();
    return 0;
}

int enc_close() {
    if(Enc_Library_Impl->close == NULL) return 0;
    (*Enc_Library_Impl->close)();
    Cur_Library = enc_lib_none;
    return 0;
}

size_t enc_get_key_size() {
    return Enc_Library_Impl->key_size;
}

size_t enc_get_nonce_size() {
    return Enc_Library_Impl->nonce_size;
}

size_t enc_get_block_size() {
    return Enc_Library_Impl->block_size;
}

size_t enc_get_encrypted_size(enc_config cfg, size_t raw_size) {
    init_libraries();
    return Impls[cfg.lib].get_encrypted_size(cfg.alg, raw_size);
}

char* enc_make_key() {
    return (*Enc_Library_Impl->make_key)(Enc_Library_Impl->key_size);
}

char* enc_make_nonce() {
    return (*Enc_Library_Impl->make_nonce)(Enc_Library_Impl->nonce_size);
}
