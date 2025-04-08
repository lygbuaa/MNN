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

LIB_MNN_PATH=${MNN_PATH}/build
export LD_LIBRARY_PATH=${LIB_MNN_PATH}:$LD_LIBRARY_PATH

HOST_IP=127.0.0.1
HOST_PORT=10001
CONFIG_FILE_NAME=config.json
MODEL_PATH_0=${MNN_PATH}/../vlm_models/Qwen2.5-7B-Instruct-MNN
MODEL_PATH_1=${MNN_PATH}/../vlm_models/Qwen2.5-VL-7B-Instruct-MNN
# TEMPERATURE=0.0
# PROMPT_FILE_PATH=${VL_MODEL_PATH}/prompt.txt

${MNN_PATH}/build/llm_server \
${HOST_IP} \
${HOST_PORT} \
${CONFIG_FILE_NAME} \
${MODEL_PATH_0} \
${MODEL_PATH_1}
