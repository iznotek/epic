#!/usr/bin/env bash
set -ev
EPIC_GRPC_VERSION="1.24.0"
EPIC_GRPC_DIR=""grpc-"$EPIC_GRPC_VERSION"

if [ ${TRAVIS_OS_NAME} == 'osx' ]; then
    brew install grpc
else
    if [[ ! -d ${EPIC_GRPC_DIR} ]];then
        sudo rm -rf grpc*
        git clone -b v${EPIC_GRPC_VERSION} --single-branch https://github.com/grpc/grpc.git ${EPIC_GRPC_DIR}
        cd ${EPIC_GRPC_DIR}
        git submodule update --init
        make -j6
    else
        cd ${EPIC_GRPC_DIR}
    fi

    sudo make install
    cd ..
fi
