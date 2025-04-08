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
mkdir -p kentucky/build
mkdir -p kentucky/images
mkdir -p kentucky/scripts

cp build/*.so                       kentucky/build
cp build/express/libMNN_Express.so  kentucky/build
cp build/tools/cv/libMNNOpenCV.so   kentucky/build
cp build/llm_server                 kentucky/build
cp -r scripts/run_mllm_server.sh    kentucky/scripts
cp -r scripts/*.py                  kentucky/scripts
cp -r images/270p                   kentucky/images

cp -r scripts/mnn_llm_server.service    kentucky/scripts
cp -r scripts/install_llm_service.sh    kentucky/scripts
cp -r scripts/socket_client.py          kentucky/scripts