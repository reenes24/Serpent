#ifndef serpent_wayland_h
#define serpent_wayland_h

#include "common/common.h"
#include "value/value.h"

typedef struct SerpentWindow SerpentWindow;

// Native Serpent Fonksiyonları (VM tarafından çağrılacak)
Value wlCreateNative(int argCount, Value* args);
Value wlGetBufferNative(int argCount, Value* args);
Value wlCommitNative(int argCount, Value* args);
Value wlPollNative(int argCount, Value* args);
Value wlCloseNative(int argCount, Value* args);

// Yeni Fonksiyonlar
Value wlDrawRectNative(int argCount, Value* args);
Value wlDrawTextNative(int argCount, Value* args);
Value wlDrawLineNative(int argCount, Value* args);
Value wlDrawCircleNative(int argCount, Value* args);

// Fare durum fonksiyonları
Value wlGetMouseXNative(int argCount, Value* args);
Value wlGetMouseYNative(int argCount, Value* args);
Value wlGetMouseBtnNative(int argCount, Value* args);

// GUI Bileşenleri
Value wlButtonNative(int argCount, Value* args);
Value wlTextBoxNative(int argCount, Value* args);

#endif
