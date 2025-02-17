
{
    description = "Flake for Zephyr development";

    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs/nixos-24.11";
#         zephyr.url = "github:zephyrproject-rtos/zephyr";
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
                pythonPackages.west
#                 patoolib
                pythonPackages.pyelftools
# dev tools 
                git
                openssh
                tree
#                 libmagic
            ];
        };
    };
}
