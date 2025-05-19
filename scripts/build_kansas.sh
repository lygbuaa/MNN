#!/bin/bash

function find_mnn_path() {
    # echo "@i@ --> find dir: ${0}"
    this_script_dir=$( dirname -- "$0"; )
    pwd_dir=$( pwd; )

    if [ "${this_script_dir:0:1}" = "/" ]
    then
        # echo "get absolute path ${this_script_dir}" > /dev/tty
        MNN_ROOT_PATH=${this_script_dir}"/../"
    else
        # echo "get relative path ${this_script_dir}" > /dev/tty
        MNN_ROOT_PATH=${pwd_dir}"/"${this_script_dir}"/../"
    fi
    echo "${MNN_ROOT_PATH}"
}

MNN_ROOT_PATH=$( find_mnn_path )
cd ${MNN_ROOT_PATH}

BUILD_TYPE=Release  #Debug or Release
# ARCH_TYPE=aarch64
CPU_ARCHITECTURE=`uname -m`
echo "CPU_ARCHITECTURE: ${CPU_ARCHITECTURE}"

BUILD_CMD_LINE="-DCMAKE_INSTALL_PREFIX=${MNN_ROOT_PATH}/install "
BUILD_CMD_LINE="${BUILD_CMD_LINE} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} "
BUILD_CMD_LINE="${BUILD_CMD_LINE} -DMNN_LOW_MEMORY=true -DMNN_CPU_WEIGHT_DEQUANT_GEMM=true \
                                  -DMNN_BUILD_LLM=true -DMNN_SUPPORT_TRANSFORMER_FUSE=true \
                                  -DLLM_SUPPORT_VISION=true -DMNN_BUILD_OPENCV=true -DMNN_IMGCODECS=true"

if [ "$CPU_ARCHITECTURE" = "x86_64" ]; then
    BUILD_CMD_LINE="${BUILD_CMD_LINE} -DMNN_AVX512=true"
elif [ "$CPU_ARCHITECTURE" = "aarch64" ]; then
    BUILD_CMD_LINE="${BUILD_CMD_LINE} -DMNN_ARM82=true"
else
    echo "invalid CPU_ARCHITECTURE: ${CPU_ARCHITECTURE}"
    exit 1
fi
BUILD_CMD_LINE="${BUILD_CMD_LINE} ${MNN_ROOT_PATH} "

echo "BUILD_CMD_LINE: ${BUILD_CMD_LINE}"

mkdir -p build
cd build
cmake ${BUILD_CMD_LINE}
make -j8
make install