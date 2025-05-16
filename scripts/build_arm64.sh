#!/bin/bash

function find_mnn_path() {
    # echo "@i@ --> find dir: ${0}"
    this_script_dir=$( dirname -- "$0"; )
    pwd_dir=$( pwd; )

    if [ "${this_script_dir:0:1}" = "/" ]
    then
        # echo "get absolute path ${this_script_dir}" > /dev/tty
        MNN_PATH=${this_script_dir}"/../"
    else
        # echo "get relative path ${this_script_dir}" > /dev/tty
        MNN_PATH=${pwd_dir}"/"${this_script_dir}"/../"
    fi
    echo "${MNN_PATH}"
}

MNN_PATH=$( find_mnn_path )
cd ${MNN_PATH}
mkdir -p build

cd build
cmake ../ -DMNN_LOW_MEMORY=true -DMNN_CPU_WEIGHT_DEQUANT_GEMM=true \
          -DMNN_BUILD_LLM=true -DMNN_SUPPORT_TRANSFORMER_FUSE=true \
          -DLLM_SUPPORT_VISION=true -DMNN_BUILD_OPENCV=true -DMNN_IMGCODECS=true \
          -DMNN_ARM82=true  #-DMNN_OPENCL=true
make -j8