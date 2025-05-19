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

CPU_ARCHITECTURE=`uname -m`

mkdir -p kansas/include
mkdir -p kansas/lib/${CPU_ARCHITECTURE}
mkdir -p kansas/transformers
mkdir -p kansas/scripts
mkdir -p kansas/tools/cv/include/cv
mkdir -p kansas/tools/audio/include/audio
mkdir -p kansas/tools/cpp
mkdir -p kansas/3rd_party/rapidjson
mkdir -p kansas/source/core/

cp scripts/CMakeLists.txt.kansas        kansas/CMakeLists.txt
cp -r 3rd_party/rapidjson/*             kansas/3rd_party/rapidjson/
cp -r source/core/*.h                   kansas/source/core/
cp -r source/core/*.hpp                 kansas/source/core/
cp -r include/*                         kansas/include/
cp -r tools/cv/include/cv/*             kansas/tools/cv/include/cv/
cp -r tools/audio/include/audio/*       kansas/tools/audio/include/audio/
cp -r tools/cpp/*.hpp                   kansas/tools/cpp
cp -r transformers/*                    kansas/transformers/
rm -rf kansas/transformers/llm/engine/ios
cp build/libMNN.so                      kansas/lib/${CPU_ARCHITECTURE}/
cp build/express/libMNN_Express.so      kansas/lib/${CPU_ARCHITECTURE}/
cp build/tools/cv/libMNNOpenCV.so       kansas/lib/${CPU_ARCHITECTURE}/
cp scripts/build_kansas.sh              kansas/scripts/
cp scripts/socket_client.py             kansas/scripts/
cp scripts/socket_server.py             kansas/scripts/
