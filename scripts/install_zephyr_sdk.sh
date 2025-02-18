#!/bin/sh

ZEPHYR_SDK_VERSION="0.17.0"
ZEPHYR_SDK_FILE="zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64_minimal.tar.xz"
ZEPHYR_BASE_URI="https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}"
SHA256SUM="0514d2c684dfb5f6327374bfed0b3dcf727ff1500195d26b3730f98252fed095"

SCRIPT_DIR=$(cd $(dirname $0); pwd)
SDK_FOLDER="zephyr-sdk"
ZEPHYR_SDK_INSTALL_DIR=${SCRIPT_DIR}/../${SDK_FOLDER}

TOOLCHAIN="arm-zephyr-eabi"

if [ ! -d ${ZEPHYR_SDK_INSTALL_DIR} ]; then
    cd ${LOCAL_HOME}
    mkdir -p ${ZEPHYR_SDK_INSTALL_DIR}

    wget ${ZEPHYR_BASE_URI}/${ZEPHYR_SDK_FILE} 
    echo "${SHA256SUM} ${ZEPHYR_SDK_FILE}" | sha256sum -c -
    tar xvf ${ZEPHYR_SDK_FILE} -C ${SDK_FOLDER} --strip-components=1

    rm ${ZEPHYR_SDK_FILE}
    cd ${ZEPHYR_SDK_INSTALL_DIR}
    ./setup.sh -t ${TOOLCHAIN}
fi
