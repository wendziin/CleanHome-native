# Imagem de build do CleanHome (capp nativo C/GLES2) para Tizen 6.0 armv7.
#
# arm32v5/debian:buster-slim: glibc 2.28 e gcc arm-linux-gnueabi (softfp), a mesma
# ABI de chamada do Tizen. O binario e linkado contra o rootstrap Tizen 6.0
# (glibc 2.30), entao roda em qualquer TV Tizen 6.0 ou mais nova.
FROM --platform=linux/arm/v5 arm32v5/debian:buster-slim

# buster saiu de deb.debian.org; os indices ficam no arquivo.
RUN sed -i 's#deb.debian.org#archive.debian.org#g; /security\.debian\.org/d' /etc/apt/sources.list && \
    apt-get -qq -o Acquire::Check-Valid-Until=false update && \
    apt-get -qq -o Acquire::Check-Valid-Until=false install -y --no-install-recommends \
      build-essential pkg-config ca-certificates xz-utils file zip unzip curl && \
    rm -rf /var/lib/apt/lists/*

ENV CFLAGS="-O2 -fPIC -march=armv7-a -mfloat-abi=softfp -mfpu=neon" \
    CXXFLAGS="-O2 -fPIC -march=armv7-a -mfloat-abi=softfp -mfpu=neon"
