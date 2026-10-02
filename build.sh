#!/bin/bash
# ============================================================
#  Compila o libldwrapper.so para Android
# ============================================================

set -e

# ---------- Configuração ----------
# Ajuste o caminho do NDK se necessário
NDK="${ANDROID_NDK_HOME:-$HOME/android-ndk-r24}"
TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/linux-aarch64"
CLANG="$TOOLCHAIN/bin/aarch64-linux-android21-clang++"

# ---------- Verificações ----------
if [ ! -f "$CLANG" ]; then
    echo "✗ Clang não encontrado em: $CLANG"
    echo "  Defina ANDROID_NDK_HOME ou ajuste o caminho no script."
    exit 1
fi

# ---------- Compilação ----------
echo "→ Compilando libldwrapper.so..."
$CLANG \
    -fPIC -pie -O2 \
    -Wl,-dynamic-linker,/system/bin/linker64 \
    -o libldwrapper.so \
    main.cpp

# ---------- Resultado ----------
echo ""
echo "✓ Compilado com sucesso!"
echo ""
file libldwrapper.so
ls -lh libldwrapper.so
echo ""
echo "Próximo passo:"
echo "  cp libldwrapper.so <seu-projeto>/jniLibs/arm64-v8a/"