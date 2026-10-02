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