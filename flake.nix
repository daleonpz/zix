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
{
    description = "Flake for Zephyr development";

    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs/nixos-24.11";
    };

    outputs = { self, nixpkgs }:
    
    let 
        system = "x86_64-linux";
        pkgs = import nixpkgs { system = system; };
        python = pkgs.python311;
        pythonPackages = pkgs.python311Packages;
    in
    {
        devShells.${system}.default = pkgs.mkShell {
            name = "zephyr-dev";
            buildInputs = with pkgs; [
                cmake
                ninja
                gperf
                ccache
                dfu-util
                dtc 
                wget
                curl
                python
                xz
                file
                gnumake
                gcc
                gcc_multi
                SDL2
                openocd
                which # used in zephyr sdk setup.sh
                cacert # used in zephyr sdk
                pythonPackages.pip
                pythonPackages.setuptools
                pythonPackages.wheel
                pythonPackages.tkinter
                pythonPackages.pyelftools
                # mcuboot
                pythonPackages.click
                pythonPackages.cryptography
                pythonPackages.intelhex
                pythonPackages.cbor2
                pythonPackages.pyyaml
                pythonPackages.pytest
                # dev tools 
                git
                openssh
                tree
            ];

        shellHook = ''
            if [ ! -d .env ]; then
                python -m venv .env
            fi 
            source .env/bin/activate
            export HOME=$(pwd)
            pip install west
            if [ -d ".west" ]; then
              echo ".zephyr already initialized"
            else
              echo "Initializing zephyr"
              west init
            fi
            west update
            sh ~/scripts/install_zephyr_sdk.sh
            sh ~/scripts/install_stm32.sh
            sh ~/scripts/install_power_profiler.sh
        '';
        };
    };
}
