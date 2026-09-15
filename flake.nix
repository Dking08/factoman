{
  description = "FactoMan - Factorio management tool";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      packages = forAllSystems (system:
        let
          pkgs = import nixpkgs {
            inherit system;
          };

          # FactoMan uses libcurl's WebSocket API (wss://).
          # Nixpkgs disables WebSocket support in curl by default.
          curlWebsocket = pkgs.curl.override {
            websocketSupport = true;
          };
        in
        {
          default = pkgs.stdenv.mkDerivation {
            pname = "factoman";
            version = "1.0.0";

            src = ./.;

            nativeBuildInputs = with pkgs; [
              cmake
              ninja
              pkg-config
            ];

            buildInputs = [
              pkgs.fltk
              curlWebsocket
              pkgs.nlohmann_json

              # FLTK/X11 dependencies
              pkgs.libX11
              pkgs.libXext
              pkgs.libXinerama
              pkgs.libXcursor
              pkgs.libXfixes
              pkgs.libXrender
              pkgs.libXft
              pkgs.libXrandr

              # OpenGL
              pkgs.libGL
              pkgs.libGLU
            ];

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
            ];

            installPhase = ''
              mkdir -p $out/bin
              cp factoman $out/bin/
            '';

            meta = {
              description = "Factorio management tool";
              homepage = "https://github.com/";
              platforms = nixpkgs.lib.platforms.linux;
              mainProgram = "factoman";
            };
          };
        });

      apps = forAllSystems (system:
        let
          package = self.packages.${system}.default;
        in
        {
          default = {
            type = "app";
            program = "${package}/bin/factoman";
          };
        });

      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs {
            inherit system;
          };

          curlWebsocket = pkgs.curl.override {
            websocketSupport = true;
          };
        in
        {
          default = pkgs.mkShell {
            packages = [
              pkgs.cmake
              pkgs.ninja
              pkgs.pkg-config

              pkgs.fltk
              curlWebsocket
              pkgs.nlohmann_json

              # FLTK/X11 dependencies
              pkgs.libX11
              pkgs.libXext
              pkgs.libXinerama
              pkgs.libXcursor
              pkgs.libXfixes
              pkgs.libXrender
              pkgs.libXft
              pkgs.libXrandr

              # OpenGL
              pkgs.libGL
              pkgs.libGLU
            ];
          };
        });
    };
}
