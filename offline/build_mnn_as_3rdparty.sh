#!/bin/bash

function find_mnn_root_path() {
    # echo "@i@ --> find dir: ${0}"
    this_script_dir=$( dirname -- "$0"; )
    pwd_dir=$( pwd; )

    if [ "${this_script_dir:0:1}" = "/" ]
    then
        # echo "get absolute path ${this_script_dir}" > /dev/tty
        MNN_ROOT_PATH=${this_script_dir}
    else
        # echo "get relative path ${this_script_dir}" > /dev/tty
        MNN_ROOT_PATH=${pwd_dir}"/"${this_script_dir}
    fi
    echo "${MNN_ROOT_PATH}"
}

MNN_ROOT_PATH=$( find_mnn_root_path )
cd ${MNN_ROOT_PATH}

MNN_SRC_DIR=MNN-3.2.0
MNN_SRC_ZIP_NAME=${MNN_SRC_DIR}.zip

if [ ! -f "${MNN_SRC_ZIP_NAME}" ]; then
    echo "download mnn source code in current path" >&2
    echo "wget -c -O MNN-3.2.0.zip https://github.com/alibaba/MNN/archive/refs/tags/3.2.0.zip" >&2
    wget -c -O MNN-3.2.0.zip https://github.com/alibaba/MNN/archive/refs/tags/3.2.0.zip
else
    echo "${MNN_SRC_ZIP_NAME} exists in current path" >&2
fi

if [ ! -d "${MNN_SRC_DIR}" ]; then
    echo "unzip ${MNN_SRC_ZIP_NAME}"
    rm -rf ${MNN_SRC_DIR}
    unzip ${MNN_SRC_ZIP_NAME}
else
    echo "${MNN_SRC_DIR} already exists"
fi

cd  ${MNN_SRC_DIR}
mkdir -p build
cd build
cmake ../ -DMNN_LOW_MEMORY=true -DMNN_CPU_WEIGHT_DEQUANT_GEMM=true \
          -DMNN_BUILD_LLM=true -DMNN_SUPPORT_TRANSFORMER_FUSE=true \
          -DLLM_SUPPORT_VISION=true -DMNN_BUILD_OPENCV=true -DMNN_IMGCODECS=true \
          -DLLM_SUPPORT_AUDIO=true -DMNN_BUILD_AUDIO=true -DMNN_ARM82=true \
          -DCMAKE_INSTALL_PREFIX=${MNN_ROOT_PATH}/${MNN_SRC_DIR}/install
make -j8
make install

cd ${MNN_ROOT_PATH}/${MNN_SRC_DIR}
CPU_ARCHITECTURE=`uname -m`
MNN_PACKAGE_PATH=MakeSiengineGreatAgain

## copy headers && libraries from install
mkdir -p                            ${MNN_PACKAGE_PATH}/include
cp -r install/include/*             ${MNN_PACKAGE_PATH}/include/
mkdir -p                            ${MNN_PACKAGE_PATH}/lib/${CPU_ARCHITECTURE}
cp -r -P install/lib/*              ${MNN_PACKAGE_PATH}/lib/${CPU_ARCHITECTURE}/
## libMNNOpenCV.so is not installed
cp build/tools/cv/libMNNOpenCV.so   ${MNN_PACKAGE_PATH}/lib/${CPU_ARCHITECTURE}/

## copy llm source files
mkdir -p                            ${MNN_PACKAGE_PATH}/transformers
cp -r transformers/llm              ${MNN_PACKAGE_PATH}/transformers/
rm -rf                              ${MNN_PACKAGE_PATH}/transformers/llm/engine/ios

## copy headers in tools 
mkdir -p                            ${MNN_PACKAGE_PATH}/tools/cpp
cp -r tools/cpp/*.hpp               ${MNN_PACKAGE_PATH}/tools/cpp

## copy other headers
mkdir -p                            ${MNN_PACKAGE_PATH}/3rd_party/rapidjson
mkdir -p                            ${MNN_PACKAGE_PATH}/source/core/
cp -r 3rd_party/rapidjson/*         ${MNN_PACKAGE_PATH}/3rd_party/rapidjson/
cp -r source/core/*.h               ${MNN_PACKAGE_PATH}/source/core/
cp -r source/core/*.hpp             ${MNN_PACKAGE_PATH}/source/core/

### move ${MNN_PACKAGE_PATH} to ${MNN_ROOT_PATH}
mv ${MNN_PACKAGE_PATH}/* ${MNN_ROOT_PATH}
cd ${MNN_ROOT_PATH}
# rm -rf ${MNN_SRC_DIR}
# rm -rf ${MNN_SRC_ZIP_NAME}