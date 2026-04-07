#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "common/common.h"
#include "compiler/compiler.h"
#include "memory/memory.h"
#include "object/object.h"
#include "table/table.h"
#include "vm/vm.h"
#include "wayland/serpent_wayland.h"
#include "net/serpent_net.h"

// Tekil (Singleton) VM nesnesi
VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    resetStack();
}

static Value clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static Value allocNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) return NIL_VAL;
    size_t size = (size_t)AS_NUMBER(args[0]);
    void* ptr = malloc(size);
    return POINTER_VAL(ptr);
}

static Value freeNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_POINTER(args[0])) return NIL_VAL;
    free(AS_POINTER(args[0]));
    return NIL_VAL;
}

static Value sizeofNative(int argCount, Value* args) {
    if (argCount != 1) return NUMBER_VAL(0);
    return NUMBER_VAL(sizeof(Value));
}

static void defineNative(const char* name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable(&vm.strings);
    initTable(&vm.globals);

    defineNative("clock", clockNative);
    defineNative("alloc", allocNative);
    defineNative("free", freeNative);
    defineNative("sizeof", sizeofNative);

    // Wayland Native Functions
    defineNative("wl_create", wlCreateNative);
    defineNative("wl_get_buffer", wlGetBufferNative);
    defineNative("wl_commit", wlCommitNative);
    defineNative("wl_poll", wlPollNative);
    defineNative("wl_close", wlCloseNative);
    defineNative("wl_draw_rect", wlDrawRectNative);
    defineNative("wl_draw_text", wlDrawTextNative);
    defineNative("wl_draw_line", wlDrawLineNative);
    defineNative("wl_draw_circle", wlDrawCircleNative);

    defineNative("wl_get_mouse_x", wlGetMouseXNative);
    defineNative("wl_get_mouse_y", wlGetMouseYNative);
    defineNative("wl_get_mouse_btn", wlGetMouseBtnNative);
    defineNative("wl_button", wlButtonNative);
    defineNative("wl_textbox", wlTextBoxNative);

    defineNative("http_get", httpGetNative);
    defineNative("http_post", httpPostNative);
}

void freeVM() {
    freeTable(&vm.strings);
    freeTable(&vm.globals);
    freeObjects();
}

static bool call(ObjFunction* function, int argCount) {
    if (argCount != function->arity) {
        runtimeError("Expected %d arguments but got %d.",
                     function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_FUNCTION:
                return call(AS_FUNCTION(callee), argCount);
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
            default:
                break;
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int length = a->length + b->length;
    char* chars = ALLOCATE_ARRAY(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    push(OBJ_VAL(result));
}

static InterpretResult run() {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
    (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(valueType, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
    } while (false)

#define BITWISE_OP(op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        uint64_t b = (uint64_t)AS_NUMBER(pop()); \
        uint64_t a = (uint64_t)AS_NUMBER(pop()); \
        push(NUMBER_VAL((double)(a op b))); \
    } while (false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(&frame->function->chunk,
                               (int)(frame->ip - frame->function->chunk.code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: push(READ_CONSTANT()); break;
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: {
                if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(BOOL_VAL(a > b));
                } else if (IS_POINTER(peek(0)) && IS_POINTER(peek(1))) {
                    uintptr_t b_addr = (uintptr_t)AS_POINTER(pop());
                    uintptr_t a_addr = (uintptr_t)AS_POINTER(pop());
                    push(BOOL_VAL(a_addr > b_addr));
                } else {
                    runtimeError("Operands must be two numbers or two pointers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_LESS: {
                if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(BOOL_VAL(a < b));
                } else if (IS_POINTER(peek(0)) && IS_POINTER(peek(1))) {
                    uintptr_t b_addr = (uintptr_t)AS_POINTER(pop());
                    uintptr_t a_addr = (uintptr_t)AS_POINTER(pop());
                    push(BOOL_VAL(a_addr < b_addr));
                } else {
                    runtimeError("Operands must be two numbers or two pointers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b_num = AS_NUMBER(pop());
                    double a_num = AS_NUMBER(pop());
                    push(NUMBER_VAL(a_num + b_num));
                } else if (IS_STRING(peek(1)) && IS_NUMBER(peek(0))) {
                    // String + Number [str, num]
                    double num = AS_NUMBER(pop());
                    ObjString* str = AS_STRING(pop());
                    char buf[32];
                    int len = snprintf(buf, sizeof(buf), "%g", num);
                    push(OBJ_VAL(str)); // Push A
                    push(OBJ_VAL(copyString(buf, len))); // Push B
                    concatenate(); // A + B = str + num
                } else if (IS_NUMBER(peek(1)) && IS_STRING(peek(0))) {
                    // Number + String [num, str]
                    ObjString* str = AS_STRING(pop());
                    double num = AS_NUMBER(pop());
                    char buf[32];
                    int len = snprintf(buf, sizeof(buf), "%g", num);
                    push(OBJ_VAL(copyString(buf, len))); // Push A
                    push(OBJ_VAL(str)); // Push B
                    concatenate(); // A + B = num + str
                } else if (IS_POINTER(peek(1)) && IS_NUMBER(peek(0))) {
                    int offset = (int)AS_NUMBER(pop());
                    uintptr_t base_addr = (uintptr_t)AS_POINTER(pop());
                    push(POINTER_VAL((void*)(base_addr + offset)));
                } else {
                    runtimeError("Operands must be numbers, strings, or pointer + number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: {
                if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b_num = AS_NUMBER(pop());
                    double a_num = AS_NUMBER(pop());
                    push(NUMBER_VAL(a_num - b_num));
                } else if (IS_POINTER(peek(1)) && IS_NUMBER(peek(0))) {
                    int offset = (int)AS_NUMBER(pop());
                    uintptr_t base_addr = (uintptr_t)AS_POINTER(pop());
                    push(POINTER_VAL((void*)(base_addr - offset)));
                } else {
                    runtimeError("Operands must be numbers or pointer - number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
            case OP_MOD: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    runtimeError("Operands must be numbers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(fmod(a, b)));
                break;
            }
            case OP_NOT:      push(BOOL_VAL(isFalsey(pop()))); break;
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }
            case OP_BIT_AND: BITWISE_OP(&); break;
            case OP_BIT_OR:  BITWISE_OP(|); break;
            case OP_BIT_XOR: BITWISE_OP(^); break;
            case OP_LSHIFT:  BITWISE_OP(<<); break;
            case OP_RSHIFT:  BITWISE_OP(>>); break;
            case OP_BIT_NOT: {
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL((double)(~(uint64_t)AS_NUMBER(pop()))));
                break;
            }
            case OP_PRINT:
                printValue(pop());
                printf("\n");
                break;
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name); 
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_POP: pop(); break;
            case OP_CALL: {
                int argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_ADDR_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(POINTER_VAL(&frame->slots[slot]));
                break;
            }
            case OP_ADDR_GLOBAL: {
                ObjString* name = READ_STRING();
                Value* ptr = tableGetPtr(&vm.globals, name);
                if (ptr == NULL) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(POINTER_VAL(ptr));
                break;
            }
            case OP_DEREF: {
                Value ptrVal = pop();
                if (!IS_POINTER(ptrVal)) {
                    runtimeError("Can only dereference pointers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value* ptr = (Value*)AS_POINTER(ptrVal);
                push(*ptr);
                break;
            }
            case OP_STORE_DEREF: {
                Value val_to_store = peek(0);
                Value ptr_val = peek(1);
                if (!IS_POINTER(ptr_val)) {
                    runtimeError("Can only dereference pointers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value* target_ptr = (Value*)AS_POINTER(ptr_val);
                *target_ptr = val_to_store;
                pop(); // val
                pop(); // ptrVal
                push(val_to_store);
                break;
            }
            case OP_RETURN: {
                Value result = pop();
                vm.frameCount--;
                if (vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
#undef BITWISE_OP
}

InterpretResult interpret(const char* source) {
    ObjFunction* function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));
    call(function, 0);

    return run();
}
