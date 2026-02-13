#!/bin/bash

#!/bin/bash

DEP_DIR=$(realpath dependencies)
OUT_DIR=$(realpath ../enc_wrapper-ins)

rm -rf ${OUT_DIR}

echo "Dependency Directory: ${DEP_DIR}"
echo "Output Directory: ${OUT_DIR}"

export GCRYPT_ROOT_DIR=${DEP_DIR}/gcrypt-ins
export GPG_ERROR_ROOT_DIR=${DEP_DIR}/gpgerror-ins

rm -r out
mkdir out
cd out

cmake .. \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
    -DCMAKE_INSTALL_PREFIX=${OUT_DIR} \
    -DENC_WRAPPER_ENABLE_NETTLE=Off

mv compile_commands.json ..
