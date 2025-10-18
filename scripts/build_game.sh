#!/bin/bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

cd $PROJECT_ROOT

if [[ "$OSTYPE" == "darwin"* ]]; then
  # Hot reloadable game library
  #  This tells the compiler not to resolve raylib symbols that
  #  it'll exist at runtime
  #  -undefined dynamic_lookup
  clang -g -Wall -Wextra -dynamiclib -fPIC \
    src/game.c \
    -I./lib/raylib/src/ \
    -undefined dynamic_lookup \
    -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo -framework OpenGL \
    -o ./build/game.dylib
else
  # gcc was trying to read the file before it was done writting, so I create a tmp file and move it once its done writing
  gcc ./src/game.c -g -fPIC -shared \
    -I./lib/raylib/src \
    -o ./build/game.so.tmp && mv ./build/game.so.tmp ./build/game.so
fi
