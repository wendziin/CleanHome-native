#!/bin/bash
# Gera o TPK do CleanHome para Tizen 6.0 (TVs 2021+).
#
#   bash tools/tpk.sh
#
# Precisa de Docker (o CI tem). O cache fica em ~/.cache/cleanhome-tpk:
# rootstrap Tizen 6.0 (76 MB) e os fontes SDL2/FreeType, baixados uma vez.
#
# Por que nao Tizen Studio: o perfil TV nativo nao e publico (o Studio 6.0 so
# publica mobile/wearable), mas o rootstrap mobile-6.0-device.core e a mesma
# base de sistema do Tizen 6.0 da TV. O app so usa app framework + Elm + Evas
# GL, entao compilar contra ele gera um capp valido. Compilar dentro de um
# Debian armel (glibc 2.28) mantem o binario carregavel em glibc 2.30.
#
# O TPK sai SEM assinatura: o Apps2Samsung assina no momento da instalacao.
# Com Tizen Studio no PATH e TIZEN_PROFILE definido, `tizen package` assina
# aqui (util se voce tem certificado Samsung com o DUID da TV).
set -euo pipefail
cd "$(dirname "$0")/.."

CACHE="${CLEANHOME_TPK_CACHE:-$HOME/.cache/cleanhome-tpk}"
VER="${CLEANHOME_VERSION:-0.1.0}"
IMAGE="${CLEANHOME_TPK_IMAGE:-cleanhome-tpk:6.0}"
RS_ZIP=mobile-6.0-rs-device.core_0.0.123_ubuntu-64.zip
RS_URL="https://download.tizen.org/sdk/tizenstudio/official/binary/$RS_ZIP"
RS="$CACHE/rs/data/platforms/tizen-6.0/mobile/rootstraps/mobile-6.0-device.core"
OUT=build/tpk
STAGE="$OUT/stage"

mkdir -p "$CACHE/src" "$OUT"

baixar_tar() { # url ext
  local nome d
  nome=$(basename "$1")
  d="$CACHE/src/${nome%.tar.*}"
  if [ -d "$d" ]; then return; fi
  echo "[deps] $nome"
  curl -fsSL "$1" | tar "x$2" -C "$CACHE/src"
}

if [ ! -d "$RS" ]; then
  echo "[rootstrap] Tizen 6.0 ($RS_ZIP)"
  [ -f "$CACHE/$RS_ZIP" ] || curl -fsSL "$RS_URL" -o "$CACHE/$RS_ZIP.part"
  [ -f "$CACHE/$RS_ZIP" ] || mv "$CACHE/$RS_ZIP.part" "$CACHE/$RS_ZIP"
  mkdir -p "$CACHE/rs"
  unzip -qo "$CACHE/$RS_ZIP" -d "$CACHE/rs"
fi
test -d "$RS/usr/lib" || { echo "rootstrap invalido em $RS" >&2; exit 1; }

baixar_tar https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-2.30.9.tar.gz z
baixar_tar https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.2/SDL2_image-2.8.2.tar.gz z
baixar_tar https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.22.0/SDL2_ttf-2.22.0.tar.gz z
baixar_tar https://download-mirror.savannah.gnu.org/releases/freetype/freetype-2.13.3.tar.xz J

echo "[imagem] $IMAGE"
docker build -q -t "$IMAGE" tools/tpk

if [ ! -f "$CACHE/prefix/lib/libSDL2_ttf.a" ]; then
  echo "[deps] SDL2/SDL2_image/SDL2_ttf/FreeType estaticos (primeira vez)"
  # deps.sh vive no repo, nao no cache: monta tools/tpk so para leitura.
  docker run --rm --platform linux/arm/v5 \
    -v "$CACHE:/w" -v "$PWD/tools/tpk:/tools/tpk:ro" "$IMAGE" \
    bash /tools/tpk/deps.sh
fi

echo "[build] compilando contra o sysroot Tizen 6.0"
docker run --rm --platform linux/arm/v5 \
  -v "$CACHE:/w" -v "$CACHE/prefix:/deps:ro" -v "$RS:/rs:ro" \
  -v "$PWD:/work" -w /work "$IMAGE" bash /work/tools/tpk/compile.sh

test -x "$OUT/bin/cleanhome" || { echo "bin/cleanhome nao gerado" >&2; exit 1; }

echo "[stage] montando o pacote"
rm -rf "$STAGE"
mkdir -p "$STAGE/bin" "$STAGE/res" "$STAGE/shared/res"
cp "$OUT/bin/cleanhome" "$STAGE/bin/cleanhome"
sed "s/ version=\"[^\"]*\">/ version=\"$VER\">/" tizen-manifest.xml > "$STAGE/tizen-manifest.xml"
cp -R assets "$STAGE/res/assets"
cp shared/res/icon.png "$STAGE/shared/res/icon.png"

TPK="$PWD/$OUT/cleanhome-$VER.tpk"
rm -f "$TPK"
if [ -n "${TIZEN_PROFILE:-}" ] && command -v tizen >/dev/null 2>&1; then
  echo "[tpk] assinando com o perfil $TIZEN_PROFILE"
  mkdir -p "$OUT/assinado"
  tizen package -t tpk -s "$TIZEN_PROFILE" -o "$PWD/$OUT/assinado" -- "$STAGE"
  mv "$OUT/assinado"/*.tpk "$TPK"
else
  echo "[tpk] sem assinatura (TIZEN_PROFILE nao definido): o Apps2Samsung assina na instalacao"
  # O zip roda na imagem (que tem zip); o host do CI nem sempre tem.
  docker run --rm --platform linux/arm/v5 -e VER="$VER" -v "$PWD:/work" "$IMAGE" \
    sh -c 'cd /work/build/tpk/stage && zip -qr "/work/build/tpk/cleanhome-$VER.tpk" .'
fi

test -f "$OUT/cleanhome-$VER.tpk" || { echo "TPK nao gerado" >&2; exit 1; }
echo "=== $OUT/cleanhome-$VER.tpk ($(du -h "$TPK" | cut -f1))"
unzip -l "$TPK" | grep -E 'tizen-manifest.xml|bin/cleanhome|shared/res/icon.png' || true
unzip -l "$TPK" | tail -1
