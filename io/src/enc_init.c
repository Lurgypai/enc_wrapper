#include "enc_init.h"

#include <mpi.h>

int ENC_RANK_G = 0;

void enc_init() {
    MPI_Comm_rank(MPI_COMM_WORLD, &ENC_RANK_G);
}
