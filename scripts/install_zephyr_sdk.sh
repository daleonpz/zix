# Copyright 2025 Daniel Paredes (daleonpz)
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
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
