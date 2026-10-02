
https://img.shields.io/badge/license-MIT-blue.svg https://img.shields.io/badge/platform-Android-green.svg https://img.shields.io/badge/language-C%2B%2B-blue.svg
# ldwrapper

Um wrapper minimalista que resolve um problema específico do **LLD no Android**: o linker do LLVM detecta em qual modo deve operar pelo `argv[0]`, mas quando ele é empacotado num APK, o arquivo precisa ser renomeado para `lib*.so` — e isso quebra a detecção.

## O problema

O [LLD](https://lld.llvm.org/) é o linker do projeto LLVM. Ele é um **driver genérico** que se comporta de forma diferente dependendo do nome pelo qual é invocado:

| `argv[0]` | Modo |
|---|---|
| `ld.lld` | Unix/Linux ✅ |
| `ld64.lld` | macOS |
| `lld-link` | Windows |
| `wasm-ld` | WebAssembly |

No Android, para um binário ser extraído do APK para `nativeLibraryDir/`, ele **precisa** se chamar `lib*.so`. Ou seja:

- `ld.lld` → precisa virar `libldreal.so`
- Mas aí o LLD reclama: `lld is a generic driver. Invoke ld.lld (Unix)...`

## A solução

Este wrapper:

1. É executado como `libldwrapper.so` (nome válido pro Android extrair)
2. Localiza o `libldreal.so` na mesma pasta
3. Executa ele passando `argv[0] = "ld.lld"`

Resultado: o LLD entra em modo Unix normalmente, sem reclamar.

## Compilação

Requer o [Android NDK](https://developer.android.com/ndk):

```bash
aarch64-linux-android21-clang++ \
    -fPIC -pie -O2 \
    -Wl,-dynamic-linker,/system/bin/linker64 \
    -o libldwrapper.so main.cpp
```

Ou simplesmente:

```bash
./build.sh
```

Uso

1. Coloque libldwrapper.so em jniLibs/arm64-v8a/ do seu projeto
2. Renomeie o LLD real (ld.lld) para libldreal.so e coloque na mesma pasta
3. No código Java, invoque:

```java
File linker = new File(nativeDir, "libldwrapper.so");

List<String> cmd = new ArrayList<>();
cmd.add(clang);
cmd.add("-fuse-ld=" + linker.getAbsolutePath());
// ... resto dos argumentos
```

Por que não usar /system/bin/linker64 diretamente?

Você pode, mas precisa que o binário alvo tenha argv[0] correto — o que exige uma camada extra de manipulação que o linker64 não fornece de forma limpa. Este wrapper faz isso de forma explícita e portável.

Flags de debug

Defina a variável de ambiente LDWRAPPER_VERBOSE=1 para ver o que o wrapper está fazendo:

```bash
LDWRAPPER_VERBOSE=1 ./libldwrapper.so --version
```

Compatibilidade

· ✅ Android 5.0+ (API 21)
· ✅ ARM64 (aarch64-linux-android)
· ✅ ARM32 (armv7a-linux-androideabi)
· ✅ x86, x86_64

Licença

MIT — veja LICENSE.

```

---

## 3. `LICENSE` (MIT)

```markdown
MIT License

Copyright (c) 2026 MaikoTS

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

4. build.sh

```bash
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
```

Depois roda:

```bash
chmod +x build.sh
```

---

5. .gitignore

```gitignore
# Binários compilados
*.so
*.o
*.a

# Arquivos temporários
*.tmp
*.swp
*~

# IDE
.vscode/
.idea/
*.iml

# Sistema
.DS_Store
Thumbs.db
```

---

6. Makefile (opcional, para quem prefere)

```makefile
# ============================================================
#  Makefile para compilar o ldwrapper
# ============================================================

NDK ?= $(HOME)/android-ndk-r24
TOOLCHAIN = $(NDK)/toolchains/llvm/prebuilt/linux-aarch64
CLANG = $(TOOLCHAIN)/bin/aarch64-linux-android21-clang++

CFLAGS = -fPIC -pie -O2
LDFLAGS = -Wl,-dynamic-linker,/system/bin/linker64

TARGET = libldwrapper.so
SOURCE = main.cpp

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CLANG) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all clean
```

Uso:

```bash
make          # compila
make clean    # limpa
```

---