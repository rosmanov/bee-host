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
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "beectl";
          version = "1.4.2-1";

          src = ./.;

          nativeBuildInputs = with pkgs; [ cmake git cacert ];

          buildInputs = with pkgs; [ ];

          # Set SSL certificates for git to clone dependencies
          NIX_SSL_CERT_FILE = "${pkgs.cacert}/etc/ssl/certs/ca-bundle.crt";

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
          ];

          meta = with pkgs.lib; {
            description = "Native Messaging Host for the Bee Browser Extension <https://github.com/rosmanov/chrome-bee>";
            homepage = "https://github.com/rosmanov/bee-host";
            license = licenses.mit;
            platforms = platforms.linux;
            maintainers = [ ];
          };
        };

        apps.default = {
          type = "app";
          program = "${self.packages.${system}.default}/usr/local/bin/beectl";
        };
      });
}
