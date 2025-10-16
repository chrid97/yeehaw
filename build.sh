#!/bin/bash

mkdir -p build/

if [ ! -f ./lib/raylib/src/libraylib.a ]; then
  echo "Building Raylib library..."
  (cd lib/raylib/src && make PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=STATIC)
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
  echo "Building for macOS..."

  clang -g -Wall -Wextra \
    src/platform.c \
    -I./lib/raylib/src/ \
    ./lib/raylib/src/libraylib.a \
    -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo -framework OpenGL \
    -o ./build/horse-riding

  # Hot reloadable game library
  #  This tells the compiler not to resolve raylib symbols that
  #  it'll exist at runtime
  #  -undefined dynamic_lookup
  clang -g -Wall -Wextra -dynamiclib -fPIC \
    src/main.c \
    -I./lib/raylib/src/ \
    -undefined dynamic_lookup \
    -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo -framework OpenGL \
    -o ./build/game.dylib
else
  echo "Building for Linux..."

  gcc ./src/main.c -g \
    -I./lib/raylib/include \
    -L./lib/raylib/lib -Wl,-rpath=\$ORIGIN/../lib/raylib/lib \
    -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 \
    -o ./build/horse-riding
fi
