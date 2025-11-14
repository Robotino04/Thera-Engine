let
  pkgs = import <nixpkgs> {};
in
  pkgs.mkShell {
    packages = [
      pkgs.cargo
      pkgs.clippy
      pkgs.rustc
      pkgs.cargo-watch
    ];
  }
