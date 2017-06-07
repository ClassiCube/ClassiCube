#include "GameStructs.h"

IGameComponent IGameComponent_MakeEmpty(void) {
	IGameComponent comp;
	comp.Init = NULL;
	comp.Free = NULL;
	comp.Ready = NULL;
	comp.Reset = NULL;
	comp.OnNewMap = NULL;
	comp.OnNewMapLoaded = NULL;
	return comp;
}