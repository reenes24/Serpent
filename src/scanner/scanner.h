#ifndef serpent_scanner_h
#define serpent_scanner_h

typedef enum {
    // Tek karakterlik semboller
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR, TOKEN_PERCENT,

    // Bir veya iki karakterli semboller
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_AMPERSAND, TOKEN_PIPE, TOKEN_CARET, TOKEN_TILDE,
    TOKEN_LEFT_SHIFT, TOKEN_RIGHT_SHIFT,

    // Temel tipler
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

    // Anahtar Kelimeler (Keywords)
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

    TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start; // Tokenin koddaki başlangıç konumu
    int length;        // Tokenin uzunluğu
    int line;          // Bulunduğu satır (Hata ayıklama için)
} Token;

void initScanner(const char* source);
Token scanToken(void);

#endif
