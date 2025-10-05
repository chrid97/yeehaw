#!/bin/bash

mkdir -p build/

if [[ "$OSTYPE" == "darwin"* ]]; then
  # macOS settings
  eval cc ./src/main.c $(pkg-config --libs --cflags raylib) -o ./build/horse-riding
else
  gcc ./src/main.c -g \
    -I./lib/raylib/include \
    -L./lib/raylib/lib -Wl,-rpath=\$ORIGIN/../lib/raylib/lib \
    -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 \
    -o ./build/horse-riding
fi
