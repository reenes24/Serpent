#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/common.h"
#include "chunk/chunk.h"
#include "debug/debug.h"
#include "vm/vm.h"
#include "scanner/scanner.h"
#include "compiler/builder.h"

// Geliştirme aşaması için sadece tokenleri yazdıracak geçici run fonksiyonu
static void run(const char* source) {
    InterpretResult result = interpret(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

static void repl() {
    char line[1024];
    for (;;) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        run(line);
    }
}

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Dosya acilamadi: \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Dosya okunurken bellek ayrilamadi.\n");
        exit(74);
    }
    
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Dosya okunamadi: \"%s\".\n", path);
        exit(74);
    }
    
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static void runFile(const char* path) {
    char* source = readFile(path);
    run(source);
    free(source);
}

int main(int argc, const char* argv[]) {
    // Derleme modu kontrolü
    if (argc >= 3 && strcmp(argv[1], "-c") == 0) {
        const char* sourcePath = argv[2];
        const char* outPath = "app"; // Varsayılan isim

        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                outPath = argv[i+1];
                break;
            }
        }

        compileToBinary(sourcePath, outPath);
        return 0;
    }

    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Kullanim:\n");
        fprintf(stderr, "  Calistir: serpent [dosya]\n");
        fprintf(stderr, "  Derle:    serpent -c [dosya] -o [cikis]\n");
        exit(64);
    }
    
    freeVM();

    return 0;
}
