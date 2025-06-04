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
echo "Script directory: ${SCRIPT_DIR}"
source "${SCRIPT_DIR}"/nrf_tools_config.sh

mkdir -p "${NRFTOOLS_DIR}"

echo "==== Installing nRF Tools ===="

echo "     Installing JLink ${JLINK_VER} in ${JLINK_INSTALL_DIR}"
if [ ! -d "${JLINK_INSTALL_DIR}/${JLINK_DIR}" ]; then
    echo "Downloading JLink installer..."
    curl -L -o "${SCRIPT_DIR}/${JLINK_FILE}" "${JLINK_URL}" --data accept_license_agreement=accepted
    mkdir -p "${JLINK_INSTALL_DIR}"
    tar -xzf "${SCRIPT_DIR}/${JLINK_FILE}" -C "${JLINK_INSTALL_DIR}"
    rm "${SCRIPT_DIR}/${JLINK_FILE}"
else
    echo "JLink ${JLINK_VER} installer already exists."
fi

echo "     Checking if JLink rules exist in /etc/udev/rules.d/99-jlink.rules"
if [ ! -f /etc/udev/rules.d/99-jlink.rules ]; then
    echo "JLink installed in ${JLINK_INSTALL_DIR}"
    echo "**NOTE**: From outside nix-shell copy Jlink rules to /etc/udev/rules.d/ to avoid permission issues."
    echo "         run 'sudo cp ${JLINK_INSTALL_DIR}/${JLINK_DIR}/99-jlink.rules /etc/udev/rules.d/'"
    echo "         run 'sudo udevadm control --reload-rules' and 'sudo udevadm trigger' to apply the changes."
else
    echo "JLink rules already exist in /etc/udev/rules.d/99-jlink.rules"
fi

echo "     Installing nRF Connect for Desktop in ${NRFTOOLS_DIR}" 
if [ ! -f "${NRFTOOLS_DIR}/${NRFTOOLS_CONNECT_APP_FILE}" ]; then
    echo "Downloading nRF Connect for Desktop installer..."
    echo "Power profiler url is ${NRFTOOLS_CONNECT_INSTALLER_URL}"
    curl -L -o "${SCRIPT_DIR}/${NRFTOOLS_CONNECT_APP_FILE}" "${NRFTOOLS_CONNECT_INSTALLER_URL}"
    chmod +x "${SCRIPT_DIR}/${NRFTOOLS_CONNECT_APP_FILE}"
    mkdir -p "${NRFTOOLS_DIR}"
    mv "${SCRIPT_DIR}/${NRFTOOLS_CONNECT_APP_FILE}" "${NRFTOOLS_DIR}/"
else
    echo "nRF Connect for Desktop installer already exists."
fi

echo "     Installing nRF Util in ${NRFTOOLS_UTIL_DIR}"
if [ ! -f "${NRFTOOLS_UTIL_EXEC}" ]; then
    echo "Downloading nRF Util..."
    curl -L -o "${SCRIPT_DIR}/${NRFTOOLS_UTIL_FILE}" "${NRFTOOLS_UTIL_URL}"
    chmod +x "${SCRIPT_DIR}/${NRFTOOLS_UTIL_FILE}"
    mv "${SCRIPT_DIR}/${NRFTOOLS_UTIL_FILE}" "${NRFTOOLS_UTIL_EXEC}"
else
    echo "nRF Util already exists."
fi

echo "     Downloading nRF Sniffer firmware"
if [ ! -f "${NRFTOOLS_UTIL_DIR}/${NRFTOOL_SNIFFER_FW_FILE}" ]; then
    echo "Downloading nRF Sniffer firmware..."
    mkdir -p "${NRFTOOLS_UTIL_DIR}"
    curl -L -o "${NRFTOOLS_UTIL_DIR}/${NRFTOOL_SNIFFER_FW_FILE}" "${NRFTOOL_SNIFFER_FW_URL}"
    unzip "${NRFTOOLS_UTIL_DIR}/${NRFTOOL_SNIFFER_FW_FILE}" -d "${NRFTOOLS_UTIL_DIR}"
    echo "nRF Sniffer firmware downloaded and extracted to ${NRFTOOLS_UTIL_DIR}"
else
    echo "nRF Sniffer firmware already exists."
fi

echo "     Installing nRF Sniffer for Bluetooth LE"
${NRFTOOLS_UTIL_EXEC} install ble-sniffer
if [ $? -ne 0 ]; then
    echo "Error installing BLE Sniffer. Please check the logs."
    exit 1
fi

echo "     Installing nRF Device tool"
${NRFTOOLS_UTIL_EXEC} install device
if [ $? -ne 0 ]; then
    echo "Error installing device. Please check the logs."
    exit 1
fi

echo "     Checking if nRF rules exists"
if [ ! -f /lib/udev/rules.d/71-nrf.rules ]; then
    echo "Downloading nRF rules..."
    wget "${NRFTOOLS_RULES_URL}" -O ${SCRIPT_DIR}/71-nrf.rules
    echo "**NOTE**: From outside nix-shell copy nRF rules to /lib/udev/rules.d/ to avoid permission issues."
    echo "         run 'sudo cp ${SCRIPT_DIR}/71-nrf.rules /lib/udev/rules.d/'"
    echo " Restart your computer to apply the changes."
else
    echo "nRF rules already exist."
fi

echo "     Checking if nRF blacklist rules exists"
if [ ! -f /lib/udev/rules.d/99-mm-nrf-blacklist.rules ]; then
    echo "Downloading nRF blacklist rules..."
    wget "${NRFTOOLS_RULES_BLACKLIST_URL}" -O ${SCRIPT_DIR}/99-mm-nrf-blacklist.rules
    echo "**NOTE**: From outside nix-shell copy nRF blacklist rules to /lib/udev/rules.d/ to avoid permission issues."
    echo "         run 'sudo cp ${SCRIPT_DIR}/99-mm-nrf-blacklist.rules /lib/udev/rules.d/'"
    echo " Restart your computer to apply the changes."
else
    echo "nRF blacklist rules already exist."
fi
