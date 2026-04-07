#ifndef serpent_table_h
#define serpent_table_h

#include "common/common.h"
#include "value/value.h"

typedef struct {
    ObjString* key;
    Value value;
} Entry;

typedef struct Table {
    int count;
    int capacity;
    Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);

// Yeni: Pointer desteği için değerin adresini döner
Value* tableGetPtr(Table* table, ObjString* key);

#endif
