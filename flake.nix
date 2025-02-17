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
                #pythonPackages.west
                pythonPackages.pip
                pythonPackages.setuptools
                pythonPackages.wheel
                pythonPackages.tkinter
#                 patoolib
                pythonPackages.pyelftools
# dev tools 
                git
                openssh
                tree
#                 libmagic
            ];

# TODO: make a better approach to this, check better zephyr , maybe from west
#  need pyenv because it make easier to deal with west

        shellHook = ''
            export HOME=$PWD/.nix-home
            mkdir -p $HOME
            if [ -! -d $HOME/zephyrproject ]; then
                python -m venv ~/zephyrproject/.venv
                source ~/zephyrproject/.venv/bin/activate
                pip install west
                west init ~/zephyrproject
                cd ~/zephyrproject
                west update
                west zephyr-export
                west packages pip --intall
                cd ~/zephyrproject/zephyr
                west skd install
            else
                source ~/zephyrproject/.venv/bin/activate
                cd ~/zephyrproject
                west update
            fi
        '';
        };
    };
}
