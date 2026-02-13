#include "enc_store_contig.h"
#include <fcntl.h>
#include <unistd.h>

enc_store_contig enc_store_contig_open(char* filename) {
    enc_store_contig store;
    store.obj_cnt = 0;
    store.obj_reserved = 0;
    store.objs = NULL;
    store.obj_offsets = NULL;

    store.file = open(filename, O_CREAT | O_APPEND| O_RDWR);

    return store;
}

void enc_store_contig_close(enc_store_contig store) {
    close(store.file);
}


