#!/bin/bash
# ============================================================
#  Compila o libldwrapper.so para Android
# ============================================================

set -e

# ---------- Detecta o NDK ----------
if [ -z "$ANDROID_NDK_HOME" ]; then
    if [ -n "$ANDROID_NDK_LATEST_HOME" ]; then
        ANDROID_NDK_HOME="$ANDROID_NDK_LATEST_HOME"
    else
        echo "✗ ANDROID_NDK_HOME não definido."
        echo "  Defina a variável de ambiente ou ajuste este script."
        exit 1
    fi
fi

# ---------- Detecta a plataforma do host ----------
UNAME_S=$(uname -s)
UNAME_M=$(uname -m)

case "$UNAME_S" in
    Linux)
        case "$UNAME_M" in
            aarch64|arm64) HOST_TAG="linux-aarch64" ;;
            x86_64|amd64)  HOST_TAG="linux-x86_64"  ;;
            *)
                echo "✗ Arquitetura de host não suportada: $UNAME_M"
                exit 1
                ;;
        esac
        ;;
    Darwin)
        case "$UNAME_M" in
            arm64)  HOST_TAG="darwin-arm64"  ;;
            x86_64) HOST_TAG="darwin-x86_64" ;;
            *)
                echo "✗ Arquitetura macOS não suportada: $UNAME_M"
                exit 1
                ;;
        esac
        ;;
    *)
        echo "✗ Sistema operacional não suportado: $UNAME_S"
        exit 1
        ;;
esac

# ---------- Localiza o clang ----------
TOOLCHAIN="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$HOST_TAG"
CLANG="$TOOLCHAIN/bin/aarch64-linux-android21-clang++"

echo "→ Plataforma detectada: $HOST_TAG"
echo "→ Toolchain:            $TOOLCHAIN"

if [ ! -f "$CLANG" ]; then
    echo ""
    echo "✗ Clang não encontrado em: $CLANG"
    echo "  Verifique se o NDK está instalado corretamente."
    exit 1
fi

# ---------- Compilação ----------
echo ""
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
