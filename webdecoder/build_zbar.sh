export EMSDK_QUIET=1
# setup emsdk: https://emscripten.org/docs/getting_started/downloads.html
source "/home/dragorn421/Documents/emsdk/emsdk_env.sh"
export EMSDK_QUIET=0

mkdir -p build/zbar
cd build/zbar

emconfigure ../../zbar/configure --enable-codes=qrcode --disable-video --without-x --without-xshm --without-dbus --without-jpeg --without-npapi --without-gtk --without-gir --without-python --without-qt --without-java

emmake make
