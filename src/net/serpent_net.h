#ifndef serpent_net_h
#define serpent_net_h

#include "common/common.h"
#include "value/value.h"

// Native Serpent Network Functions
Value httpGetNative(int argCount, Value* args);
Value httpPostNative(int argCount, Value* args);

#endif
