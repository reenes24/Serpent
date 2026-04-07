#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler/builder.h"

// Kaynak kodunu bir C string değişkeni olarak dışa aktarır
static void writeSourceToC(const char* sourcePath, const char* outCFile) {
    FILE* src = fopen(sourcePath, "r");
    FILE* dst = fopen(outCFile, "w");

    if (!src || !dst) {
        fprintf(stderr, "Hata: Dosya islemi basarisiz.\n");
        exit(1);
    }

    fprintf(dst, "const char* _serpent_source = \"");
    
    int c;
    while ((c = fgetc(src)) != EOF) {
        if (c == '\\') fprintf(dst, "\\\\");
        else if (c == '\"') fprintf(dst, "\\\"");
        else if (c == '\n') fprintf(dst, "\\n");
        else if (c == '\r') fprintf(dst, "\\r");
        else if (c == '\t') fprintf(dst, "\\t");
        else fputc(c, dst);
    }

    fprintf(dst, "\";\n");

    fclose(src);
    fclose(dst);
}

void compileToBinary(const char* sourcePath, const char* outPath) {
    const char* tempC = "_serpent_app_data.c";
    writeSourceToC(sourcePath, tempC);

    printf("Derleniyor: %s -> %s\n", sourcePath, outPath);

    // GCC Komutu: Runner + Embedded Source + VM Sources (main.c haric)
    char command[4096];
    snprintf(command, sizeof(command),
        "gcc -O3 -I src "
        "src/compiler/runner_template.c %s "
        "src/chunk/chunk.c src/debug/debug.c src/memory/memory.c "
        "src/value/value.c src/vm/vm.c src/scanner/scanner.c "
        "src/compiler/compiler.c src/object/object.c src/table/table.c "
        "src/wayland/serpent_wayland.c "
        "src/wayland/xdg-shell-protocol.c "
        "src/wayland/xdg-decoration-protocol.c "
        "src/net/serpent_net.c "
        "-lwayland-client -lcurl -lm "
        "-o %s",
        tempC, outPath);

    int result = system(command);

    if (result == 0) {
        printf("Basarili! Executable olusturuldu: %s\n", outPath);
    } else {
        fprintf(stderr, "Hata: GCC derleme sirasinda hata verdi.\n");
    }

    // Gecici dosyayi temizle
    remove(tempC);
}
