{
  description = "Development environment for C++ NES Emulator";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      system = "aarch64-darwin";
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        name = "NESemu";

        nativeBuildInputs = with pkgs; [
          cmake
          gnumake
          pkg-config
          llvmPackages_18.clang
        ];

        buildInputs = with pkgs; [
          SDL2
          SDL2_image
          SDL2_ttf
          libiconv
        ];

      };
    };
}
