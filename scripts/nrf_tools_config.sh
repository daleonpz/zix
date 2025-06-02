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
###### MODIFY THIS VARIABLE TO CHANGE THE INSTALLATION DIRECTORY ###########
JLINK_INSTALL_DIR="${HOME}/JLink"
NRFTOOLS_DIR="${HOME}/nrftools"


# This version of JLink is required for the nRF environment to work properly.
JLINK_VER="818"
JLINK_DIR="JLink_Linux_V${JLINK_VER}_x86_64"
JLINK_FILE="${JLINK_DIR}.tgz"
JLINK_URL="https://www.segger.com/downloads/jlink/${JLINK_FILE}"

NRFTOOLS_RULES_URL="https://raw.githubusercontent.com/NordicSemiconductor/nrf-udev/refs/heads/main/nrf-udev_1.0.1-all/lib/udev/rules.d/71-nrf.rules"
NRFTOOLS_RULES_BLACKLIST_URL="https://raw.githubusercontent.com/NordicSemiconductor/nrf-udev/refs/heads/main/nrf-udev_1.0.1-all/lib/udev/rules.d/99-mm-nrf-blacklist.rules"

# Verify the last version of nRF Connect for Desktop
# https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-Desktop
NRFTOOLS_CONNECT_INSTALLER_VER="5-1-0"
NRFTOOLS_CONNECT_FILE_VER=$(echo "${NRFTOOLS_CONNECT_INSTALLER_VER}" | sed 's/-/./g')
NRFTOOLS_CONNECT_APP_FILE="nrfconnect-${NRFTOOLS_CONNECT_FILE_VER}-x86_64.appimage"
NRFTOOLS_CONNECT_INSTALLER_URL="https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/desktop-software/nrf-connect-for-desktop/${NRFTOOLS_CONNECT_INSTALLER_VER}/${NRFTOOLS_CONNECT_APP_FILE}"

# Variables related to nRF Util
NRFTOOLS_UTIL_DIR="${NRFTOOLS_DIR}/nrfutil_tools"
NRFTOOLS_UTIL_URL="https://files.nordicsemi.com/artifactory/swtools/external/nrfutil/executables/x86_64-unknown-linux-gnu/nrfutil"
NRFTOOLS_UTIL_FILE="nrfutil"
NRFTOOLS_UTIL_EXEC="${NRFTOOLS_DIR}/${NRFTOOLS_UTIL_FILE}"

# Variables related to nRF Sniffer
NRFTOOL_SNIFFER_FW_FILE="nrf_sniffer_for_bluetooth_le_4.1.1.zip"
NRFTOOL_SNIFFER_FW_URL="https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/desktop-software/nrf-sniffer/sw/${NRFTOOL_SNIFFER_FW_FILE}"

# Variables related to nRF Connect SDK
NRFTOOL_SDK_CONNECT_VERSION="v3.1.0"
