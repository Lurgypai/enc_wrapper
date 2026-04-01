#!/bin/bash

#!/bin/bash

DEP_DIR=$(realpath ../../dependencies)
WRAPPER_DIR=$(realpath ../../enc_wrapper-ins)
OUT_DIR=$(realpath ../../enc_io-ins)
MPI_DIR=$(realpath ../../mpich-ins)

export GCRYPT_ROOT_DIR=${DEP_DIR}/gcrypt-ins
export GPG_ERROR_ROOT_DIR=${DEP_DIR}/gpgerror-ins

rm -rf ${OUT_DIR}

echo "Dependency Directory: ${DEP_DIR}"
echo "Output Directory: ${OUT_DIR}"

rm -r out
mkdir out
cd out

CC=gcc
CXX=g++

# select compiler
if [[ $ENABLE_MPI ]]; then
    echo "MPI enabled"
    MPICC=$(which mpicc)
    MPICXX=$(which mpicxx)
    if [[ -z $MPICC || -z $MPICC ]]; then
        MPICC=${MPI_DIR}/bin/mpicc
        MPICXX=${MPI_DIR}/bin/mpicxx

        if [[ ! -f $MPICC || ! -f $MPICXX ]]; then
            echo "MPI was not detected on the system, and not found in the $DEP_DIR"
            exit 1
        else
            echo "Using MPI in ${DEP_DIR}, MPICC=$MPICC MPICXX=$MPICXX"
        fi
    else
        echo "Using system MPI, MPICC=$MPICC, MPICXX=$MPICXX"
    fi

    CC=$MPICC
    CXX=$MPICXX
    CFLAGS="-DENABLE_MPI"
else
    echo "MPI disabled"
fi

cmake .. \
    -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} -DCMAKE_C_FLAGS=${CFLAGS} \
    -Denc_wrapper_DIR=${WRAPPER_DIR}/cmake \
    -DCMAKE_INSTALL_PREFIX=${OUT_DIR} \
    -DCMAKE_BUILD_TYPE=Debug
