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
          ];

          postInstall = ''
            # Fix hardcoded paths in CMakeLists.txt for NixOS
            if [ -d "$out/usr/local/bin" ]; then
              mkdir -p "$out/bin"
              mv "$out/usr/local/bin/beectl" "$out/bin/"
              rmdir -p "$out/usr/local/bin" || true
            fi
            if [ -d "$out/usr/lib/mozilla" ]; then
              mkdir -p "$out/lib/mozilla"
              mv "$out/usr/lib/mozilla/native-messaging-hosts" "$out/lib/mozilla/"
              rmdir -p "$out/usr/lib/mozilla" || true
            fi
            if [ -d "$out/usr/lib64" ]; then
              rm -rf "$out/usr/lib64"
            fi
            if [ -d "$out/usr" ]; then
              rmdir -p "$out/usr" || true
            fi
          '';

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
