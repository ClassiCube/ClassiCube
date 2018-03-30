#ifndef CC_GAMEMODE_H
#define CC_GAMEMODE_H
#include "GameStructs.h"
#include "Input.h"
#include "Widgets.h"
/* Implements behaviour specific for creative / survival game modes.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

IGameComponent GameMode_MakeComponent(void);

bool GameMode_HandlesKeyDown(Key key);
bool GameMode_PickingLeft(void);
bool GameMode_PickingRight(void);
void GameMode_PickLeft(BlockID old);
void GameMode_PickMiddle(BlockID old);
void GameMode_PickRight(BlockID old, BlockID block);
HotbarWidget* GameMode_MakeHotbar(void);
void GameMode_BeginFrame(Real64 delta);
void GameMode_EndFrame(Real64 delta);
#endif