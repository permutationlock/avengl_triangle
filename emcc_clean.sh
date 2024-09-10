#! /bin/bash
./build -cc "emcc" -ldflags "-s ASYNCIFY -s USE_GLFW=3" \
    -exext ".html .js .wasm" -no-glfw -syslibs "" clean
make clean
