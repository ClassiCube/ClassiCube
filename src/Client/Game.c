#include "Game.h"
#include "Block.h"
#include "World.h"
#include "WeatherRenderer.h"
#include "Lighting.h"
#include "MapRenderer.h"
#include "GraphicsAPI.h"
#include "Camera.h"
#include "Options.h"
#include "Funcs.h"
#include "ExtMath.h"
#include "Gui.h"
#include "Window.h"
#include "Event.h"
#include "Utils.h"
#include "ErrorHandler.h"
#include "Entity.h"
#include "Chat.h"
#include "Platform.h"

UInt8 Game_UsernameBuffer[String_BufferSize(STRING_SIZE)];
extern String Game_Username = String_FromEmptyArray(Game_UsernameBuffer);
UInt8 Game_MppassBuffer[String_BufferSize(STRING_SIZE)];
extern String Game_Mppass = String_FromEmptyArray(Game_MppassBuffer);

UInt8 Game_IPAddressBuffer[String_BufferSize(STRING_SIZE)];
extern String Game_IPAddress = String_FromEmptyArray(Game_IPAddressBuffer);
UInt8 Game_FontNameBuffer[String_BufferSize(STRING_SIZE)];
extern String Game_FontName = String_FromEmptyArray(Game_FontNameBuffer);

Int32 Game_GetWindowScale(void) {
	Real32 windowScale = min(Game_Width / 640.0f, Game_Height / 480.0f);
	return 1 + (Int32)windowScale;
 }

Real32 Game_Scale(Real32 value) {
	return (Real32)((Int32)(value * 10 + 0.5f)) / 10.0f;
}

Real32 Game_GetHotbarScale(void) {
	return Game_Scale(Game_GetWindowScale() * Game_RawHotbarScale);
}

Real32 Game_GetInventoryScale(void) {
	return Game_Scale(Game_GetWindowScale() * (Game_RawInventoryScale * 0.5f));
}

Real32 Game_GetChatScale(void) {
	return Game_Scale(Game_GetWindowScale() * Game_RawChatScale);
}

UInt8 game_defTexPackBuffer[String_BufferSize(STRING_SIZE)];
String game_defTexPack = String_FromEmptyArray(game_defTexPackBuffer);

void Game_GetDefaultTexturePack(STRING_TRANSIENT String* texPack) {
	UInt8 texPathBuffer[String_BufferSize(STRING_SIZE)];
	String texPath = String_InitAndClearArray(texPathBuffer);
	String_Format2(&texPath, "texpacks%b%s", &Platform_DirectorySeparator, &game_defTexPack);

	if (Platform_FileExists(&texPath) && !Game_ClassicMode) {
		String_AppendString(texPack, &game_defTexPack);
	} else {
		String_AppendConst(texPack, "default.zip");
	}
}

void Game_SetDefaultTexturePack(STRING_PURE String* texPack) {
	String_Set(&game_defTexPack, texPack);
	Options_Set(OPT_DEFAULT_TEX_PACK, texPack);
}


bool game_CursorVisible = true, game_realCursorVisible = true;
bool Game_GetCursorVisible(void) { return game_CursorVisible; }
void Game_SetCursorVisible(bool visible) {
	/* Defer mouse visibility changes */
	game_realCursorVisible = visible;
	if (Gui_OverlaysCount > 0) return;

	/* Only set the value when it has changed */
	if (game_CursorVisible == visible) return;
	Window_SetCursorVisible(visible);
	game_CursorVisible = visible;
}

bool Game_ChangeTerrainAtlas(Bitmap* atlas) {
	String terrain = String_FromConst("terrain.png");
	if (!Game_ValidateBitmap(&terrain, atlas)) return false;

	if (atlas->Width != atlas->Height) {
		String m1 = String_FromConst("&cUnable to use terrain.png from the texture pack."); Chat_Add(&m1);
		String m2 = String_FromConst("&c Its width is not the same as its height.");        Chat_Add(&m2);
		return false;
	}
	if (Gfx_LostContext) return false;

	Atlas1D_Free();
	Atlas2D_Free();
	Atlas2D_UpdateState(atlas);
	Atlas1D_UpdateState();

	Event_RaiseVoid(&TextureEvents_AtlasChanged);
	return true;
}

void Game_UpdateProjection(void) {
	Game_DefaultFov = Options_GetInt(OPT_FIELD_OF_VIEW, 1, 150, 70);
	Camera_Active->GetProjection(&Gfx_Projection);

	Gfx_SetMatrixMode(MATRIX_TYPE_PROJECTION);
	Gfx_LoadMatrix(&Gfx_Projection);
	Gfx_SetMatrixMode(MATRIX_TYPE_MODELVIEW);
	Event_RaiseVoid(&GfxEvents_ProjectionChanged);
}

void Game_UpdateBlock(Int32 x, Int32 y, Int32 z, BlockID block) {
	BlockID oldBlock = World_GetBlock(x, y, z);
	World_SetBlock(x, y, z, block);

	if (Weather_Heightmap != NULL) {
		WeatherRenderer_OnBlockChanged(x, y, z, oldBlock, block);
	}
	Lighting_OnBlockChanged(x, y, z, oldBlock, block);

	/* Refresh the chunk the block was located in. */
	Int32 cx = x >> 4, cy = y >> 4, cz = z >> 4;
	ChunkInfo* chunk = MapRenderer_GetChunk(cx, cy, cz);
	chunk->AllAir &= Block_Draw[block] == DRAW_GAS;
	MapRenderer_RefreshChunk(cx, cy, cz);
}

void Game_SetViewDistance(Int32 distance, bool userDist) {
	if (userDist) {
		Game_UserViewDistance = distance;
		Options_SetInt32(OPT_VIEW_DISTANCE, distance);
	}

	distance = min(distance, Game_MaxViewDistance);
	if (distance == Game_ViewDistance) return;
	Game_ViewDistance = distance;

	Event_RaiseVoid(&GfxEvents_ViewDistanceChanged);
	Game_UpdateProjection();
}

bool Game_CanPick(BlockID block) {
	if (Block_Draw[block] == DRAW_GAS)    return false;
	if (Block_Draw[block] == DRAW_SPRITE) return true;

	if (Block_Collide[block] != COLLIDE_LIQUID) return true;
	return Game_ModifiableLiquids && Block_CanPlace[block] && Block_CanDelete[block];
}

void Game_SetDefaultSkinType(Bitmap* bmp) {
	Game_DefaultPlayerSkinType = Utils_GetSkinType(bmp);
	if (Game_DefaultPlayerSkinType == SKIN_TYPE_INVALID) {
		ErrorHandler_Fail("char.png has invalid dimensions");
	}

	Int32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		Entity* entity = Entities_List[i];
		if (entity == NULL || entity->TextureId != NULL) continue;
		entity->SkinType = Game_DefaultPlayerSkinType;
	}
}

bool Game_UpdateTexture(GfxResourceID* texId, Stream* src, bool setSkinType) {
	Bitmap bmp; Bitmap_DecodePng(&bmp, src);
	bool success = Game_ValidateBitmap(&src->Name, &bmp);

	if (success) {
		Gfx_DeleteTexture(texId);
		if (setSkinType) Game_SetDefaultSkinType(&bmp);
		*texId = Gfx_CreateTexture(&bmp, true, false);
	}

	Platform_MemFree(bmp.Scan0);
	return success;
}

bool Game_ValidateBitmap(STRING_PURE String* file, Bitmap* bmp) {
	UInt8 msgBuffer[String_BufferSize(STRING_SIZE * 2)];
	String msg = String_InitAndClearArray(msgBuffer);

	if (bmp->Scan0 == NULL) {
		String_Format1(&msg, "&cError loading %s from the texture pack.", file); Chat_Add(&msg);
		return false;
	}

	Int32 maxSize = Gfx_MaxTextureDimensions;
	if (bmp->Width > maxSize || bmp->Height > maxSize) {
		String_Format1(&msg, "&cUnable to use %s from the texture pack.", file); Chat_Add(&msg);
		String_Clear(&msg);

		String_Format4(&msg, "&c Its size is (%i,%i), your GPU supports (%i, %i) at most.", &bmp->Width, &bmp->Height, &maxSize, &maxSize);
		Chat_Add(&msg);
		return false;
	}

	if (!Math_IsPowOf2(bmp->Width) || !Math_IsPowOf2(bmp->Height)) {
		String_Format1(&msg, "&cUnable to use %s from the texture pack.", file); Chat_Add(&msg);
		String_Clear(&msg);

		String_Format2(&msg, "&c Its size is (%i,%i), which is not power of two dimensions.", &bmp->Width, &bmp->Height);
		Chat_Add(&msg);
		return false;
	}
	return true;
}