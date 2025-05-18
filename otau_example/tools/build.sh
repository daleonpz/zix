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

ARGS=("$@")

if [ -z "$1" ]; then
    west build -b nucleo_wb55rg --pristine --sysbuild
    exit 1
fi

if ! [[ "$1" =~ ^[0-9]+(\.[0-9]+){0,2}$ ]]; then
    echo "Error: Invalid version number format. Please use X.Y.Z format."
    exit 1
fi

IFS='.' read -r major minor patch <<< "$1"

if [ -z "$major" ] || [ -z "$minor" ] || [ -z "$patch" ]; then
    echo "Error: Invalid version number format. Please use X.Y.Z format."
    exit 1
fi

VERSION_MAJOR=$major
VERSION_MINOR=$minor
VERSION_PATCH=$patch

west build -b nucleo_wb55rg --pristine --sysbuild  -- \
            -Dotau_example_VERSION_MAJOR=$VERSION_MAJOR \
            -Dotau_example_VERSION_MINOR=$VERSION_MINOR \
            -Dotau_example_VERSION_PATCH=$VERSION_PATCH
