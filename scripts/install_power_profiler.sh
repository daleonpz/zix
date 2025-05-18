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
ls -l "${SCRIPT_DIR}"
source "${SCRIPT_DIR}"/power_profiler_config.sh

## Download the installer if it doesn't exist
if [ ! -f "${SCRIPT_DIR}"/"${POWER_PROF_INSTALLER_FILE}" ]; then
    echo "Downloading nRF Connect for Desktop installer..."
    echo "Power profiler url is ${POWER_PROF_INSTALLER_URL}"
    curl -L -o "${SCRIPT_DIR}/${POWER_PROF_INSTALLER_FILE}" "${POWER_PROF_INSTALLER_URL}"
    chmod +x "${SCRIPT_DIR}/${POWER_PROF_INSTALLER_FILE}"
else
    echo "nRF Connect for Desktop installer already exists."
fi

if [ ! -f "${SCRIPT_DIR}/${JLINK_FILE}" ]; then
    echo "Downloading JLink installer..."
    curl -L -o "${SCRIPT_DIR}/${JLINK_FILE}" "${JLINK_URL}" --data accept_license_agreement=accepted
    mkdir -p "${JLINK_INSTALL_DIR}"
    tar -xzf "${SCRIPT_DIR}/${JLINK_FILE}" -C "${JLINK_INSTALL_DIR}"
else
    echo "JLink ${JLINK_VER} installer already exists."
fi

if [ ! -f /etc/udev/rules.d/99-jlink.rules ]; then
    echo "JLink installed in ${JLINK_INSTALL_DIR}"
    echo "**NOTE**: From outside nix-shell copy Jlink rules to /etc/udev/rules.d/ to avoid permission issues."
    echo "         run 'sudo cp ${JLINK_INSTALL_DIR}/${JLINK_DIR}/99-jlink.rules /etc/udev/rules.d/'"
    echo "         run 'sudo udevadm control --reload-rules' and 'sudo udevadm trigger' to apply the changes."
fi




