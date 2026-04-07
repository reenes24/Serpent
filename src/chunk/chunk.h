#ifndef serpent_chunk_h
#define serpent_chunk_h

#include "common/common.h"
#include "value/value.h"

// OpCode: Sanal makinenin anlayacağı talimatların (instruction) numaraları
typedef enum {
    OP_CONSTANT, // Sabit sayıyı yığına ekle
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,      // Toplama
    OP_SUBTRACT, // Çıkarma
    OP_MULTIPLY, // Çarpma
    OP_DIVIDE,   // Bölme
    OP_MOD,      // Modulo (%)
    OP_NOT,      // !x mantıksal değil
    OP_NEGATE,   // Negatif yapma (-x)
    OP_BIT_AND,  // &
    OP_BIT_OR,   // |
    OP_BIT_XOR,  // ^
    OP_BIT_NOT,  // ~
    OP_LSHIFT,   // <<
    OP_RSHIFT,   // >>
    OP_DEFINE_GLOBAL, // Global değişken tanımla
    OP_GET_GLOBAL,    // Global değişken değerini al
    OP_SET_GLOBAL,    // Global değişken değerini güncelle
    OP_GET_LOCAL,     // Yerel değişken değerini al
    OP_SET_LOCAL,     // Yerel değişken değerini güncelle
    OP_JUMP,          // Şartsız atlama
    OP_JUMP_IF_FALSE, // Şartlı atlama (yanlışsa)
    OP_LOOP,          // Geriye atlama (döngü için)
    OP_CALL,          // Fonksiyon çağrısı
    OP_ADDR_LOCAL,    // &x (local)
    OP_ADDR_GLOBAL,   // &x (global)
    OP_DEREF,         // *ptr (read)
    OP_STORE_DEREF,   // *ptr = val (write)
    OP_POP,           // Yığının en üstündeki değeri at (ifade sonu)
    OP_PRINT,         // Ekrana yazdır
    OP_RETURN,        // Fonksiyondan dön
} OpCode;

// Chunk (Parça): Bir dizi bytecode komutunu ve veriyi temsil eder
typedef struct Chunk {
    int count;
    int capacity;
    uint8_t* code; // Bytecode array
    int* lines;    // Her bir opcode'un kaynak koddaki satır numarası (hata raporlamak için)
    ValueArray constants; // Bu chunk'taki sabit değerlerin havuzu
} Chunk;

void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);
void freeChunk(Chunk* chunk);

#endif
