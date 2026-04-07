#ifndef serpent_compiler_h
#define serpent_compiler_h

#include <stdbool.h>
#include "object/object.h"
#include "vm/vm.h"

// Kaynak kodunu alır, derler ve ObjFunction döner.
ObjFunction* compile(const char* source);

#endif
