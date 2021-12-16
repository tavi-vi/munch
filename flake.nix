
{
  inputs.flake-utils.url = "github:numtide/flake-utils";

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem
      (system:
        let pkgs = nixpkgs.legacyPackages.${system}; in
        {
          defaultPackage = pkgs.stdenv.mkDerivation {
            name = "munch";
            src = self;
            buildInputs = [ pkgs.SDL2 ];
          };
        }
      );
}
