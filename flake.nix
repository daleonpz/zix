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
                python
                xz
                file
                gnumake
                gcc
                gcc_multi
                SDL2
                which # used in zephyr sdk setup.sh
                cacert # used in zephyr sdk
                pythonPackages.pip
                pythonPackages.setuptools
                pythonPackages.wheel
                pythonPackages.tkinter
                pythonPackages.pyelftools
                # dev tools 
                git
                openssh
                tree
                minicom
            ];

        shellHook = ''
            if [ ! -d .env ]; then
                python -m venv .env
            fi 
            source .env/bin/activate
            export HOME=$(pwd)
            pip install west
            west init
            west update
            sh scripts/install_zephyr_sdk.sh

            sh scripts/install_stm32.sh
        '';
        };
    };
}
