{
  description = "BeeCtl - Native Messaging Host for Browser's External Editor";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        # Build libuv with static library enabled
        libuv-static = pkgs.libuv.overrideAttrs (oldAttrs: {
          dontDisableStatic = true;
          cmakeFlags = (oldAttrs.cmakeFlags or []) ++ [
            "-DBUILD_SHARED_LIBS=OFF"
          ];
        });

        # Build cJSON with static library enabled
        cjson-static = pkgs.cjson.overrideAttrs (oldAttrs: {
          dontDisableStatic = true;
          cmakeFlags = (oldAttrs.cmakeFlags or []) ++ [
            "-DBUILD_SHARED_AND_STATIC_LIBS=ON"
            "-DBUILD_SHARED_LIBS=OFF"
          ];
        });
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "beectl";
          version = "1.4.2-1";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            pkg-config
          ];

          buildInputs = [
            libuv-static
            cjson-static
          ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
            "-DUSE_SYSTEM_DEPS=ON"
            "-DBEECTL_BIN_DIR=bin"
            "-DBEECTL_CHROME_MANIFEST_DIR=etc/opt/chrome/native-messaging-hosts"
            "-DBEECTL_CHROMIUM_MANIFEST_DIR=etc/chromium/native-messaging-hosts"
            "-DBEECTL_FIREFOX_MANIFEST_DIR=lib/mozilla/native-messaging-hosts"
            "-DBEECTL_FIREFOX_MANIFEST_DIR_ALT="
          ];

          meta = with pkgs.lib; {
            description = "Native Messaging Host for the Bee Browser Extension <https://github.com/rosmanov/chrome-bee>";
            homepage = "https://github.com/rosmanov/bee-host";
            license = licenses.mit;
            platforms = platforms.linux ++ platforms.darwin;
            maintainers = [ ];
          };
        };

        apps.default = {
          type = "app";
          program = "${self.packages.${system}.default}/bin/beectl";
        };
      });
}
