{ pkgs, fetchurl, stdenv }:

let
    version = "0.17.0";
    sdkFile = "zephyr-sdk-${version}_linux-x86_64.tar.xz";
    baseUri = "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${version}";
    sha256 = "e7536b4879f689cfd4ef9c1939069da6c4cf0e3030a2940175d6354e7b8b69e1";
in

    stdenv.mkDerivation {
        pname = "zephyr-sdk";
        inherit version;

        src = fetchurl {
            url = "${baseUri}/${sdkFile}";
            sha256 = sha256;
        };

        sourceRoot = ".";

        nativeBuildInputs = [ pkgs.wget pkgs.xz pkgs.file pkgs.which pkgs.cacert ];
        
        dontUseCmakeConfigure = true;
        
        unpackPhase = ''
            tar xvf $src
        '';

        buildPhase = ''
            cd zephyr-sdk-${version}
            ./setup.sh -t arm-zephyr-eabi
        '';

        installPhase = ''
            mkdir -p $out
            cp -r * $out/
        '';
    }
