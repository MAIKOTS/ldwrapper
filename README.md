Claro — aqui está o conteúdo corrigido do README.md para substituir:

```md
# ldwrapper

Um wrapper minimalista que resolve um problema específico do LLD no Android: o linker do LLVM detecta em qual modo deve operar pelo `argv[0]`, mas quando ele é empacotado num APK, o arquivo precisa ser renomeado para `lib*.so` — e isso quebra a detecção.

## O problema

O [LLD](https://lld.llvm.org/) é o linker do projeto LLVM. Ele é um driver genérico que se comporta de forma diferente dependendo do nome pelo qual é invocado:

| `argv[0]` | Modo |
|---|---|
| `ld.lld` | Unix/Linux ✅ |
| `ld64.lld` | macOS |
| `lld-link` | Windows |
| `wasm-ld` | WebAssembly |

No Android, para um binário ser extraído do APK para `nativeLibraryDir/`, ele precisa se chamar `lib*.so`. Ou seja:

- `ld.lld` → precisa virar `libldreal.so`
- Mas aí o LLD reclama: `lld is a generic driver. Invoke ld.lld (Unix)...`

## A solução

Este wrapper:

1. É executado como `libldwrapper.so` (nome válido para Android extrair)
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

## Uso

1. Coloque `libldwrapper.so` em `jniLibs/arm64-v8a/` do seu projeto
2. Renomeie o LLD real (`ld.lld`) para `libldreal.so` e coloque na mesma pasta
3. No código Java, invoque:

```java
File linker = new File(nativeDir, "libldwrapper.so");

List<String> cmd = new ArrayList<>();
cmd.add(clang);
cmd.add("-fuse-ld=" + linker.getAbsolutePath());
// ... resto dos argumentos
```

### Por que não usar `/system/bin/linker64` diretamente?

Você pode, mas precisa que o binário alvo tenha `argv[0]` correto — o que exige uma camada extra de manipulação que o linker64 não fornece de forma limpa. Este wrapper faz isso de forma explícita e portável.

## Flags de debug

Defina a variável de ambiente `LDWRAPPER_VERBOSE=1` para ver o que o wrapper está fazendo:

```bash
LDWRAPPER_VERBOSE=1 ./libldwrapper.so --version
```

## Compatibilidade

- ✅ Android 5.0+ (API 21)
- ✅ ARM64 (`aarch64-linux-android`)
- ✅ ARM32 (`armv7a-linux-androideabi`)
- ✅ x86, x86_64

## Licença

MIT — veja [LICENSE](LICENSE).
```

Se quiser, também posso te mandar uma versão mais “profissional” do README, com:
- exemplo de estrutura do projeto
- instruções de instalação mais detalhadas
- seção “Como funciona internamente”
- links para uso com CMake/Gradle.