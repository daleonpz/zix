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

        nixHome = "$PWD/.nix-home";
        zephyrDir = "~/zephyrproject";
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
                pythonPackages.pip
                pythonPackages.setuptools
                pythonPackages.wheel
                pythonPackages.tkinter
                pythonPackages.pyelftools
                # dev tools 
                git
                openssh
                tree
            ];

        shellHook = ''
            export HOME=${nixHome}
            mkdir -p ${nixHome}
            if [ -! -d ${zephyrDir} ]; then
                python -m venv ${zephyrDir}/.venv
                source ${zephyrDir}/.venv/bin/activate
                pip install west
                west init ${zephyrDir} && cd ${zephyrDir}
                west update
                west zephyr-export
                west packages pip --install
                cd ${zephyrDir}/zephyr
                west skd install
            else
                source ${zephyrDir}/.venv/bin/activate
                cd ${zephyrDir}
                west update
            fi
        '';
        };
    };
}
