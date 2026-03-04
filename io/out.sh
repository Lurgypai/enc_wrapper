#!/bin/bash

#!/bin/bash

DEP_DIR=$(realpath ../../dependencies)
WRAPPER_DIR=$(realpath ../../enc_wrapper-ins)
OUT_DIR=$(realpath ../../enc_io-ins)
export GCRYPT_ROOT_DIR=${DEP_DIR}/gcrypt-ins
export GPG_ERROR_ROOT_DIR=${DEP_DIR}/gpgerror-ins

rm -rf ${OUT_DIR}

echo "Dependency Directory: ${DEP_DIR}"
echo "Output Directory: ${OUT_DIR}"

rm -r out
mkdir out
cd out

cmake .. \
    -Denc_wrapper_DIR=${WRAPPER_DIR}/cmake \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
    -DCMAKE_INSTALL_PREFIX=${OUT_DIR} \
    -DCMAKE_BUILD_TYPE=Debug

mv compile_commands.json ..
