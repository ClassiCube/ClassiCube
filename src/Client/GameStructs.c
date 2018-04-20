#include "GameStructs.h"

void IGameComponent_NullFunc(void) { }
IGameComponent IGameComponent_MakeEmpty(void) {
	IGameComponent comp;
	comp.Init  = IGameComponent_NullFunc;
	comp.Free  = IGameComponent_NullFunc;
	comp.Ready = IGameComponent_NullFunc;
	comp.Reset = IGameComponent_NullFunc;
	comp.OnNewMap       = IGameComponent_NullFunc;
	comp.OnNewMapLoaded = IGameComponent_NullFunc;
	return comp;
}