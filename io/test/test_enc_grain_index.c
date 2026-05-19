#include <stdlib.h>
#include <stdio.h>

#include "enc_grain_index.h"

void test_enc_grain_index_select() {
    enc_grain_index idx = enc_grain_index_make();
    enc_grain_index_add_grain(&idx, 0, 0, 16);
    enc_grain_index_add_grain(&idx, 1, 16, 16);
    enc_grain_index_add_grain(&idx, 2, 32, 16);
    enc_grain_index_add_grain(&idx, 3, 48, 16);

    size_t* ids;
    size_t id_cnt;
    enc_grain_index_select_grains(&idx, 0, 64, &ids, &id_cnt);

    printf("Expected ids: 0, 1, 2, 3\n");
    printf("Received ids: ");
    for(int i = 0; i != id_cnt; ++i) {
        printf("%lu, ", ids[i]);
    }
    printf("\n");
    free(ids);

    enc_grain_index_select_grains(&idx, 0, 16, &ids, &id_cnt);

    printf("Expected ids: 0\n");
    printf("Received ids: ");
    for(int i = 0; i != id_cnt; ++i) {
        printf("%lu, ", ids[i]);
    }
    printf("\n");
    free(ids);

    enc_grain_index_select_grains(&idx, 16, 16, &ids, &id_cnt);

    printf("Expected ids: 1\n");
    printf("Received ids: ");
    for(int i = 0; i != id_cnt; ++i) {
        printf("%lu, ", ids[i]);
    }
    printf("\n");
    free(ids);

    enc_grain_index_select_grains(&idx, 16, 32, &ids, &id_cnt);

    printf("Expected ids: 1, 2\n");
    printf("Received ids: ");
    for(int i = 0; i != id_cnt; ++i) {
        printf("%lu, ", ids[i]);
    }
    printf("\n");
    free(ids);

    enc_grain_index_select_grains(&idx, 8, 32, &ids, &id_cnt);

    printf("Expected ids: 0, 1, 2\n");
    printf("Received ids: ");
    for(int i = 0; i != id_cnt; ++i) {
        printf("%lu, ", ids[i]);
    }
    printf("\n");
    free(ids);
    
    enc_grain_index_select_grains(&idx, 32, 32, &ids, &id_cnt);

    printf("Expected ids: 2, 3\n");
    printf("Received ids: ");
    for(int i = 0; i != id_cnt; ++i) {
        printf("%lu, ", ids[i]);
    }
    printf("\n");
    free(ids);

    enc_grain_index_free(&idx);

    printf("Select tests complete\n");
}

void test_enc_grain_index_put_parse() {
    enc_grain_index idx1 = enc_grain_index_make();
    enc_grain_index_add_grain(&idx1, 0, 0, 16);
    enc_grain_index_add_grain(&idx1, 1, 16, 16);
    enc_grain_index_add_grain(&idx1, 2, 32, 16);
    enc_grain_index_add_grain(&idx1, 3, 48, 16);

    size_t size = enc_grain_index_get_size(&idx1);
    void* buf = malloc(size);
    enc_grain_index_put(&idx1, buf);

    enc_grain_index idx2;
    enc_grain_index_parse(&idx2, buf);

    int meta_success = idx1.cnt == idx2.cnt && idx1.reserved == idx2.reserved;
    int grain_success = 1;

    for(int i = 0; i != idx1.cnt; ++i) {
        enc_grain_index_desc* desc1 = &idx1.grains[i];
        enc_grain_index_desc* desc2 = &idx2.grains[i];

        if(desc1->id != desc2->id || desc1->offset != desc2->offset || desc1->size != desc2->size) {
            grain_success = 0;
            break;
        }
    }

    printf("meta_success: %d\n", meta_success);
    printf("grain_success: %d\n", grain_success);
    printf("put/parse tests complete\n");
}

int main() {
    test_enc_grain_index_select();
    test_enc_grain_index_put_parse();
}
