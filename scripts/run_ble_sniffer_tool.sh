#!/bin/bash
SCRIPT_DIR=$(cd $(dirname $0); pwd)
source "${SCRIPT_DIR}"/power_profiler_config.sh

ARGS=("$@")

echo "Running sniffer tool with arguments: ${ARGS[@]}"
echo "LD_LIBRARY_PATH=${SNIFF_JLINK_INSTALL_DIR}/${SNIFF_JLINK_DIR}"
# echo "LD_LIBRARY_PATH=${JLINK_INSTALL_DIR}/${JLINK_DIR}"
echo "Script directory: ${SCRIPT_DIR}/${SNIFF_UTIL_FILE}"
LD_LIBRARY_PATH="${SNIFF_JLINK_INSTALL_DIR}/${SNIFF_JLINK_DIR}" "${SCRIPT_DIR}/${SNIFF_UTIL_FILE}" "${ARGS[@]}"
# LD_LIBRARY_PATH="${JLINK_INSTALL_DIR}/${JLINK_DIR}" "${SCRIPT_DIR}/${SNIFF_UTIL_FILE}" "${ARGS[@]}"
