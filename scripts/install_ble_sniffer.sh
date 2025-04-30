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
#!/bin/bash
SCRIPT_DIR=$(cd $(dirname $0); pwd)
source "${SCRIPT_DIR}"/power_profiler_config.sh

if [ ! -f "${SCRIPT_DIR}/${SNIFF_JLINK_FILE}" ]; then
    echo "Downloading JLink installer..."
    curl -L -o "${SCRIPT_DIR}/${SNIFF_JLINK_FILE}" "${SNIFF_JLINK_URL}" --data accept_license_agreement=accepted
    mkdir -p "${SNIFF_JLINK_INSTALL_DIR}"
    tar -xzf "${SCRIPT_DIR}/${SNIFF_JLINK_FILE}" -C "${SNIFF_JLINK_INSTALL_DIR}"
else
    echo "JLink ${SNIFF_JLINK_VER} installer already exists."
fi

SNIFF_UTIL_URL="https://files.nordicsemi.com/artifactory/swtools/external/nrfutil/executables/x86_64-unknown-linux-gnu/nrfutil"
if [ ! -f "${SCRIPT_DIR}/${SNIFF_UTIL_FILE}" ]; then
    echo "Downloading sniffer tool..."
    curl -L -o "${SCRIPT_DIR}/${SNIFF_UTIL_FILE}" "${SNIFF_UTIL_URL}"
    chmod +x "${SCRIPT_DIR}/${SNIFF_UTIL_FILE}"
else
    echo "sniffer tool already exists."
fi

SNIFF_UTIL_EXEC="${SCRIPT_DIR}/${SNIFF_UTIL_FILE}"

${SNIFF_UTIL_EXEC} install ble-sniffer
if [ $? -ne 0 ]; then
    echo "Error installing BLE Sniffer. Please check the logs."
    exit 1
fi

${SNIFF_UTIL_EXEC} install device
if [ $? -ne 0 ]; then
    echo "Error installing device. Please check the logs."
    exit 1
fi
