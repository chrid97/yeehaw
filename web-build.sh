#!/bin/bash

# emcc -o build/game.html src/main.c -Os -Wall \
#   /home/chris/repos/raylib/src/libraylib.a \
#   -I./src \
#   -I/home/chris/repos/raylib/src \
#   -L/home/chris/repos/raylib/src \
#   -s USE_GLFW=3 \
#   -s ASYNCIFY \
#   -s INITIAL_MEMORY=64MB \
#   -s ALLOW_MEMORY_GROWTH=1 \
#   -DPLATFORM_WEB
#

emcc -o build/game.html src/main.c \
  -s USE_GLFW=3 \
  -s ASYNCIFY \
  -s INITIAL_MEMORY=64MB \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
  -DPLATFORM_WEB \
  -I/home/chris/repos/raylib/src \
  /home/chris/repos/raylib/src/libraylib.a \
  --preload-file assets \
  --preload-file maps
