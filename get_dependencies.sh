#!/bin/bash

DEP_DIR=$(realpath ../dependencies)

rm -rf ${DEP_DIR}
mkdir ${DEP_DIR}

pushd ${DEP_DIR} > /dev/null

INSTALL_DIR=$(pwd)

GPG_ERROR_CONFIG=$(which gpg-error-config)
if [[ -z ${GPG_ERROR_CONFIG} ]]; then
    echo "Downloading and unzipping gpg error"
    wget https://www.gnupg.org/ftp/gcrypt/libgpg-error/libgpg-error-1.56.tar.bz2
    tar -xvf "libgpg-error-1.56.tar.bz2"
    rm "libgpg-error-1.56.tar.bz2"
    echo "Compiling gpg error"
    pushd "libgpg-error-1.56" > /dev/null
    ./autogen.sh
    ./configure --prefix=${INSTALL_DIR}/gpgerror-ins --enable-install-gpg-error-config
    make -j`nproc` && make install
    popd > /dev/null
else
    echo "Found gpg-error-config at \"${GPG_ERROR_CONFIG}\", skipping install"
fi

GCRYPT_CONFIG=$(which libgcrypt-config)
if [[ -z ${GCRYPT_CONFIG} ]]; then
    echo "Downloading and unzipping gcrypt"
    wget https://www.gnupg.org/ftp/gcrypt/libgcrypt/libgcrypt-1.11.2.tar.bz2
    tar -xvf "libgcrypt-1.11.2.tar.bz2"
    rm "libgcrypt-1.11.2.tar.bz2"
    pushd "libgcrypt-1.11.2" > /dev/null
    ./configure --prefix=${INSTALL_DIR}/gcrypt-ins --with-libgpg-error-prefix=${INSTALL_DIR}/gpgerror-ins
    make -j`nproc` && make install
    popd > /dev/null
else
    echo "Found gcrypt-config at \"${GCRYPT_CONFIG}\", skipping install"
fi

echo "Downlaind and unzipping nettle"
wget https://ftp.gnu.org/gnu/nettle/nettle-3.10.2.tar.gz
tar -xvf "nettle-3.10.2.tar.gz"
rm "nettle-3.10.2.tar.gz"
pushd "nettle-3.10.2" > /dev/null
./configure --prefix=${INSTALL_DIR}/nettle-ins
make -j`nproc` && make install
popd > /dev/null

MPICC=$(which mpicc)
if [[ -z ${MPICC} ]]; then
    git clone git@github.com:pmodels/mpich.git
    pushd mpich
        git submodule update --init
        ./autogen.sh
        ./configure --prefix=${DEP_DIR}/mpich-ins
        make -j`nproc` && make install
    popd
else
    echo "Found mpicc at \"${MPICC}\", skipping install"
fi

popd
