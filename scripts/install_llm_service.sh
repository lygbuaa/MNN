#!/bin/bash
set -x

# setup env
export PROJ_INSTALL_PATH=/opt/202504_wuhan_llm

mkdir -p /usr/lib/systemd/system
cp ${PROJ_INSTALL_PATH}/mnn_llm_server.ad1000/scripts/mnn_llm_server.service /usr/lib/systemd/system
systemctl daemon-reload
