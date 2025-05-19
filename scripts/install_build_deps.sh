#!/bin/bash

# install package without interaction
export UCF_FORCE_CONFFNEW=YES
export DEBIAN_FRONTEND=noninteractive

NONINTERACTIVE_INSTALL="apt-get install -o Dpkg::Options::=--force-confnew -yfqq "
# ${NONINTERACTIVE_INSTALL} cmake nlohmann-json3-dev libjson-c-dev libopencv-dev libtinyxml2-dev libpcl-dev
echo "mnn_server has nothing to install"
