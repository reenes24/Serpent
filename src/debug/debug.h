#ifndef serpent_debug_h
#define serpent_debug_h

#include "chunk/chunk.h"

// Chunk içeriğini okunaklı bir şekilde ekrana yazdırır
void disassembleChunk(Chunk* chunk, const char* name);

// Tek bir komutu (instruction) okuyup ekrana yazdırır
int disassembleInstruction(Chunk* chunk, int offset);

#endif
