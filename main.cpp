// ============================================================
//  ldwrapper — Executa o LLD com o nome correto (argv[0])
//
//  Uso no Android: o LLD (linker do LLVM) detecta em qual modo
//  deve operar pelo nome do arquivo que o invoca:
//
//    ld.lld      → modo Unix/Linux  (o que queremos)
//    ld64.lld    → modo macOS
//    lld-link    → modo Windows
//    wasm-ld     → modo WebAssembly
//
//  Quando o LLD é empacotado num APK Android, ele precisa ser
//  renomeado para "libldreal.so" para que o Android o extraia
//  para nativeLibraryDir/. Esse nome quebra a detecção de modo,
//  fazendo o LLD reclamar:
//
//    "lld is a generic driver. Invoke ld.lld (Unix), ..."
//
//  Este wrapper resolve o problema: ele é executado como
//  "libldwrapper.so", localiza o "libldreal.so" na mesma pasta,
//  e o executa passando argv[0]="ld.lld".
//
//  Compilação:
//    aarch64-linux-android21-clang++ \
//        -fPIC -pie -O2 \
//        -Wl,-dynamic-linker,/system/bin/linker64 \
//        -o libldwrapper.so main.cpp
//
//  Licença: MIT
// ============================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>

// ------------------------------------------------------------
//  Localiza o binário "alvo" (libldreal.so) a partir do caminho
//  do próprio wrapper.
//
//  Ex: /data/app/.../lib/arm64/libldwrapper.so
//      → /data/app/.../lib/arm64/libldreal.so
// ------------------------------------------------------------
static bool encontrarAlvo(char* destino, size_t tam) {
    ssize_t n = readlink("/proc/self/exe", destino, tam - 1);
    if (n <= 0) {
        return false;
    }
    destino[n] = '\0';

    // Substitui o nome do arquivo pelo alvo
    char* ultimaBarra = strrchr(destino, '/');
    if (!ultimaBarra) {
        return false;
    }

    strcpy(ultimaBarra + 1, "libldreal.so");
    return true;
}

// ------------------------------------------------------------
//  Executa o binário alvo passando argv[0]="ld.lld".
//
//  O argv[0] é o que o LLD usa para decidir o modo. Como
//  nossa lib foi renomeada para "libldreal.so", precisamos
//  corrigir aqui.
// ------------------------------------------------------------
static int executar(const char* alvo, int argc, char** argv) {
    // Aloca espaço para: "ld.lld" + args originais (sem o argv[0] do wrapper) + NULL
    char** novoArgv = (char**) malloc(sizeof(char*) * (argc + 1));
    if (!novoArgv) {
        fprintf(stderr, "[ldwrapper] malloc falhou\n");
        return 1;
    }

    novoArgv[0] = (char*) "ld.lld";
    for (int i = 1; i < argc; i++) {
        novoArgv[i] = argv[i];
    }
    novoArgv[argc] = nullptr;

    execv(alvo, novoArgv);

    // Se chegou aqui, execv falhou
    fprintf(stderr, "[ldwrapper] execv falhou em '%s': %s\n",
            alvo, strerror(errno));
    free(novoArgv);
    return 127;
}

// ------------------------------------------------------------
//  Main
// ------------------------------------------------------------
int main(int argc, char** argv) {
    // Desabilita buffer em stderr/stdout — garante que as
    // mensagens de erro apareçam mesmo se o processo crashar
    setvbuf(stderr, nullptr, _IONBF, 0);
    setvbuf(stdout, nullptr, _IONBF, 0);

    // Modo debug (opcional): se LDWRAPPER_VERBOSE=1, mostra tudo
    bool verbose = getenv("LDWRAPPER_VERBOSE") != nullptr;

    // Encontra o binário alvo (libldreal.so)
    char alvo[4096];
    if (!encontrarAlvo(alvo, sizeof(alvo))) {
        fprintf(stderr, "[ldwrapper] não foi possível localizar o alvo\n");
        return 1;
    }

    if (verbose) {
        fprintf(stderr, "[ldwrapper] alvo: %s\n", alvo);
        fprintf(stderr, "[ldwrapper] argc: %d\n", argc);
        for (int i = 0; i < argc; i++) {
            fprintf(stderr, "[ldwrapper] argv[%d]=%s\n", i, argv[i]);
        }
    }

    // Verifica se o alvo existe
    if (access(alvo, F_OK) != 0) {
        fprintf(stderr, "[ldwrapper] alvo não existe: %s\n", alvo);
        return 66; // EX_NOINPUT
    }

    // Executa
    return executar(alvo, argc, argv);
}