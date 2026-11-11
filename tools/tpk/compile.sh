# Compila o CleanHome para Tizen 6.0 armv7 dentro da imagem tools/tpk/Dockerfile.
#
# Montagens esperadas:
#   /rs   rootstrap mobile-6.0-device.core (somente leitura)
#   /deps prefixo com SDL2/SDL2_image/SDL2_ttf/FreeType estaticos
#   /work repositorio (fontes em /work/src, saida em /work/build/tpk)
#
# Nao usa pkg-config: os .pc do rootstrap puxam cadeias quebradas (evas ->
# emile -> libjpeg.pc, que o rootstrap nao publica). Os headers e as libs do
# Tizen sao referenciados direto, como no project_def.prop.
set -eu

R=/rs
D=/deps
OUT=/work/build/tpk
OBJ="$OUT/obj"
mkdir -p "$OUT/bin" "$OBJ"

test -d "$R/usr/lib" || { echo "rootstrap ausente em $R" >&2; exit 1; }
test -f "$D/lib/libSDL2_ttf.a" || { echo "deps estaticas ausentes em $D" >&2; exit 1; }

# Headers do EFL ficam em *-1 (e em *-1/<lib>); appfw e dlog tem diretorio
# proprio. Da raiz do rootstrap entram so os headers que a libc do container
# nao tem (tizen.h e afins): incluir a raiz inteira poria o limits.h/unistd.h do
# Tizen na frente dos do gcc e o preprocessamento quebraria.
SHIM=/tmp/tizen-inc
rm -rf $SHIM && mkdir -p $SHIM
for h in "$R"/usr/include/*.h; do
  b=$(basename "$h")
  [ -e "/usr/include/$b" ] || ln -sf "$h" "$SHIM/$b"
done
# <GLES2/gl2.h> e <KHR/khrplatform.h> sao incluidos com o diretorio na frente.
for d in GLES2 KHR; do
  [ -d "$R/usr/include/$d" ] && ln -sfn "$R/usr/include/$d" "$SHIM/$d"
done

INC="-I$SHIM"
for d in "$R"/usr/include/*-1 "$R"/usr/include/*-1/*/ \
         "$R/usr/include/appfw" "$R/usr/include/dlog"; do
  [ -d "$d" ] && INC="$INC -I$d"
done

CFLAGS="-O2 -fPIC -march=armv7-a -mfloat-abi=softfp -mfpu=neon -std=c99 \
-Wall -Wextra -Wno-unused-parameter -DCLEANHOME_TIZEN \
-I/work/src -I$D/include -I$D/include/SDL2 $INC"

# Libs do firmware referenciadas pelo caminho: os nomes diferem do -l esperado
# (libecore_evas.so, nao libecore-evas.so) e assim uma ausencia falha na hora.
TI_LIBS=""
for l in libelementary libevas libcapi-appfw-application libcapi-appfw-app-manager \
         libcapi-appfw-app-control libcapi-appfw-app-common libdlog libGLESv2; do
  if [ ! -f "$R/usr/lib/$l.so" ]; then
    echo "lib $l.so ausente no rootstrap" >&2
    exit 1
  fi
  TI_LIBS="$TI_LIBS $R/usr/lib/$l.so"
done

SRCS="main_tizen platform_tizen render text image launcher config"

# dash nao tem arrays: CFLAGS nao tem espacos dentro de um argumento, entao o
# word splitting normal serve.
for s in $SRCS; do
  # shellcheck disable=SC2086
  arm-linux-gnueabi-gcc $CFLAGS -c "/work/src/$s.c" -o "$OBJ/$s.o"
done

OBJS=""
for s in $SRCS; do OBJS="$OBJS $OBJ/$s.o"; done

# shellcheck disable=SC2086
arm-linux-gnueabi-gcc -o "$OUT/bin/cleanhome" $OBJS \
  -L"$D/lib" -Wl,--start-group -lSDL2_ttf -lSDL2_image -lSDL2 -lfreetype -Wl,--end-group \
  -L"$R/usr/lib" -L"$R/lib" \
  -Wl,-rpath-link,"$R/usr/lib" -Wl,-rpath-link,"$R/lib" $TI_LIBS \
  -lm -ldl -lpthread -lrt

if ! readelf -h "$OUT/bin/cleanhome" | grep -q 'ARM'; then
  echo "binario nao e ARM:" >&2
  file "$OUT/bin/cleanhome" >&2
  exit 1
fi

echo "=== bin/cleanhome"
ls -l "$OUT/bin/cleanhome"
echo "--- NEEDED"
objdump -p "$OUT/bin/cleanhome" | grep NEEDED
echo "--- glibc maxima exigida"
objdump -T "$OUT/bin/cleanhome" | grep -oE 'GLIBC_[0-9.]+' | sort -uV | tail -1
