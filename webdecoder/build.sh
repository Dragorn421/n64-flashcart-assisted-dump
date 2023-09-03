#!/bin/bash
set -ev

set +v
export EMSDK_QUIET=1
# setup emsdk: https://emscripten.org/docs/getting_started/downloads.html
source "/home/dragorn421/Documents/emsdk/emsdk_env.sh"
export EMSDK_QUIET=0
set -v

mkdir -p build

emcc heavylifting_main.c \
-o build/heavylifting_main.js \
-sINITIAL_MEMORY=200015872 \
-s'EXPORTED_RUNTIME_METHODS=["cwrap"]' \
-s'EXPORTED_FUNCTIONS=["_malloc","_free"]'

emcc heavylifting_imgdec.c build/zbar/zbar/.libs/libzbar.a \
-o build/heavylifting_imgdec.js \
-sBUILD_AS_WORKER=1 \
-s'EXPORTED_FUNCTIONS=["_heavylifting_decode_image"]'

rm -rf website
mkdir -p website

cp -r --target-directory=website *.html *.css *.js jsmodules build/*.js build/*.wasm
