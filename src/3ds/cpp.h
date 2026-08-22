//code from https://github.com/skyfloogle/red-viper
#pragma once

#ifdef __3DS__
#include <3ds/types.h>
#include <3ds/services/hid.h>

Result cppInit(void);
void cppExit(void);
bool cppGetConnected(void);
void cppCircleRead(circlePosition *pos);
u32 cppKeysHeld(void);
u8 cppBatteryLevel(void);
#endif
