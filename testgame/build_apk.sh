#!/bin/sh
set -e
SDK="${ANDROID_HOME:-$HOME/Android/Sdk}"
BT="$(ls -d "$SDK"/build-tools/* | sort -V | tail -1)"
JAR="$(ls -d "$SDK"/platforms/android-* | sort -V | tail -1)/android.jar"
DIR="$(cd "$(dirname "$0")" && pwd)"
sh "$DIR/build.sh" >/dev/null
rm -rf "$DIR/out/apk" && mkdir -p "$DIR/out/apk/lib/armeabi-v7a" "$DIR/out/apk/assets"
"$BT/aapt2" compile --dir "$DIR/apk/res" -o "$DIR/out/apk/res.zip"
"$BT/aapt2" link -o "$DIR/out/testgame.apk" -I "$JAR" --manifest "$DIR/apk/AndroidManifest.xml" "$DIR/out/apk/res.zip"
cp "$DIR/out/libgame.so" "$DIR/out/apk/lib/armeabi-v7a/"
cp "$DIR/out/hello.txt" "$DIR/out/apk/assets/"
(cd "$DIR/out/apk" && zip -q -r ../testgame.apk lib assets)
ls -la "$DIR/out/testgame.apk"
