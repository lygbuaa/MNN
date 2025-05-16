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

VL_MODEL_PATH=${MNN_PATH}/../vlm_models/Qwen2.5-VL-7B-Instruct-MNN
CONFIG_FILE_PATH=${VL_MODEL_PATH}/config.json
PROMPT_FILE_PATH=${VL_MODEL_PATH}/prompt.txt
HOST_IP=192.18.34.101
HOST_PORT=10001
TEMPERATURE=0.0

${MNN_PATH}/build/llm_demo \
${CONFIG_FILE_PATH} \
${PROMPT_FILE_PATH}

# --host ${HOST_IP} \
# --port ${HOST_PORT} \
# -p ${PROMPT}