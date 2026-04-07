#ifndef serpent_vm_h
#define serpent_vm_h

#include <stdint.h>
#include "table/table.h"
#include "value/value.h"

struct ObjFunction;

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_MAX)

typedef struct {
    struct ObjFunction* function;
    uint8_t* ip;
    Value* slots;
} CallFrame;

// Sanal Makine Yapısı
typedef struct VM {
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    // Yığın (Stack) - LIFO (Son giren ilk çıkar) geçici hesaplamalar için
    Value stack[STACK_MAX];
    Value* stackTop; // Yığının en üstünü işaret eder

    Table strings; // Intern edilmiş string'ler
    Table globals; // Global değişkenler
    struct Obj* objects; // Heap'teki tüm objelerin başı
} VM;

// Programın çalışma sonuçları
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM(void);
void freeVM(void);

// Şimdilik sadece bytecode chunk çalıştırıyoruz, 
// daha sonra "interpret" fonksiyonuna kaynak kod stringi vereceğiz.
InterpretResult interpret(const char* source);

void push(Value value);
Value pop(void);

extern VM vm;

#endif
