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

LD_LIBRARY_PATH="${JLINK_INSTALL_DIR}/${JLINK_DIR}" "${SCRIPT_DIR}/${POWER_PROF_INSTALLER_FILE}"
