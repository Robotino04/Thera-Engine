let
  pkgs = import <nixpkgs> {};
in
  pkgs.mkShell {
    packages = [
      pkgs.cargo
      pkgs.clippy
      pkgs.rustc
      pkgs.cargo-watch
      pkgs.samply

      pkgs.cmake
      pkgs.llvmPackages.clang-tools
      pkgs.llvmPackages.clang
      pkgs.python3

      pkgs.stockfish
      pkgs.cutechess
    ];
  }
