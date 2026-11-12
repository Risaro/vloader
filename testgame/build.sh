#!/bin/sh
set -e
NDK="${ANDROID_NDK:-$HOME/Android/Sdk/ndk/27.2.12479018}"
CC="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi21-clang"
DIR="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$DIR/out"
"$CC" -shared -fPIC -O2 -mthumb -o "$DIR/out/libgame.so" "$DIR/game.c" -lGLESv2 -llog -lm
cp "$DIR/profile.txt" "$DIR/out/profile.txt"
printf 'privet s karty pamyati\n' > "$DIR/out/hello.txt"
ls -la "$DIR/out"
