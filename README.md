![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Android-green.svg)
![Language](https://img.shields.io/badge/language-C%2B%2B-blue.svg)
![Build](https://github.com/MAIKOTS/ldwrapper/actions/workflows/build.yml/badge.svg)
# ldwrapper

Um wrapper minimalista que resolve um problema específico do **LLD no Android**: o linker do LLVM detecta em qual modo deve operar pelo `argv[0]`, mas quando ele é empacotado num APK, o arquivo precisa ser renomeado para `lib*.so` — e isso quebra a detecção.

## Por que isso existe

Se você está compilando C/C++ para Android **dentro do próprio app** (sem PC), 
precisa rodar o `clang` e o `lld` a partir de `nativeLibraryDir/`. Mas o 
Android só extrai arquivos com nome `lib*.so` — e o LLD depende do nome do 
arquivo para saber em qual modo operar.

Este wrapper resolve esse conflito.
##

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

## Como funciona

```
┌────────────────────────────────────────┐
│  1. ProcessBuilder invoca libldwrapper.so             
│     (que está em nativeLibraryDir/)                  
└────────────────────────────────────────┘
                              ↓     
┌────────────────────────────────────────┐
│  2. ldwrapper lê /proc/self/exe                       
│     → descobre que está em .../lib/arm64/           
└────────────────────────────────────────┘
                              ↓
┌────────────────────────────────────────┐
│  3. Monta o caminho do alvo:                               
│     .../lib/arm64/libldreal.so                             
└────────────────────────────────────────┘
                              ↓
┌────────────────────────────────────────┐
│  4. execv(alvo, ["ld.lld", ...args])                       
│     ^^^^^^^^  argv[0] correto para o LLD                   
└────────────────────────────────────────┘
                              ↓
┌────────────────────────────────────────┐
│  5. LLD detecta "ld.lld" → modo Unix ✅                    
└────────────────────────────────────────┘
```

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

## Compatibilidade
```
• ✅ Android 5.0+ (API 21)
```
```
• ✅ ARM64 (aarch64-linux-android)
```
```
• ✅ ARM32 (armv7a-linux-androideabi)
```
```
• ✅ x86, x86_64
```

## Uso rápido (Java)

java
String nativeDir = ctx.getApplicationInfo().nativeLibraryDir;

List<String> cmd = Arrays.asList(
    nativeDir + "/libclang.so",
    "-shared", "-fPIC", "-O2",
    "--target=aarch64-linux-android21",
    "-fuse-ld=" + nativeDir + "/libldwrapper.so",
    "-o", saida.getAbsolutePath(),
    fonte.getAbsolutePath()
);

ProcessBuilder pb = new ProcessBuilder(cmd);
pb.environment().put("LD_LIBRARY_PATH", nativeDir);
Process p = pb.start();
int exit = p.waitFor();
##

Requisitos:

1. `libldwrapper.so` e `libldreal.so` em `jniLibs/arm64-v8a/`
2. Renomear `ld.lld` do NDK para `libldreal.so` antes de copiar
3. Definir `LD_LIBRARY_PATH` para `nativeLibraryDir`

## Como usar em Java/Kotlin

### 1. Colocar os arquivos no projeto

Copie os dois binários para `jniLibs/arm64-v8a/` do seu projeto:

```
app/src/main/jniLibs/arm64-v8a/
├── libldwrapper.so    ← este wrapper
└── libldreal.so       ← o LLD real (renomeado de "ld.lld")
```

> **Importante:** o arquivo `ld.lld` original do NDK deve ser **renomeado** para `libldreal.so` — senão o Android não extrai para `nativeLibraryDir/`.

### 2. Carregar a lib no início do app (opcional, mas recomendado)

Para garantir que o Android extraia os `.so` na primeira execução:

```java
static {
    try {
        System.loadLibrary("ldreal");
        System.loadLibrary("ldwrapper");
    } catch (Throwable t) {
        Log.w("App", "Falha ao carregar libs nativas", t);
    }
}
```

### 3. Invocar o linker

#### Java

```java
import android.content.Context;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public void invocarLinker(Context ctx, File fonte, File saida) throws IOException {
    // 1. Localiza os binários em nativeLibraryDir
    String nativeDir = ctx.getApplicationInfo().nativeLibraryDir;
    File clang     = new File(nativeDir, "libclang.so");
    File ldwrapper = new File(nativeDir, "libldwrapper.so");

    // 2. Monta o comando
    List<String> cmd = new ArrayList<>();
    cmd.add(clang.getAbsolutePath());
    cmd.add("-x"); cmd.add("c++");
    cmd.add("-shared");
    cmd.add("-fPIC");
    cmd.add("-O2");
    cmd.add("-std=c++17");
    cmd.add("--target=aarch64-linux-android21");

    // ★ Aponta pro wrapper (que passa argv[0]="ld.lld" pro LLD real)
    cmd.add("-fuse-ld=" + ldwrapper.getAbsolutePath());

    cmd.add("-o"); cmd.add(saida.getAbsolutePath());
    cmd.add(fonte.getAbsolutePath());

    // 3. Executa com LD_LIBRARY_PATH configurado
    ProcessBuilder pb = new ProcessBuilder(cmd);
    pb.redirectErrorStream(true);
    pb.environment().put("LD_LIBRARY_PATH", nativeDir);

    Process processo = pb.start();

    // 4. Captura a saída
    try (BufferedReader reader = new BufferedReader(
            new InputStreamReader(processo.getInputStream()))) {
        String linha;
        while ((linha = reader.readLine()) != null) {
            Log.i("Linker", linha);
        }
    }

    int exit = processo.waitFor();
    Log.i("Linker", "Exit code: " + exit);
}
```

#### Kotlin

```kotlin
import android.content.Context
import java.io.BufferedReader
import java.io.File
import java.io.InputStreamReader

fun invocarLinker(ctx: Context, fonte: File, saida: File) {
    val nativeDir = ctx.applicationInfo.nativeLibraryDir
    val clang     = File(nativeDir, "libclang.so")
    val ldwrapper = File(nativeDir, "libldwrapper.so")

    val cmd = listOf(
        clang.absolutePath,
        "-x", "c++",
        "-shared",
        "-fPIC",
        "-O2",
        "-std=c++17",
        "--target=aarch64-linux-android21",
        "-fuse-ld=${ldwrapper.absolutePath}",  // ★ wrapper
        "-o", saida.absolutePath,
        fonte.absolutePath
    )

    val pb = ProcessBuilder(cmd)
    pb.redirectErrorStream(true)
    pb.environment()["LD_LIBRARY_PATH"] = nativeDir

    val processo = pb.start()

    BufferedReader(InputStreamReader(processo.inputStream)).use { reader ->
        reader.forEachLine { Log.i("Linker", it) }
    }

    val exit = processo.waitFor()
    Log.i("Linker", "Exit code: $exit")
}
```

### 4. Saída esperada

- **Sucesso:** `Exit code: 0` e o arquivo `.so` gerado no caminho especificado
- **Falha:** `Exit code: 1` com mensagens de erro do LLD no logcat

## Como saber se funcionou

Se você ver o erro:

```
lld is a generic driver.
Invoke ld.lld (Unix), ld64.lld (macOS), lld-link (Windows) instead
```

Significa que o wrapper **não está sendo usado** — o LLD real foi chamado direto sem corrigir o `argv[0]`. Verifique se o caminho passado em `-fuse-ld=` aponta para o **`libldwrapper.so`** (e não para o `libldreal.so`).

## Alternativas

Se você usa **CMake** em vez de `ProcessBuilder`:

```cmake
set(CMAKE_LINKER "${CMAKE_ANDROID_NDK}/../nativeLibraryDir/libldwrapper.so")
```

Ou se preferir **linha de comando direta** (via shell):

```bash
LD_LIBRARY_PATH=/path/to/nativeLibraryDir \
    /path/to/nativeLibraryDir/libldwrapper.so \
    --version
```

## Limitações

- ⚠️ O wrapper assume que `libldreal.so` está **na mesma pasta** que `libldwrapper.so`. Se você mover um sem o outro, ele falha.
- ⚠️ Definir `LD_LIBRARY_PATH` é **obrigatório** para o LLD achar as libs dele (`libc++_shared.so`, etc).
- ⚠️ Não funciona como **symlink** — precisa ser uma **cópia real** do arquivo.
