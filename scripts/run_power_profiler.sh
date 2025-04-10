#!/bin/bash
SCRIPT_DIR=$(cd $(dirname $0); pwd)
source "${SCRIPT_DIR}"/power_profiler_config.sh

LD_LIBRARY_PATH="${JLINK_INSTALL_DIR}/${JLINK_DIR}" "${SCRIPT_DIR}/${POWER_PROF_INSTALLER_FILE}"
