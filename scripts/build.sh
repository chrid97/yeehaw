#!/bin/bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

cd $PROJECT_ROOT
mkdir -p build/

# Ensure raylib is built for system platform
if [ ! -f ./lib/raylib/src/libraylib.a ]; then
  echo "📦 Building Raylib library..."
  (cd lib/raylib/src && make PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=STATIC)
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
  echo "🍎 Building for macOS..."

  clang -g -Wall -Wextra \
    src/platform.c \
    -I./lib/raylib/src/ \
    ./lib/raylib/src/libraylib.a \
    -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo -framework OpenGL \
    -o ./build/horse-riding
else
  echo "🐧 Building for Linux..."

  gcc ./src/platform.c -g \
    -I./lib/raylib/src \
    ./lib/raylib/src/libraylib.a \
    -lglfw -lGL -lopenal -lm -lpthread -ldl -lrt -lX11 \
    -rdynamic \
    -o ./build/horse-riding
fi

bash scripts/build_game.sh

echo "✅ Build successful!"
