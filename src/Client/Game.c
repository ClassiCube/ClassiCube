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
#include "GameMode.h"
#include "Drawer2D.h"
#include "ModelCache.h"
#include "Particle.h"
#include "AsyncDownloader.h"
#include "Animations.h"
#include "Inventory.h"
#include "InputHandler.h"
#include "ServerConnection.h"
#include "TexturePack.h"
#include "Screens.h"
#include "SelectionBox.h"
#include "SkyboxRenderer.h"
#include "AxisLinesRenderer.h"
#include "EnvRenderer.h"
#include "BordersRenderer.h"
#include "HeldBlockRenderer.h"
#include "PickedPosRenderer.h"

UInt8 Game_UsernameBuffer[String_BufferSize(STRING_SIZE)];
extern String Game_Username = String_FromEmptyArray(Game_UsernameBuffer);
UInt8 Game_MppassBuffer[String_BufferSize(STRING_SIZE)];
extern String Game_Mppass = String_FromEmptyArray(Game_MppassBuffer);

UInt8 Game_IPAddressBuffer[String_BufferSize(STRING_SIZE)];
extern String Game_IPAddress = String_FromEmptyArray(Game_IPAddressBuffer);
UInt8 Game_FontNameBuffer[String_BufferSize(STRING_SIZE)];
extern String Game_FontName = String_FromEmptyArray(Game_FontNameBuffer);


IGameComponent Game_Components[26];
Int32 Game_ComponentsCount;
ScheduledTask Game_Tasks[6];
Int32 Game_TasksCount;

void Game_AddComponent(IGameComponent* comp) {
	if (Game_ComponentsCount == Array_Elems(Game_Components)) {
		ErrorHandler_Fail("Game_AddComponent - hit max count");
	}
	Game_Components[Game_ComponentsCount++] = *comp;
}

ScheduledTask ScheduledTask_Add(Real64 interval, ScheduledTaskCallback callback) {
	ScheduledTask task = { 0.0, interval, callback };
	if (Game_TasksCount == Array_Elems(Game_Tasks)) {
		ErrorHandler_Fail("ScheduledTask_Add - hit max count");
	}
	Game_Tasks[Game_TasksCount++] = task;
	return task;
}

void ScheduledTask_TickAll(Real64 time) {
	Int32 i;
	for (i = 0; i < Game_TasksCount; i++) {
		ScheduledTask task = Game_Tasks[i];
		task.Accumulator += time;

		while (task.Accumulator >= task.Interval) {
			task.Callback(&task);
			task.Accumulator -= task.Interval;
		}
		Game_Tasks[i] = task;
	}
}


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


void Game_Load(void) {
	IGameComponent comp;
	Gfx_Init();
	Gfx_MakeApiInfo();
	Drawer2D_Init();

	Entities_Init();
	TextureCache_Init();
	/* TODO: Survival vs Creative game mode */
	comp = GameMode_MakeComponent(); Game_AddComponent(&comp);

	InputHandler_Init();
	defaultIb = Graphics.MakeDefaultIb();
	comp = Particles_MakeComponent(); Game_AddComponent(&comp);
	comp = TabList_MakeComponent();   Game_AddComponent(&comp);

	Game_LoadOptions();
	Game_LoadGuiOptions();
	comp = Chat_MakeComponent(); Game_AddComponent(&comp);

	WorldEvents.OnNewMap += OnNewMapCore;
	WorldEvents.OnNewMapLoaded += OnNewMapLoadedCore;
	Events.TextureChanged += TextureChangedCore;

	Block_Init();
	ModelCache_Init();
	comp = AsyncDownloader_MakeComponent(); Game_AddComponent(&comp);
	comp = Lighting_MakeComponent();        Game_AddComponent(&comp);

	Drawer2D_UseBitmappedChat = Game_ClassicMode || !Options_GetBool(OPT_USE_CHAT_FONT, false);
	Drawer2D_BlackTextShadows = Options_GetBool(OPT_BLACK_TEXT, false);
	Gfx_Mipmaps               = Options_GetBool(OPT_MIPMAPS, false);

	comp = Animations_MakeComponent(); Game_AddComponent(&comp);
	comp = Inventory_MakeComponent();  Game_AddComponent(&comp);
	Block_SetDefaultPerms();
	WorldEnv_Reset();

	LocalPlayer_Init(); 
	comp = LocalPlayer_MakeComponent(); Game_AddComponent(&comp);
	Entities_List[ENTITIES_SELF_ID] = &LocalPlayer_Instance.Base;
	Size2D size = Window_GetClientSize();
	Game_Width = size.Width; Game_Height = size.Height;

	MapRenderer = new MapRenderer();
	string renType = Options_Get(OptionsKey.RenderType, "normal");
	if (!SetRenderType(renType)) {
		SetRenderType("normal");
	}

	if (Game_IPAddress.length == 0) {
		ServerConnection_InitSingleplayer();
	} else {
		ServerConnection_InitMultiplayer();
	}

	Graphics.LostContextFunction = ServerConnection_Tick;
	Camera_Init();
	Game_UpdateProjection();

	comp = Gui_MakeComponent();               Game_AddComponent(&comp);
	comp = Selections_MakeComponent();        Game_AddComponent(&comp);
	comp = WeatherRenderer_MakeComponent();   Game_AddComponent(&comp);
	comp = HeldBlockRenderer_MakeComponent(); Game_AddComponent(&comp);

	Gfx_SetDepthTest(true);
	Gfx_SetDepthTestFunc(COMPARE_FUNC_LESSEQUAL);
	/* Gfx_SetDepthWrite(true) */
	Gfx_SetAlphaBlendFunc(BLEND_FUNC_SRC_ALPHA, BLEND_FUNC_INV_SRC_ALPHA);
	Gfx_SetAlphaTestFunc(COMPARE_FUNC_GREATER, 0.5f);

	comp = PickedPosRenderer_MakeComponent(); Game_AddComponent(&comp);
	comp = AudioPlayer_MakeComponent();       Game_AddComponent(&comp);
	comp = AxisLinesRenderer_MakeComponent(); Game_AddComponent(&comp);
	comp = SkyboxRenderer_MakeComponent();    Game_AddComponent(&comp);

	/* TODO: plugin dll support */
	/* List<string> nonLoaded = PluginLoader.LoadAll(); */

	Int32 i;
	for (i = 0; i < Game_ComponentsCount; i++) {
		Game_Components[i].Init();
	}
	Game_ExtractInitialTexturePack();

	for (i = 0; i < Game_ComponentsCount; i++) {
		Game_Components[i].Ready();
	}
	Game_InitScheduledTasks();

	/* TODO: plugin dll support * /
	/* if (nonLoaded != null) {
		for (int i = 0; i < nonLoaded.Count; i++) {
			Overlay warning = new PluginOverlay(this, nonLoaded[i]);
			Gui_ShowOverlay(warning, false);
		}
	}*/

	if (Gfx_WarnIfNecessary()) {
		BordersRenderer_UseLegacyMode(true);
		EnvRenderer_UseLegacyMode(true);
	}

	UInt8 loadTitleBuffer[String_BufferSize(STRING_SIZE)];
	String loadTitle = String_InitAndClearArray(loadTitleBuffer);
	String_Format2(&loadTitle, "Connecting to %s:%i..", &Game_IPAddress, &Game_Port);
	String loadMsg = String_MakeNull();

	Gui_SetNewScreen(LoadingScreen_MakeInstance(&loadTitle, &loadMsg));
	ServerConnection_Connect(&Game_IPAddress, Game_Port);
}

void Game_ExtractInitialTexturePack(void) {
	UInt8 texPackBuffer[String_BufferSize(STRING_SIZE)];
	String texPack = String_InitAndClearArray(texPackBuffer);
	Options_Get(OPT_DEFAULT_TEX_PACK, &game_defTexPack, "default.zip");

	String_AppendConst(&texPack, "default.zip");
	TexturePack_ExtractZip_File(&texPack);
	String_Clear(&texPack);

	/* in case the user's default texture pack doesn't have all required textures */
	Game_GetDefaultTexturePack(&texPack);
	if (!String_CaselessEqualsConst(&texPack, "default.zip")) {
		exturePack_ExtractZip_File(&texPack);
	}
}

void Game_LoadOptions(void) {
	Game_ClassicMode       = Options_GetBool("mode-classic", false);
	Game_ClassicHacks      = Options_GetBool(OPT_ALLOW_CLASSIC_HACKS, false);
	Game_AllowCustomBlocks = Options_GetBool(OPT_USE_CUSTOM_BLOCKS, true);
	Game_UseCPE            = Options_GetBool(OPT_USE_CPE, true);
	Game_SimpleArmsAnim    = Options_GetBool(OPT_SIMPLE_ARMS_ANIM, false);
	Game_ChatLogging       = Options_GetBool(OPT_CHAT_LOGGING, true);
	Game_ClassicArmModel   = Options_GetBool(OPT_CLASSIC_ARM_MODEL, Game_ClassicMode);

	Game_ViewBobbing = Options_GetBool(OPT_VIEW_BOBBING, true);
	FpsLimit method  = Options_GetEnum(OPT_FPS_LIMIT, 0, FpsLimit_Names, FpsLimit_Count);
	Game_SetFpsLimitMethod(method);

	Game_ViewDistance     = Options_GetInt(OPT_VIEW_DISTANCE, 16, 4096, 512);
	Game_UserViewDistance = Game_ViewDistance;
	Game_SmoothLighting   = Options_GetBool(OPT_SMOOTH_LIGHTING, false);

	Game_DefaultFov = Options_GetInt(OPT_FIELD_OF_VIEW, 1, 150, 70);
	Game_Fov        = Game_DefaultFov;
	Game_ZoomFov    = Game_DefaultFov;
	Game_ModifiableLiquids = !Game_ClassicMode && Options_GetBool(OPT_MODIFIABLE_LIQUIDS, false);
	Game_CameraClipping    = Options_GetBool(OPT_CAMERA_CLIPPING, true);
	Game_MaxChunkUpdates   = Options_GetInt(OPT_MAX_CHUNK_UPDATES, 4, 1024, 30);

	Game_AllowServerTextures = Options_GetBool(OPT_USE_SERVER_TEXTURES, true);
	Game_MouseSensitivity    = Options_GetInt(OPT_SENSITIVITY, 1, 100, 30);
	Game_ShowBlockInHand     = Options_GetBool(OPT_SHOW_BLOCK_IN_HAND, true);
	Game_InvertMouse         = Options_GetBool(OPT_INVERT_MOUSE, false);

	/* TODO: Do we need to support option to skip SSL */
	/*bool skipSsl = Options_GetBool("skip-ssl-check", false);
	if (skipSsl) {
		ServicePointManager.ServerCertificateValidationCallback = delegate { return true; };
		Options.Set("skip-ssl-check", false);
	}*/
}

void Game_LoadGuiOptions(void) {
	Game_ChatLines         = Options_GetInt(OPT_CHATLINES, 1, 30, 12);
	Game_ClickableChat     = Options_GetBool(OPT_CLICKABLE_CHAT, false);
	Game_RawInventoryScale = Options_GetFloat(OPT_INVENTORY_SCALE, 0.25f, 5.0f, 1.0f);
	Game_RawHotbarScale    = Options_GetFloat(OPT_HOTBAR_SCALE,    0.25f, 5.0f, 1.0f);
	Game_RawChatScale      = Options_GetFloat(OPT_CHAT_SCALE,      0.35f, 5.0f, 1.0f);
	Game_ShowFPS           = Options_GetBool(OPT_SHOW_FPS, true);

	Game_UseClassicGui     = Options_GetBool(OPT_USE_CLASSIC_GUI, true)      || Game_ClassicMode;
	Game_UseClassicTabList = Options_GetBool(OPT_USE_CLASSIC_TABLIST, false) || Game_ClassicMode;
	Game_UseClassicOptions = Options_GetBool(OPT_USE_CLASSIC_OPTIONS, false) || Game_ClassicMode;

	Game_TabAutocomplete = Options_GetBool(OPT_TAB_AUTOCOMPLETE, false);
	Options_Get(OPT_FONT_NAME, &Game_FontName, "Arial");
	if (Game_ClassicMode) {
		String_Clear(&Game_FontName);
		String_AppendConst(&Game_FontName, "Arial");
	}

	/* TODO: Handle Arial font not working */
}

ScheduledTask entTask;
void Game_InitScheduledTasks(void) {
	#define GAME_DEF_TICKS (1.0 / 20)
	#define GAME_NET_TICKS (1.0 / 60)

	ScheduledTask_Add(30, AsyncDownloader_PurgeOldEntriesTask);
	ScheduledTask_Add(GAME_NET_TICKS, ServerConnection_Tick);
	entTask = ScheduledTask_Add(GAME_DEF_TICKS, Entities_Tick);

	ScheduledTask_Add(GAME_DEF_TICKS, Particles_Tick);
	ScheduledTask_Add(GAME_DEF_TICKS, Animations_Tick);
}