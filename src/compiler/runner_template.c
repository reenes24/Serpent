#include <stdio.h>
#include <stdlib.h>
#include "vm/vm.h"

// Bu veri builder.c tarafından hex-encoded olarak sağlanacak
extern const char* _serpent_source;

int main(int argc, const char* argv[]) {
    initVM();
    
    InterpretResult result = interpret(_serpent_source);
    
    freeVM();

    if (result == INTERPRET_COMPILE_ERROR) return 65;
    if (result == INTERPRET_RUNTIME_ERROR) return 70;

    return 0;
}
