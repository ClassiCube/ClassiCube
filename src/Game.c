#include "Game.h"
#include "Block.h"
#include "World.h"
#include "Lighting.h"
#include "MapRenderer.h"
#include "Graphics.h"
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
#include "AxisLinesRenderer.h"
#include "EnvRenderer.h"
#include "HeldBlockRenderer.h"
#include "PickedPosRenderer.h"
#include "GraphicsCommon.h"
#include "Menus.h"
#include "Audio.h"
#include "DisplayDevice.h"
#include "Stream.h"
#include "Bitmap.h"

struct IGameComponent Game_Components[26];
int Game_ComponentsCount;
struct ScheduledTask Game_Tasks[6];
int Game_TasksCount, entTaskI;

char Game_UsernameBuffer[FILENAME_SIZE];
String Game_Username = String_FromArray(Game_UsernameBuffer);
char Game_MppassBuffer[STRING_SIZE];
String Game_Mppass = String_FromArray(Game_MppassBuffer);

char Game_IPAddressBuffer[STRING_SIZE];
String Game_IPAddress = String_FromArray(Game_IPAddressBuffer);
char Game_FontNameBuffer[STRING_SIZE];
String Game_FontName = String_FromArray(Game_FontNameBuffer);

void Game_AddComponent(struct IGameComponent* comp) {
	if (Game_ComponentsCount == Array_Elems(Game_Components)) {
		ErrorHandler_Fail("Game_AddComponent - hit max count");
	}
	Game_Components[Game_ComponentsCount++] = *comp;
	IGameComponent_MakeEmpty(comp);
}

void IGameComponent_NullFunc(void) { }
void IGameComponent_MakeEmpty(struct IGameComponent* comp) {
	comp->Init  = IGameComponent_NullFunc;
	comp->Free  = IGameComponent_NullFunc;
	comp->Ready = IGameComponent_NullFunc;
	comp->Reset = IGameComponent_NullFunc;
	comp->OnNewMap       = IGameComponent_NullFunc;
	comp->OnNewMapLoaded = IGameComponent_NullFunc;
}

int ScheduledTask_Add(double interval, ScheduledTaskCallback callback) {
	struct ScheduledTask task = { 0.0, interval, callback };
	if (Game_TasksCount == Array_Elems(Game_Tasks)) {
		ErrorHandler_Fail("ScheduledTask_Add - hit max count");
	}
	Game_Tasks[Game_TasksCount++] = task;
	return Game_TasksCount - 1;
}


int Game_GetWindowScale(void) {
	float windowScale = min(Game_Width / 640.0f, Game_Height / 480.0f);
	return 1 + (int)windowScale;
 }

float Game_Scale(float value) {
	return (float)((int)(value * 10 + 0.5f)) / 10.0f;
}

float Game_GetHotbarScale(void) {
	return Game_Scale(Game_GetWindowScale() * Game_RawHotbarScale);
}

float Game_GetInventoryScale(void) {
	return Game_Scale(Game_GetWindowScale() * (Game_RawInventoryScale * 0.5f));
}

float Game_GetChatScale(void) {
	return Game_Scale(Game_GetWindowScale() * Game_RawChatScale);
}

char game_defTexPackBuffer[STRING_SIZE];
String game_defTexPack = String_FromArray(game_defTexPackBuffer);

void Game_GetDefaultTexturePack(String* texPack) {
	char texPathBuffer[STRING_SIZE];
	String texPath = String_FromArray(texPathBuffer);
	String_Format2(&texPath, "texpacks%r%s", &Directory_Separator, &game_defTexPack);

	if (File_Exists(&texPath) && !Game_ClassicMode) {
		String_AppendString(texPack, &game_defTexPack);
	} else {
		String_AppendConst(texPack, "default.zip");
	}
}

void Game_SetDefaultTexturePack(const String* texPack) {
	String_Copy(&game_defTexPack, texPack);
	Options_Set(OPT_DEFAULT_TEX_PACK, texPack);
}

bool Game_ChangeTerrainAtlas(Bitmap* atlas) {
	String terrain = String_FromConst("terrain.png");
	if (!Game_ValidateBitmap(&terrain, atlas)) return false;

	if (atlas->Height < atlas->Width) {
		Chat_AddRaw("&cUnable to use terrain.png from the texture pack.");
		Chat_AddRaw("&c Its height is less than its width.");
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

void Game_SetViewDistance(int distance) {
	distance = min(distance, Game_MaxViewDistance);
	if (distance == Game_ViewDistance) return;
	Game_ViewDistance = distance;

	Event_RaiseVoid(&GfxEvents_ViewDistanceChanged);
	Game_UpdateProjection();
}

void Game_UserSetViewDistance(int distance) {
	Game_UserViewDistance = distance;
	Options_SetInt(OPT_VIEW_DISTANCE, distance);
	Game_SetViewDistance(distance);
}

void Game_UpdateProjection(void) {
	Game_DefaultFov = Options_GetInt(OPT_FIELD_OF_VIEW, 1, 150, 70);
	Camera_Active->GetProjection(&Gfx_Projection);

	Gfx_SetMatrixMode(MATRIX_TYPE_PROJECTION);
	Gfx_LoadMatrix(&Gfx_Projection);
	Gfx_SetMatrixMode(MATRIX_TYPE_VIEW);
	Event_RaiseVoid(&GfxEvents_ProjectionChanged);
}

void Game_Disconnect(const String* title, const String* reason) {
	World_Reset();
	Event_RaiseVoid(&WorldEvents_NewMap);
	Gui_FreeActive();
	Gui_SetActive(DisconnectScreen_MakeInstance(title, reason));

	Drawer2D_Init();
	Block_Reset();
	TexturePack_ExtractDefault();

	int i;
	for (i = 0; i < Game_ComponentsCount; i++) {
		Game_Components[i].Reset();
	}
}

void Game_UpdateBlock(int x, int y, int z, BlockID block) {
	BlockID oldBlock = World_GetBlock(x, y, z);
	World_SetBlock(x, y, z, block);

	if (Weather_Heightmap) {
		EnvRenderer_OnBlockChanged(x, y, z, oldBlock, block);
	}
	Lighting_OnBlockChanged(x, y, z, oldBlock, block);

	/* Refresh the chunk the block was located in. */
	int cx = x >> 4, cy = y >> 4, cz = z >> 4;
	struct ChunkInfo* chunk = MapRenderer_GetChunk(cx, cy, cz);
	chunk->AllAir &= Block_Draw[block] == DRAW_GAS;
	MapRenderer_RefreshChunk(cx, cy, cz);
}

bool Game_CanPick(BlockID block) {
	if (Block_Draw[block] == DRAW_GAS)    return false;
	if (Block_Draw[block] == DRAW_SPRITE) return true;

	if (Block_Collide[block] != COLLIDE_LIQUID) return true;
	return Game_BreakableLiquids && Block_CanPlace[block] && Block_CanDelete[block];
}

bool Game_UpdateTexture(GfxResourceID* texId, struct Stream* src, const String* file, uint8_t* skinType) {
	Bitmap bmp; 
	ReturnCode res = Png_Decode(&bmp, src);
	if (res) { Chat_LogError2(res, "decoding", file); }

	bool success = !res && Game_ValidateBitmap(file, &bmp);
	if (success) {
		Gfx_DeleteTexture(texId);
		if (skinType) { *skinType = Utils_GetSkinType(&bmp); }
		*texId = Gfx_CreateTexture(&bmp, true, false);
	}

	Mem_Free(bmp.Scan0);
	return success;
}

bool Game_ValidateBitmap(const String* file, Bitmap* bmp) {
	if (!bmp->Scan0) {
		Chat_Add1("&cError loading %s from the texture pack.", file);
		return false;
	}

	int maxWidth = Gfx_MaxTexWidth, maxHeight = Gfx_MaxTexHeight;
	if (bmp->Width > maxWidth || bmp->Height > maxHeight) {
		Chat_Add1("&cUnable to use %s from the texture pack.", file);

		Chat_Add4("&c Its size is (%i,%i), your GPU supports (%i,%i) at most.", 
			&bmp->Width, &bmp->Height, &maxWidth, &maxHeight);
		return false;
	}

	if (!Math_IsPowOf2(bmp->Width) || !Math_IsPowOf2(bmp->Height)) {
		Chat_Add1("&cUnable to use %s from the texture pack.", file);

		Chat_Add2("&c Its size is (%i,%i), which is not power of two dimensions.", 
			&bmp->Width, &bmp->Height);
		return false;
	}
	return true;
}

int Game_CalcRenderType(const String* type) {
	if (String_CaselessEqualsConst(type, "legacyfast")) return 0x03;
	if (String_CaselessEqualsConst(type, "legacy"))     return 0x01;
	if (String_CaselessEqualsConst(type, "normal"))     return 0x00;
	if (String_CaselessEqualsConst(type, "normalfast")) return 0x02;

	return -1;
}

static void Game_UpdateClientSize(void) {
	Size2D size = Window_ClientSize;
	Game_Width  = max(size.Width,  1);
	Game_Height = max(size.Height, 1);
}

static void Game_OnResize(void* obj) {
	Game_UpdateClientSize();
	Gfx_OnWindowResize();
	Game_UpdateProjection();
	Gui_OnResize();
}

static void Game_OnNewMapCore(void* obj) {
	int i;
	for (i = 0; i < Game_ComponentsCount; i++) {
		Game_Components[i].OnNewMap();
	}
}

static void Game_OnNewMapLoadedCore(void* obj) {
	int i;
	for (i = 0; i < Game_ComponentsCount; i++) {
		Game_Components[i].OnNewMapLoaded();
	}
}

static void Game_TextureChangedCore(void* obj, struct Stream* src, const String* name) {
	Bitmap bmp;
	if (String_CaselessEqualsConst(name, "terrain.png")) {
		ReturnCode res = Png_Decode(&bmp, src);

		if (res) { 
			Chat_LogError2(res, "decoding", name);
			Mem_Free(bmp.Scan0);
		} else if (!Game_ChangeTerrainAtlas(&bmp)) {
			Mem_Free(bmp.Scan0);
		}		
	} else if (String_CaselessEqualsConst(name, "default.png")) {
		ReturnCode res = Png_Decode(&bmp, src);

		if (res) { 
			Chat_LogError2(res, "decoding", name);
			Mem_Free(bmp.Scan0);
		} else {
			Drawer2D_SetFontBitmap(&bmp);
			Event_RaiseVoid(&ChatEvents_FontChanged);
		}
	}
}

static void Game_OnLowVRAMDetected(void* obj) {
	if (Game_UserViewDistance <= 16) ErrorHandler_Fail("Out of video memory!");
	Game_UserViewDistance /= 2;
	Game_UserViewDistance = max(16, Game_UserViewDistance);

	ChunkUpdater_Refresh();
	Game_SetViewDistance(Game_UserViewDistance);
	Chat_AddRaw("&cOut of VRAM! Halving view distance..");
}

static void Game_ExtractInitialTexturePack(void) {
	char texPackBuffer[STRING_SIZE];
	String texPack = String_FromArray(texPackBuffer);
	Options_Get(OPT_DEFAULT_TEX_PACK, &game_defTexPack, "default.zip");

	String_AppendConst(&texPack, "default.zip");
	TexturePack_ExtractZip_File(&texPack);
	texPack.length = 0;

	/* in case the user's default texture pack doesn't have all required textures */
	Game_GetDefaultTexturePack(&texPack);
	if (!String_CaselessEqualsConst(&texPack, "default.zip")) {
		TexturePack_ExtractZip_File(&texPack);
	}
}

static void Game_LoadOptions(void) {
	Game_ClassicMode       = Options_GetBool(OPT_CLASSIC_MODE, false);
	Game_ClassicHacks      = Options_GetBool(OPT_CLASSIC_HACKS, false);
	Game_AllowCustomBlocks = Options_GetBool(OPT_CUSTOM_BLOCKS, true);
	Game_UseCPE            = Options_GetBool(OPT_CPE, true);
	Game_SimpleArmsAnim    = Options_GetBool(OPT_SIMPLE_ARMS_ANIM, false);
	Game_ChatLogging       = Options_GetBool(OPT_CHAT_LOGGING, true);
	Game_ClassicArmModel   = Options_GetBool(OPT_CLASSIC_ARM_MODEL, Game_ClassicMode);

	Game_ViewBobbing = Options_GetBool(OPT_VIEW_BOBBING, true);
	int method  = Options_GetEnum(OPT_FPS_LIMIT, 0, FpsLimit_Names, FPS_LIMIT_COUNT);
	Game_SetFpsLimit(method);

	Game_ViewDistance     = Options_GetInt(OPT_VIEW_DISTANCE, 16, 4096, 512);
	Game_UserViewDistance = Game_ViewDistance;
	Game_SmoothLighting   = Options_GetBool(OPT_SMOOTH_LIGHTING, false);

	Game_DefaultFov = Options_GetInt(OPT_FIELD_OF_VIEW, 1, 150, 70);
	Game_Fov        = Game_DefaultFov;
	Game_ZoomFov    = Game_DefaultFov;
	Game_BreakableLiquids = !Game_ClassicMode && Options_GetBool(OPT_MODIFIABLE_LIQUIDS, false);
	Game_CameraClipping    = Options_GetBool(OPT_CAMERA_CLIPPING, true);
	Game_MaxChunkUpdates   = Options_GetInt(OPT_MAX_CHUNK_UPDATES, 4, 1024, 30);

	Game_AllowServerTextures = Options_GetBool(OPT_SERVER_TEXTURES, true);
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

static void Game_LoadGuiOptions(void) {
	Game_ChatLines         = Options_GetInt(OPT_CHATLINES, 0, 30, 12);
	Game_ClickableChat     = Options_GetBool(OPT_CLICKABLE_CHAT, false);
	Game_RawInventoryScale = Options_GetFloat(OPT_INVENTORY_SCALE, 0.25f, 5.0f, 1.0f);
	Game_RawHotbarScale    = Options_GetFloat(OPT_HOTBAR_SCALE,    0.25f, 5.0f, 1.0f);
	Game_RawChatScale      = Options_GetFloat(OPT_CHAT_SCALE,      0.35f, 5.0f, 1.0f);
	Game_ShowFPS           = Options_GetBool(OPT_SHOW_FPS, true);

	Game_UseClassicGui     = Options_GetBool(OPT_CLASSIC_GUI, true)      || Game_ClassicMode;
	Game_UseClassicTabList = Options_GetBool(OPT_CLASSIC_TABLIST, false) || Game_ClassicMode;
	Game_UseClassicOptions = Options_GetBool(OPT_CLASSIC_OPTIONS, false) || Game_ClassicMode;

	Game_TabAutocomplete = Options_GetBool(OPT_TAB_AUTOCOMPLETE, false);
	Options_Get(OPT_FONT_NAME, &Game_FontName, Font_DefaultName);
	if (Game_ClassicMode) {
		Game_FontName.length = 0;
		String_AppendConst(&Game_FontName, Font_DefaultName);
	}

	/* TODO: Handle Arial font not working */
}

static void Game_InitScheduledTasks(void) {
	#define GAME_DEF_TICKS (1.0 / 20)
	#define GAME_NET_TICKS (1.0 / 60)

	ScheduledTask_Add(30, AsyncDownloader_PurgeOldEntriesTask);
	ScheduledTask_Add(GAME_NET_TICKS, ServerConnection_Tick);
	entTaskI = ScheduledTask_Add(GAME_DEF_TICKS, Entities_Tick);

	ScheduledTask_Add(GAME_DEF_TICKS, Particles_Tick);
	ScheduledTask_Add(GAME_DEF_TICKS, Animations_Tick);
}

void Game_Free(void* obj);
void Game_Load(void) {
	Game_ViewDistance     = 512;
	Game_MaxViewDistance  = 32768;
	Game_UserViewDistance = 512;
	Game_Fov = 70;
	Game_AutoRotate = true;

	struct IGameComponent comp; IGameComponent_MakeEmpty(&comp);
	Gfx_Init();
	Gfx_SetVSync(true);
	Gfx_MakeApiInfo();

	Drawer2D_Init();
	Game_UpdateClientSize();

	Entities_Init();
	TextureCache_Init();
	/* TODO: Survival vs Creative game mode */

	InputHandler_Init();
	Particles_MakeComponent(&comp); Game_AddComponent(&comp);
	TabList_MakeComponent(&comp);   Game_AddComponent(&comp);

	Game_LoadOptions();
	Game_LoadGuiOptions();
	Chat_MakeComponent(&comp); Game_AddComponent(&comp);

	Event_RegisterVoid(&WorldEvents_NewMap,         NULL, Game_OnNewMapCore);
	Event_RegisterVoid(&WorldEvents_MapLoaded,      NULL, Game_OnNewMapLoadedCore);
	Event_RegisterEntry(&TextureEvents_FileChanged, NULL, Game_TextureChangedCore);
	Event_RegisterVoid(&GfxEvents_LowVRAMDetected,  NULL, Game_OnLowVRAMDetected);

	Event_RegisterVoid(&WindowEvents_Resized,       NULL, Game_OnResize);
	Event_RegisterVoid(&WindowEvents_Closed,        NULL, Game_Free);

#ifdef EXTENDED_BLOCKS
	Block_SetUsedCount(256);
#endif
	Block_Init();
	ModelCache_Init();

	AsyncDownloader_MakeComponent(&comp); Game_AddComponent(&comp);
	Lighting_MakeComponent(&comp);        Game_AddComponent(&comp);

	Drawer2D_BitmappedText = Game_ClassicMode || !Options_GetBool(OPT_USE_CHAT_FONT, false);
	Drawer2D_BlackTextShadows = Options_GetBool(OPT_BLACK_TEXT, false);
	Gfx_Mipmaps               = Options_GetBool(OPT_MIPMAPS, false);

	Animations_MakeComponent(&comp); Game_AddComponent(&comp);
	Inventory_MakeComponent(&comp);  Game_AddComponent(&comp);
	Block_SetDefaultPerms();
	Env_Reset();

	LocalPlayer_Init(); 
	LocalPlayer_MakeComponent(&comp); Game_AddComponent(&comp);
	Entities_List[ENTITIES_SELF_ID] = &LocalPlayer_Instance.Base;

	ChunkUpdater_Init();
	EnvRenderer_MakeComponent(&comp);     Game_AddComponent(&comp);

	char renderTypeBuffer[STRING_SIZE];
	String renderType = String_FromArray(renderTypeBuffer);
	Options_Get(OPT_RENDER_TYPE, &renderType, "normal");
	int flags = Game_CalcRenderType(&renderType);

	if (flags == -1) flags = 0;
	EnvRenderer_Legacy  = (flags & 1);
	EnvRenderer_Minimal = (flags & 2);

	if (!Game_IPAddress.length) {
		ServerConnection_InitSingleplayer();
	} else {
		ServerConnection_InitMultiplayer();
	}
	ServerConnection_MakeComponent(&comp); Game_AddComponent(&comp);
	String_AppendConst(&ServerConnection_AppName, PROGRAM_APP_NAME);

	Gfx_LostContextFunction = ServerConnection_Tick;
	Camera_Init();
	Game_UpdateProjection();

	Gui_MakeComponent(&comp);               Game_AddComponent(&comp);
	Selections_MakeComponent(&comp);        Game_AddComponent(&comp);
	HeldBlockRenderer_MakeComponent(&comp); Game_AddComponent(&comp);

	Gfx_SetDepthTest(true);
	Gfx_SetDepthTestFunc(COMPARE_FUNC_LESSEQUAL);
	/* Gfx_SetDepthWrite(true) */
	Gfx_SetAlphaBlendFunc(BLEND_FUNC_SRC_ALPHA, BLEND_FUNC_INV_SRC_ALPHA);
	Gfx_SetAlphaTestFunc(COMPARE_FUNC_GREATER, 0.5f);

	PickedPosRenderer_MakeComponent(&comp); Game_AddComponent(&comp);
	Audio_MakeComponent(&comp);             Game_AddComponent(&comp);
	AxisLinesRenderer_MakeComponent(&comp); Game_AddComponent(&comp);

	/* TODO: plugin dll support */
	/* List<string> nonLoaded = PluginLoader.LoadAll(); */

	int i;
	for (i = 0; i < Game_ComponentsCount; i++) {
		Game_Components[i].Init();
	}
	Game_ExtractInitialTexturePack();

	for (i = 0; i < Game_ComponentsCount; i++) {
		Game_Components[i].Ready();
	}
	Game_InitScheduledTasks();

	/* TODO: plugin dll support */
	/* if (nonLoaded != null) {
		for (int i = 0; i < nonLoaded.Count; i++) {
			Overlay warning = new PluginOverlay(this, nonLoaded[i]);
			Gui_ShowOverlay(warning, false);
		}
	}*/

	if (Gfx_WarnIfNecessary()) EnvRenderer_UseLegacyMode(true);

	char loadTitleBuffer[STRING_SIZE];
	String loadTitle = String_FromArray(loadTitleBuffer);
	String_Format2(&loadTitle, "Connecting to %s:%i..", &Game_IPAddress, &Game_Port);
	String loadMsg = String_MakeNull();

	Gui_FreeActive();
	Gui_SetActive(LoadingScreen_MakeInstance(&loadTitle, &loadMsg));
	ServerConnection_BeginConnect();
}

uint64_t game_frameTimer;
void Game_SetFpsLimit(enum FpsLimit method) {
	Game_FpsLimit = method;
	game_limitMs  = Game_CalcLimitMillis(method);
	Gfx_SetVSync(method == FPS_LIMIT_VSYNC);
}

float Game_CalcLimitMillis(enum FpsLimit method) {
	if (method == FPS_LIMIT_120) return 1000/120.0f;
	if (method == FPS_LIMIT_60)  return 1000/60.0f;
	if (method == FPS_LIMIT_30)  return 1000/30.0f;
	return 0;
}

static void Game_LimitFPS(void) {
	float elapsedMs = Stopwatch_ElapsedMicroseconds(&game_frameTimer) / 1000.0f;
	float leftOver  = game_limitMs - elapsedMs;

	/* going faster than limit */
	if (leftOver > 0.001f) {
		Thread_Sleep((int)(leftOver + 0.5f));
	}
}

static void Game_UpdateViewMatrix(void) {
	Gfx_SetMatrixMode(MATRIX_TYPE_VIEW);
	Camera_Active->GetView(&Gfx_View);
	Gfx_LoadMatrix(&Gfx_View);
	FrustumCulling_CalcFrustumEquations(&Gfx_Projection, &Gfx_View);
}

static void Game_Render3D(double delta, float t) {
	if (EnvRenderer_ShouldRenderSkybox()) EnvRenderer_RenderSkybox(delta);
	AxisLinesRenderer_Render(delta);
	Entities_RenderModels(delta, t);
	Entities_RenderNames(delta);

	Particles_Render(delta, t);
	Camera_Active->GetPickedBlock(&Game_SelectedPos); /* TODO: only pick when necessary */

	EnvRenderer_UpdateFog();
	EnvRenderer_RenderSky(delta);
	EnvRenderer_RenderClouds(delta);

	ChunkUpdater_Update(delta);
	MapRenderer_RenderNormal(delta);
	EnvRenderer_RenderMapSides(delta);

	Entities_DrawShadows();
	if (Game_SelectedPos.Valid && !Game_HideGui) {
		PickedPosRenderer_Update(&Game_SelectedPos);
		PickedPosRenderer_Render(delta);
	}

	/* Render water over translucent blocks when underwater for proper alpha blending */
	Vector3 pos = LocalPlayer_Instance.Base.Position;
	if (Game_CurrentCameraPos.Y < Env_EdgeHeight
		&& (pos.X < 0 || pos.Z < 0 || pos.X > World_Width || pos.Z > World_Length)) {
		MapRenderer_RenderTranslucent(delta);
		EnvRenderer_RenderMapEdges(delta);
	} else {
		EnvRenderer_RenderMapEdges(delta);
		MapRenderer_RenderTranslucent(delta);
	}

	/* Need to render again over top of translucent block, as the selection outline */
	/* is drawn without writing to the depth buffer */
	if (Game_SelectedPos.Valid && !Game_HideGui && Block_Draw[Game_SelectedPos.Block] == DRAW_TRANSLUCENT) {
		PickedPosRenderer_Render(delta);
	}

	Selections_Render(delta);
	Entities_RenderHoveredNames(delta);

	bool left   = InputHandler_IsMousePressed(MouseButton_Left);
	bool middle = InputHandler_IsMousePressed(MouseButton_Middle);
	bool right  = InputHandler_IsMousePressed(MouseButton_Right);
	InputHandler_PickBlocks(true, left, middle, right);
	if (!Game_HideGui) HeldBlockRenderer_Render(delta);
}

static void Game_DoScheduledTasks(double time) {
	int i;
	for (i = 0; i < Game_TasksCount; i++) {
		struct ScheduledTask task = Game_Tasks[i];
		task.Accumulator += time;

		while (task.Accumulator >= task.Interval) {
			task.Callback(&task);
			task.Accumulator -= task.Interval;
		}
		Game_Tasks[i] = task;
	}
}

void Game_TakeScreenshot(void) {
	Game_ScreenshotRequested = false;
	if (!Utils_EnsureDirectory("screenshots")) return;

	DateTime now; DateTime_CurrentLocal(&now);
	int year = now.Year, month = now.Month, day = now.Day;
	int hour = now.Hour, min = now.Minute, sec = now.Second;

	char fileBuffer[STRING_SIZE];
	String filename = String_FromArray(fileBuffer);
	String_Format3(&filename, "screenshot_%p2-%p2-%p4", &day, &month, &year);
	String_Format3(&filename, "-%p2-%p2-%p2.png", &hour, &min, &sec);

	char pathBuffer[FILENAME_SIZE];
	String path = String_FromArray(pathBuffer);
	String_Format2(&path, "screenshots%r%s", &Directory_Separator, &filename);

	ReturnCode res; struct Stream stream;
	res = Stream_CreateFile(&stream, &path);
	if (res) { Chat_LogError2(res, "creating", &path); return; }

	res = Gfx_TakeScreenshot(&stream, Game_Width, Game_Height);
	if (res) { 
		Chat_LogError2(res, "saving to", &path); stream.Close(&stream); return;
	}

	res = stream.Close(&stream);
	if (res) { Chat_LogError2(res, "closing", &path); return; }

	Chat_Add1("&eTaken screenshot as: %s", &filename);
}

static void Game_RenderFrame(double delta) {
	Stopwatch_Measure(&game_frameTimer);

	Gfx_BeginFrame();
	Gfx_BindIb(GfxCommon_defaultIb);
	Game_Accumulator += delta;
	Game_Vertices = 0;

	Camera_Active->UpdateMouse();
	if (!Window_Focused && !Gui_GetActiveScreen()->HandlesAllInput) {
		Gui_FreeActive();
		Gui_SetActive(PauseScreen_MakeInstance());
	}

	bool allowZoom = !Gui_Active && !Gui_HUD->HandlesAllInput;
	if (allowZoom && KeyBind_IsPressed(KeyBind_ZoomScrolling)) {
		InputHandler_SetFOV(Game_ZoomFov, false);
	}

	Game_DoScheduledTasks(delta);
	struct ScheduledTask entTask = Game_Tasks[entTaskI];
	float t = (float)(entTask.Accumulator / entTask.Interval);
	LocalPlayer_SetInterpPosition(t);

	Gfx_Clear();
	Game_CurrentCameraPos = Camera_Active->GetPosition(t);
	Game_UpdateViewMatrix();

	bool visible = !Gui_Active || !Gui_Active->BlocksWorld;
	if (!World_Blocks) visible = false;
	if (visible) {
		Game_Render3D(delta, t);
	} else {
		PickedPos_SetAsInvalid(&Game_SelectedPos);
	}

	Gui_RenderGui(delta);
	if (Game_ScreenshotRequested) Game_TakeScreenshot();

	Gfx_EndFrame();
	if (game_limitMs) Game_LimitFPS();
}

void Game_Free(void* obj) {
	ChunkUpdater_Free();
	Atlas2D_Free();
	Atlas1D_Free();
	ModelCache_Free();
	Entities_Free();

	Event_UnregisterVoid(&WorldEvents_NewMap,         NULL, Game_OnNewMapCore);
	Event_UnregisterVoid(&WorldEvents_MapLoaded,      NULL, Game_OnNewMapLoadedCore);
	Event_UnregisterEntry(&TextureEvents_FileChanged, NULL, Game_TextureChangedCore);
	Event_UnregisterVoid(&GfxEvents_LowVRAMDetected,  NULL, Game_OnLowVRAMDetected);

	Event_UnregisterVoid(&WindowEvents_Resized,       NULL, Game_OnResize);
	Event_UnregisterVoid(&WindowEvents_Closed,        NULL, Game_Free);

	int i;
	for (i = 0; i < Game_ComponentsCount; i++) {
		Game_Components[i].Free();
	}

	Drawer2D_Free();
	Gfx_Free();

	if (!Options_HasAnyChanged()) return;
	Options_Load();
	Options_Save();
}

uint64_t game_renderTimer;
void Game_Run(int width, int height, const String* title, struct DisplayDevice* device) {
	int x = device->Bounds.X + (device->Bounds.Width  - width)  / 2;
	int y = device->Bounds.Y + (device->Bounds.Height - height) / 2;
	struct GraphicsMode mode = GraphicsMode_MakeDefault();

	Window_Create(x, y, width, height, title, &mode, device);
	Window_SetVisible(true);
	Game_Load();
	Event_RaiseVoid(&WindowEvents_Resized);

	Stopwatch_Measure(&game_renderTimer);
	for (;;) {
		Window_ProcessEvents();
		if (!Window_Exists) break;

		/* Limit maximum render to 1 second (for suspended process) */
		double time = Stopwatch_ElapsedMicroseconds(&game_renderTimer) / (1000.0 * 1000.0);
		if (time > 1.0) time = 1.0;
		if (time <= 0.0) continue;

		Stopwatch_Measure(&game_renderTimer);
		Game_RenderFrame(time);
	}
}
