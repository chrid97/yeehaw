#!/bin/bash

source ~/third-party/emsdk/emsdk_env.sh

emcc -o build/yeehaw.html \
  src/platform_web.c src/game.c \
  -s USE_GLFW=3 \
  -s ASYNCIFY \
  -s INITIAL_MEMORY=64MB \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  -s FORCE_FILESYSTEM=1 \
  -DPLATFORM_WEB \
  -I./lib/raylib/src \
  ./lib/raylib/src/libraylib.web.a \
  --preload-file assets
