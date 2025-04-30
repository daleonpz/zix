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

# Verify the last version of nRF Connect for Desktop
# https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-Desktop
POWER_PROF_INSTALLER_VER="5-1-0"
## convert version from 5-1-0 to 5.1.0
POWER_PROF_FILE_VER=$(echo "${POWER_PROF_INSTALLER_VER}" | sed 's/-/./g')
POWER_PROF_INSTALLER_FILE="nrfconnect-${POWER_PROF_FILE_VER}-x86_64.appimage"
POWER_PROF_INSTALLER_URL="https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/desktop-software/nrf-connect-for-desktop/${POWER_PROF_INSTALLER_VER}/${POWER_PROF_INSTALLER_FILE}"


# This version of JLink is required for the Power Profiler
JLINK_VER="794i"
JLINK_DIR="JLink_Linux_V${JLINK_VER}_x86_64"
JLINK_FILE="${JLINK_DIR}.tgz"
JLINK_URL="https://www.segger.com/downloads/jlink/${JLINK_FILE}"

###### MODIFY THIS VARIABLE TO CHANGE THE INSTALLATION DIRECTORY ###########
JLINK_INSTALL_DIR="${HOME}/JLink"

#############################################
# Setting for the BLE Sniffer
#############################################
# This version of JLink is required for the Power Profiler
SNIFF_JLINK_VER="818"
SNIFF_JLINK_DIR="JLink_Linux_V${SNIFF_JLINK_VER}_x86_64"
SNIFF_JLINK_FILE="${SNIFF_JLINK_DIR}.tgz"
SNIFF_JLINK_URL="https://www.segger.com/downloads/jlink/${SNIFF_JLINK_FILE}"

###### MODIFY THIS VARIABLE TO CHANGE THE INSTALLATION DIRECTORY ###########
SNIFF_JLINK_INSTALL_DIR="${HOME}/jlink_sniffer"

SNIFF_UTIL_URL="https://files.nordicsemi.com/artifactory/swtools/external/nrfutil/executables/x86_64-unknown-linux-gnu/nrfutil"
SNIFF_UTIL_FILE="nrfutil"
