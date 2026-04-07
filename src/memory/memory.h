#ifndef serpent_memory_h
#define serpent_memory_h

#include "common/common.h"

// Makro: Kapasiteyi büyütmek için. 0 ise 8 yap, değilse 2 katına çıkar.
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

// Makro: Yeni bir dizi tahsis eder
#define ALLOCATE_ARRAY(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

// Makro: Bellek tahsisini yeniden boyutlandırır
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

// Makro: Bellek dizisini serbest bırakır
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

// Makro: Tek bir nesneyi serbest bırakır
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

void freeObjects(void);

#endif
