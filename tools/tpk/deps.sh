# Dependencias estaticas do TPK. Roda DENTRO da imagem tools/tpk/Dockerfile,
# com o cache montado em /w e os fontes em /w/src (tools/tpk.sh baixa).
#
# SDL2 entra sem nenhum driver: no Tizen o video vem do Elm GLView, e o SDL so
# e usado por SDL_image/SDL2_ttf (decodificar PNG/JPG e rasterizar TTF). Assim
# o app nao depende de libSDL2 do firmware.
set -e
P=/w/prefix
HOST=--host=arm-linux-gnueabi
mkdir -p "$P"

cd /w/src/SDL2-2.30.9
[ -f "$P/lib/libSDL2.a" ] || {
  ./configure $HOST -q --prefix="$P" --enable-static --disable-shared \
    --disable-audio --disable-video-x11 --disable-video-wayland \
    --disable-video-kmsdrm --disable-video-vulkan --disable-video-opengl \
    --disable-video-opengles --disable-joystick --disable-haptic --disable-sensor \
    --disable-hidapi --disable-pulseaudio --disable-alsa --disable-dbus \
    --disable-ime --disable-ibus --disable-fcitx --disable-libudev \
    --disable-video-rpi --disable-render --disable-sndio --disable-pipewire \
    --disable-jack --disable-esd --disable-arts --disable-nas --disable-oss
  make -j"$(nproc)" >/dev/null
  make install >/dev/null
}

# FreeType estatica, sem zlib/png: as fontes do app sao TTF puro.
cd /w/src/freetype-2.13.3
[ -f "$P/lib/libfreetype.a" ] || {
  ./configure $HOST -q --prefix="$P" --enable-static --disable-shared \
    --without-zlib --without-bzip2 --without-png --without-brotli --without-harfbuzz
  make -j"$(nproc)" >/dev/null
  make install >/dev/null
}

# stb_image cobre os PNG/JPG de assets/ (16 jpg + 25 png), sem libpng/libjpeg.
cd /w/src/SDL2_image-2.8.2
[ -f "$P/lib/libSDL2_image.a" ] || {
  PKG_CONFIG_PATH="$P/lib/pkgconfig" ./configure $HOST -q --prefix="$P" \
    --enable-static --disable-shared --enable-stb-image \
    --disable-jpg-shared --disable-png-shared --disable-webp \
    --disable-avif --disable-jxl --disable-tif --disable-qoi
  make -j"$(nproc)" >/dev/null
  make install >/dev/null
}

cd /w/src/SDL2_ttf-2.22.0
[ -f "$P/lib/libSDL2_ttf.a" ] || {
  PKG_CONFIG_PATH="$P/lib/pkgconfig" ./configure $HOST -q --prefix="$P" \
    --enable-static --disable-shared --disable-freetype-builtin --disable-harfbuzz
  make -j"$(nproc)" >/dev/null
  make install >/dev/null
}

ls -l "$P/lib"/*.a
