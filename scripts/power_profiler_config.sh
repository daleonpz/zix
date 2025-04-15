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

