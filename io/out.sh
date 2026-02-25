#!/bin/bash

#!/bin/bash

DEP_DIR=$(realpath ../../dependencies)
WRAPPER_DIR=$(realpath ../../enc_wrapper-ins)
OUT_DIR=$(realpath ../../enc_io-ins)
export ENC_WRAPPER_ROOT_DIR=${WRAPPER_DIR}

rm -rf ${OUT_DIR}

echo "Dependency Directory: ${DEP_DIR}"
echo "Output Directory: ${OUT_DIR}"

rm -r out
mkdir out
cd out

cmake .. \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
    -DCMAKE_INSTALL_PREFIX=${OUT_DIR} \

mv compile_commands.json ..
