# shell.nix
{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  # Specify dependencies
  buildInputs = with pkgs; [
    gcc
    sfml
  ];

  # Shell hook to display the compile and run command
  shellHook = ''
    echo ""
    echo "# To compile and run Game.cpp with SFML, use the following command:"
    echo "g++ -o app Game.cpp -lsfml-graphics -lsfml-window -lsfml-system && ./app"
    echo ""
  '';
}
