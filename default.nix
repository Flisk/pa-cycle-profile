with import <nixpkgs> {};

stdenv.mkDerivation rec {
  name = "pa-cycle-profile";
  version = "git";

  src = ./.;

  nativeBuildInputs = [ pkgconfig ];
  buildInputs = [ pulseaudio ];

  makeFlags = [ "DESTDIR=$(out) PREFIX=" ];
}
