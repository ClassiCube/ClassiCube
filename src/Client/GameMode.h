#ifndef CC_GAMEMODE_H
#define CC_GAMEMODE_H
#include "Input.h"
/* Implements behaviour specific for creative / survival game modes.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

struct IGameComponent;
struct HorbarWidget;
void GameMode_MakeComponent(struct IGameComponent* comp);

bool GameMode_HandlesKeyDown(Key key);
bool GameMode_PickingLeft(void);
bool GameMode_PickingRight(void);
void GameMode_PickLeft(BlockID old);
void GameMode_PickMiddle(BlockID old);
void GameMode_PickRight(BlockID old, BlockID block);
struct HotbarWidget* GameMode_MakeHotbar(void);
void GameMode_BeginFrame(Real64 delta);
void GameMode_EndFrame(Real64 delta);
#endif
