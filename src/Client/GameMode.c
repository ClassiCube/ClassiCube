#include "GameMode.h"

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