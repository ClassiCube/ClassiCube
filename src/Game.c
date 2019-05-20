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
#include "Logger.h"
#include "Entity.h"
#include "Chat.h"
#include "Drawer2D.h"
#include "Model.h"
#include "Particle.h"
#include "Http.h"
#include "Inventory.h"
#include "InputHandler.h"
#include "Server.h"
#include "TexturePack.h"
#include "Screens.h"
#include "SelectionBox.h"
#include "AxisLinesRenderer.h"
#include "EnvRenderer.h"
#include "HeldBlockRenderer.h"
#include "PickedPosRenderer.h"
#include "Menus.h"
#include "Audio.h"
#include "Stream.h"

struct _GameData Game;
int  Game_Port;
bool Game_UseCPEBlocks;

struct PickedPos Game_SelectedPos;
int Game_ViewDistance, Game_MaxViewDistance, Game_UserViewDistance;
int Game_Fov, Game_DefaultFov, Game_ZoomFov;

int  Game_FpsLimit, Game_Vertices;
bool Game_SimpleArmsAnim;

bool Game_ClassicMode, Game_ClassicHacks;
bool Game_AllowCustomBlocks, Game_UseCPE;
bool Game_AllowServerTextures;

bool Game_ViewBobbing, Game_HideGui;
bool Game_BreakableLiquids, Game_ScreenshotRequested;
float Game_RawHotbarScale, Game_RawChatScale, Game_RawInventoryScale;

static struct ScheduledTask Game_Tasks[6];
static int Game_TasksCount, entTaskI;

static char Game_UsernameBuffer[FILENAME_SIZE];
static char Game_MppassBuffer[STRING_SIZE];
static char Game_HashBuffer[STRING_SIZE];

String Game_Username = String_FromArray(Game_UsernameBuffer);
String Game_Mppass   = String_FromArray(Game_MppassBuffer);
String Game_Hash     = String_FromArray(Game_HashBuffer);

const char* FpsLimit_Names[FPS_LIMIT_COUNT] = {
	"LimitVSync", "Limit30FPS", "Limit60FPS", "Limit120FPS", "Limit144FPS", "LimitNone",
};

static struct IGameComponent* comps_head;
static struct IGameComponent* comps_tail;
void Game_AddComponent(struct IGameComponent* comp) {
	LinkedList_Add(comp, comps_head, comps_tail);
}

int ScheduledTask_Add(double interval, ScheduledTaskCallback callback) {
	struct ScheduledTask task;
	task.Accumulator = 0.0;
	task.Interval    = interval;
	task.Callback    = callback;

	if (Game_TasksCount == Array_Elems(Game_Tasks)) {
		Logger_Abort("ScheduledTask_Add - hit max count");
	}
	Game_Tasks[Game_TasksCount++] = task;
	return Game_TasksCount - 1;
}


int Game_GetWindowScale(void) {
	float windowScale = min(Game.Width / 640.0f, Game.Height / 480.0f);
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

static char game_defTexPackBuffer[STRING_SIZE];
static String game_defTexPack = String_FromArray(game_defTexPackBuffer);

void Game_GetDefaultTexturePack(String* texPack) {
	String texPath; char texPathBuffer[STRING_SIZE];

	String_InitArray(texPath, texPathBuffer);
	String_Format1(&texPath, "texpacks/%s", &game_defTexPack);

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
	const static String terrain = String_FromConst("terrain.png");
	if (!Game_ValidateBitmap(&terrain, atlas)) return false;

	if (atlas->Height < atlas->Width) {
		Chat_AddRaw("&cUnable to use terrain.png from the texture pack.");
		Chat_AddRaw("&c Its height is less than its width.");
		return false;
	}
	if (atlas->Width < ATLAS2D_TILES_PER_ROW) {
		Chat_AddRaw("&cUnable to use terrain.png from the texture pack.");
		Chat_AddRaw("&c It must be 16 or more pixels wide.");
		return false;
	}
	if (Gfx.LostContext) return false;

	Atlas_Free();
	Atlas_Update(atlas);

	Event_RaiseVoid(&TextureEvents.AtlasChanged);
	return true;
}

void Game_SetViewDistance(int distance) {
	distance = min(distance, Game_MaxViewDistance);
	if (distance == Game_ViewDistance) return;
	Game_ViewDistance = distance;

	Event_RaiseVoid(&GfxEvents.ViewDistanceChanged);
	Game_UpdateProjection();
}

void Game_UserSetViewDistance(int distance) {
	Game_UserViewDistance = distance;
	Options_SetInt(OPT_VIEW_DISTANCE, distance);
	Game_SetViewDistance(distance);
}

void Game_SetFov(int fov) {
	if (Game_Fov == fov) return;
	Game_Fov = fov;
	Game_UpdateProjection();
}

void Game_UpdateProjection(void) {
	Camera.Active->GetProjection(&Gfx.Projection);
	Gfx_LoadMatrix(MATRIX_PROJECTION, &Gfx.Projection);
	Event_RaiseVoid(&GfxEvents.ProjectionChanged);
}

void Game_Disconnect(const String* title, const String* reason) {
	Event_RaiseVoid(&NetEvents.Disconnected);
	Gui_FreeActive();
	Gui_SetActive(DisconnectScreen_MakeInstance(title, reason));
	Game_Reset();
}

void Game_Reset(void) {
	struct IGameComponent* comp;
	World_Reset();
	Event_RaiseVoid(&WorldEvents.NewMap);

	if (World_TextureUrl.length) {
		TexturePack_ExtractDefault();
		World_TextureUrl.length = 0;
	}

	for (comp = comps_head; comp; comp = comp->Next) {
		if (comp->Reset) comp->Reset();
	}
}

void Game_UpdateBlock(int x, int y, int z, BlockID block) {
	struct ChunkInfo* chunk;
	int cx = x >> 4, cy = y >> 4, cz = z >> 4;
	BlockID old = World_GetBlock(x, y, z);
	World_SetBlock(x, y, z, block);

	if (Weather_Heightmap) {
		EnvRenderer_OnBlockChanged(x, y, z, old, block);
	}
	Lighting_OnBlockChanged(x, y, z, old, block);

	/* Refresh the chunk the block was located in. */
	chunk = MapRenderer_GetChunk(cx, cy, cz);
	chunk->AllAir &= Blocks.Draw[block] == DRAW_GAS;
	MapRenderer_RefreshChunk(cx, cy, cz);
}

void Game_ChangeBlock(int x, int y, int z, BlockID block) {
	BlockID old = World_GetBlock(x, y, z);
	Game_UpdateBlock(x, y, z, block);
	Server.SendBlock(x, y, z, old, block);
}

bool Game_CanPick(BlockID block) {
	if (Blocks.Draw[block] == DRAW_GAS)    return false;
	if (Blocks.Draw[block] == DRAW_SPRITE) return true;
	return Blocks.Collide[block] != COLLIDE_LIQUID || Game_BreakableLiquids;
}

bool Game_UpdateTexture(GfxResourceID* texId, struct Stream* src, const String* file, uint8_t* skinType) {
	Bitmap bmp;
	bool success;
	ReturnCode res;
	
	res = Png_Decode(&bmp, src);
	if (res) { Logger_Warn2(res, "decoding", file); }

	success = !res && Game_ValidateBitmap(file, &bmp);
	if (success) {
		Gfx_DeleteTexture(texId);
		if (skinType) { *skinType = Utils_CalcSkinType(&bmp); }
		*texId = Gfx_CreateTexture(&bmp, true, false);
	}

	Mem_Free(bmp.Scan0);
	return success;
}

bool Game_ValidateBitmap(const String* file, Bitmap* bmp) {
	int maxWidth = Gfx.MaxTexWidth, maxHeight = Gfx.MaxTexHeight;
	if (!bmp->Scan0) {
		Chat_Add1("&cError loading %s from the texture pack.", file);
		return false;
	}
	
	if (bmp->Width > maxWidth || bmp->Height > maxHeight) {
		Chat_Add1("&cUnable to use %s from the texture pack.", file);

		Chat_Add4("&c Its size is (%i,%i), your GPU supports (%i,%i) at most.", 
			&bmp->Width, &bmp->Height, &maxWidth, &maxHeight);
		return false;
	}

	if (!Math_IsPowOf2(bmp->Width) || !Math_IsPowOf2(bmp->Height)) {
		Chat_Add1("&cUnable to use %s from the texture pack.", file);

		Chat_Add2("&c Its size is (%i,%i), which is not a power of two size.", 
			&bmp->Width, &bmp->Height);
		return false;
	}
	return true;
}

void Game_UpdateDimensions(void) {
	Game.Width  = max(Window_ClientBounds.Width,  1);
	Game.Height = max(Window_ClientBounds.Height, 1);
}

static void Game_OnResize(void* obj) {
	Game_UpdateDimensions();
	Gfx_OnWindowResize();
	Game_UpdateProjection();
	Gui_OnResize();
}

static void Game_OnNewMapCore(void* obj) {
	struct IGameComponent* comp;
	for (comp = comps_head; comp; comp = comp->Next) {
		if (comp->OnNewMap) comp->OnNewMap();
	}
}

static void Game_OnNewMapLoadedCore(void* obj) {
	struct IGameComponent* comp;
	for (comp = comps_head; comp; comp = comp->Next) {
		if (comp->OnNewMapLoaded) comp->OnNewMapLoaded();
	}
}

static void Game_TextureChangedCore(void* obj, struct Stream* src, const String* name) {
	Bitmap bmp;
	ReturnCode res;

	if (String_CaselessEqualsConst(name, "terrain.png")) {
		res = Png_Decode(&bmp, src);

		if (res) { 
			Logger_Warn2(res, "decoding", name);
			Mem_Free(bmp.Scan0);
		} else if (!Game_ChangeTerrainAtlas(&bmp)) {
			Mem_Free(bmp.Scan0);
		}		
	}
}

static void Game_OnLowVRAMDetected(void* obj) {
	if (Game_UserViewDistance <= 16) Logger_Abort("Out of video memory!");
	Game_UserViewDistance /= 2;
	Game_UserViewDistance = max(16, Game_UserViewDistance);

	MapRenderer_Refresh();
	Game_SetViewDistance(Game_UserViewDistance);
	Chat_AddRaw("&cOut of VRAM! Halving view distance..");
}

static void Game_WarnFunc(const String* msg) {
	String str = *msg, line;
	while (str.length) {
		String_UNSAFE_SplitBy(&str, '\n', &line);
		Chat_Add1("&c%s", &line);
	}
}

static void Game_ExtractInitialTexturePack(void) {
	String texPack; char texPackBuffer[STRING_SIZE];

	Options_Get(OPT_DEFAULT_TEX_PACK, &game_defTexPack, "default.zip");
	String_InitArray(texPack, texPackBuffer);

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
	int method;
	Game_ClassicMode       = Options_GetBool(OPT_CLASSIC_MODE, false);
	Game_ClassicHacks      = Options_GetBool(OPT_CLASSIC_HACKS, false);
	Game_AllowCustomBlocks = Options_GetBool(OPT_CUSTOM_BLOCKS, true);
	Game_UseCPE            = Options_GetBool(OPT_CPE, true);
	Game_SimpleArmsAnim    = Options_GetBool(OPT_SIMPLE_ARMS_ANIM, false);
	Game_ViewBobbing       = Options_GetBool(OPT_VIEW_BOBBING, true);

	method = Options_GetEnum(OPT_FPS_LIMIT, 0, FpsLimit_Names, FPS_LIMIT_COUNT);
	Game_SetFpsLimit(method);
	Game_ViewDistance     = Options_GetInt(OPT_VIEW_DISTANCE, 16, 4096, 512);
	Game_UserViewDistance = Game_ViewDistance;

	Game_DefaultFov = Options_GetInt(OPT_FIELD_OF_VIEW, 1, 179, 70);
	Game_Fov        = Game_DefaultFov;
	Game_ZoomFov    = Game_DefaultFov;
	Game_BreakableLiquids = !Game_ClassicMode && Options_GetBool(OPT_MODIFIABLE_LIQUIDS, false);
	Game_AllowServerTextures = Options_GetBool(OPT_SERVER_TEXTURES, true);

	Game_RawInventoryScale = Options_GetFloat(OPT_INVENTORY_SCALE, 0.25f, 5.0f, 1.0f);
	Game_RawHotbarScale    = Options_GetFloat(OPT_HOTBAR_SCALE,    0.25f, 5.0f, 1.0f);
	Game_RawChatScale      = Options_GetFloat(OPT_CHAT_SCALE,      0.35f, 5.0f, 1.0f);
	/* TODO: Do we need to support option to skip SSL */
	/*bool skipSsl = Options_GetBool("skip-ssl-check", false);
	if (skipSsl) {
		ServicePointManager.ServerCertificateValidationCallback = delegate { return true; };
		Options.Set("skip-ssl-check", false);
	}*/
}

#if defined CC_BUILD_WEB
static void Game_LoadPlugins(void) { }
#else
static void Game_LoadPlugin(const String* path, void* obj) {
#if defined CC_BUILD_WIN
	const static String ext = String_FromConst(".dll");
#elif defined CC_BUILD_OSX
	const static String ext = String_FromConst(".dylib");
#else
	const static String ext = String_FromConst(".so");
#endif

	void* lib;
	void* verSymbol;  /* EXPORT int Plugin_ApiVersion = GAME_API_VER; */
	void* compSymbol; /* EXPORT struct IGameComponent Plugin_Component = { (whatever) } */
	int ver;
	ReturnCode res;

	/* ignore accepted.txt, deskop.ini, .pdb files, etc */
	if (!String_CaselessEnds(path, &ext)) return;
	res = DynamicLib_Load(path, &lib);
	if (res) { Logger_DynamicLibWarn2(res, "loading plugin", path); return; }

	res = DynamicLib_Get(lib, "Plugin_ApiVersion", &verSymbol);
	if (res) { Logger_DynamicLibWarn2(res, "getting version of", path); return; }
	res = DynamicLib_Get(lib, "Plugin_Component", &compSymbol);
	if (res) { Logger_DynamicLibWarn2(res, "initing", path); return; }

	ver = *((int*)verSymbol);
	if (ver < GAME_API_VER) {
		Chat_Add1("&c%s plugin is outdated! Try getting a more recent version.", path);
		return;
	} else if (ver > GAME_API_VER) {
		Chat_Add1("&cYour game is too outdated to use %s plugin! Try updating it.", path);
		return;
	}

	Game_AddComponent((struct IGameComponent*)compSymbol);
}

static void Game_LoadPlugins(void) {
	const static String dir = String_FromConst("plugins");
	ReturnCode res;

	res = Directory_Enum(&dir, NULL, Game_LoadPlugin);
	if (res) Logger_Warn(res, "enumerating plugins directory");
}
#endif

void Game_Free(void* obj);
static void Game_Load(void) {
	struct IGameComponent* comp;
	Logger_WarnFunc = Game_WarnFunc;

	Game_ViewDistance     = 512;
	Game_MaxViewDistance  = 32768;
	Game_UserViewDistance = 512;
	Game_Fov = 70;

	Gfx_Init();
	Gfx_SetFpsLimit(true, 0);
	Gfx_MakeApiInfo();
	Gfx.Mipmaps = Options_GetBool(OPT_MIPMAPS, false);

	Game_UpdateDimensions();
	Game_LoadOptions();

	Event_RegisterVoid(&WorldEvents.NewMap,         NULL, Game_OnNewMapCore);
	Event_RegisterVoid(&WorldEvents.MapLoaded,      NULL, Game_OnNewMapLoadedCore);
	Event_RegisterEntry(&TextureEvents.FileChanged, NULL, Game_TextureChangedCore);
	Event_RegisterVoid(&GfxEvents.LowVRAMDetected,  NULL, Game_OnLowVRAMDetected);

	Event_RegisterVoid(&WindowEvents.Resized,       NULL, Game_OnResize);
	Event_RegisterVoid(&WindowEvents.Closing,       NULL, Game_Free);

	TextureCache_Init();
	/* TODO: Survival vs Creative game mode */

	InputHandler_Init();
	Game_AddComponent(&Blocks_Component);
	Game_AddComponent(&Drawer2D_Component);

	Game_AddComponent(&Chat_Component);
	Game_AddComponent(&Particles_Component);
	Game_AddComponent(&TabList_Component);

	Game_AddComponent(&Models_Component);
	Game_AddComponent(&Entities_Component);
	Game_AddComponent(&Http_Component);
	Game_AddComponent(&Lighting_Component);

	Game_AddComponent(&Animations_Component);
	Game_AddComponent(&Inventory_Component);
	World_Reset();

	Game_AddComponent(&MapRenderer_Component);
	Game_AddComponent(&EnvRenderer_Component);
	Game_AddComponent(&Server_Component);
	Camera_Init();
	Game_UpdateProjection();

	Game_AddComponent(&Gui_Component);
	Game_AddComponent(&Selections_Component);
	Game_AddComponent(&HeldBlockRenderer_Component);

	Gfx_SetDepthTest(true);
	Gfx_SetDepthTestFunc(COMPARE_FUNC_LESSEQUAL);
	/* Gfx_SetDepthWrite(true) */
	Gfx_SetAlphaBlendFunc(BLEND_FUNC_SRC_ALPHA, BLEND_FUNC_INV_SRC_ALPHA);
	Gfx_SetAlphaTestFunc(COMPARE_FUNC_GREATER, 0.5f);

	Game_AddComponent(&PickedPosRenderer_Component);
	Game_AddComponent(&Audio_Component);
	Game_AddComponent(&AxisLinesRenderer_Component);

	Game_LoadPlugins();
	for (comp = comps_head; comp; comp = comp->Next) {
		if (comp->Init) comp->Init();
	}

	Game_ExtractInitialTexturePack();
	entTaskI = ScheduledTask_Add(GAME_DEF_TICKS, Entities_Tick);

	if (Gfx_WarnIfNecessary()) EnvRenderer_SetMode(EnvRenderer_Minimal | ENV_LEGACY);
	Server.BeginConnect();
}

void Game_SetFpsLimit(int method) {
	float minFrameTime = 0;
	Game_FpsLimit = method;

	switch (method) {
	case FPS_LIMIT_144: minFrameTime = 1000/144.0f; break;
	case FPS_LIMIT_120: minFrameTime = 1000/120.0f; break;
	case FPS_LIMIT_60:  minFrameTime = 1000/60.0f;  break;
	case FPS_LIMIT_30:  minFrameTime = 1000/30.0f;  break;
	}
	Gfx_SetFpsLimit(method == FPS_LIMIT_VSYNC, minFrameTime);
}

static void Game_UpdateViewMatrix(void) {
	Camera.Active->GetView(&Gfx.View);
	Gfx_LoadMatrix(MATRIX_VIEW, &Gfx.View);
	FrustumCulling_CalcFrustumEquations(&Gfx.Projection, &Gfx.View);
}

static void Game_Render3D(double delta, float t) {
	Vector3 pos;
	bool left, middle, right;

	if (EnvRenderer_ShouldRenderSkybox()) EnvRenderer_RenderSkybox(delta);
	AxisLinesRenderer_Render(delta);
	Entities_RenderModels(delta, t);
	Entities_RenderNames(delta);

	Particles_Render(delta, t);
	Camera.Active->GetPickedBlock(&Game_SelectedPos); /* TODO: only pick when necessary */

	EnvRenderer_UpdateFog();
	EnvRenderer_RenderSky(delta);
	EnvRenderer_RenderClouds(delta);

	MapRenderer_Update(delta);
	MapRenderer_RenderNormal(delta);
	EnvRenderer_RenderMapSides(delta);

	Entities_DrawShadows();
	if (Game_SelectedPos.Valid && !Game_HideGui) {
		PickedPosRenderer_Update(&Game_SelectedPos);
		PickedPosRenderer_Render(delta);
	}

	/* Render water over translucent blocks when underwater for proper alpha blending */
	pos = LocalPlayer_Instance.Base.Position;
	if (Camera.CurrentPos.Y < Env.EdgeHeight && (pos.X < 0 || pos.Z < 0 || pos.X > World.Width || pos.Z > World.Length)) {
		MapRenderer_RenderTranslucent(delta);
		EnvRenderer_RenderMapEdges(delta);
	} else {
		EnvRenderer_RenderMapEdges(delta);
		MapRenderer_RenderTranslucent(delta);
	}

	/* Need to render again over top of translucent block, as the selection outline */
	/* is drawn without writing to the depth buffer */
	if (Game_SelectedPos.Valid && !Game_HideGui && Blocks.Draw[Game_SelectedPos.Block] == DRAW_TRANSLUCENT) {
		PickedPosRenderer_Render(delta);
	}

	Selections_Render(delta);
	Entities_RenderHoveredNames(delta);

	left   = InputHandler_IsMousePressed(MOUSE_LEFT);
	middle = InputHandler_IsMousePressed(MOUSE_MIDDLE);
	right  = InputHandler_IsMousePressed(MOUSE_RIGHT);

	InputHandler_PickBlocks(true, left, middle, right);
	if (!Game_HideGui) HeldBlockRenderer_Render(delta);
}

static void Game_DoScheduledTasks(double time) {
	struct ScheduledTask task;
	int i;

	for (i = 0; i < Game_TasksCount; i++) {
		task = Game_Tasks[i];
		task.Accumulator += time;

		while (task.Accumulator >= task.Interval) {
			task.Callback(&task);
			task.Accumulator -= task.Interval;
		}
		Game_Tasks[i] = task;
	}
}

void Game_TakeScreenshot(void) {
#ifndef CC_BUILD_WEB
	String filename; char fileBuffer[STRING_SIZE];
	String path;     char pathBuffer[FILENAME_SIZE];
	struct DateTime now;
	struct Stream stream;
	ReturnCode res;

	Game_ScreenshotRequested = false;
	if (!Utils_EnsureDirectory("screenshots")) return;
	DateTime_CurrentLocal(&now);

	String_InitArray(filename, fileBuffer);
	String_Format3(&filename, "screenshot_%p2-%p2-%p4", &now.Day, &now.Month, &now.Year);
	String_Format3(&filename, "-%p2-%p2-%p2.png", &now.Hour, &now.Minute, &now.Second);
	String_InitArray(path, pathBuffer);
	String_Format1(&path, "screenshots/%s", &filename);

	res = Stream_CreateFile(&stream, &path);
	if (res) { Logger_Warn2(res, "creating", &path); return; }

	res = Gfx_TakeScreenshot(&stream);
	if (res) { 
		Logger_Warn2(res, "saving to", &path); stream.Close(&stream); return;
	}

	res = stream.Close(&stream);
	if (res) { Logger_Warn2(res, "closing", &path); return; }

	Chat_Add1("&eTaken screenshot as: %s", &filename);
#endif
}

static void Game_RenderFrame(double delta) {
	struct ScheduledTask entTask;
	bool allowZoom, visible;
	float t;

	Gfx_BeginFrame();
	Gfx_BindIb(Gfx_defaultIb);
	Game.Time += delta;
	Game_Vertices = 0;

	Camera.Active->UpdateMouse(delta);
	if (!Window_Focused && !Gui_GetActiveScreen()->HandlesAllInput) {
		Gui_FreeActive();
		Gui_SetActive(PauseScreen_MakeInstance());
	}

	allowZoom = !Gui_Active && !Gui_HUD->HandlesAllInput;
	if (allowZoom && KeyBind_IsPressed(KEYBIND_ZOOM_SCROLL)) {
		InputHandler_SetFOV(Game_ZoomFov);
	}

	Game_DoScheduledTasks(delta);
	entTask = Game_Tasks[entTaskI];
	t = (float)(entTask.Accumulator / entTask.Interval);
	LocalPlayer_SetInterpPosition(t);

	Gfx_Clear();
	Camera.CurrentPos = Camera.Active->GetPosition(t);
	Game_UpdateViewMatrix();

	visible = !Gui_Active || !Gui_Active->BlocksWorld;
	if (visible && World.Blocks) {
		Game_Render3D(delta, t);
	} else {
		PickedPos_SetAsInvalid(&Game_SelectedPos);
	}

	Gui_RenderGui(delta);
	if (Game_ScreenshotRequested) Game_TakeScreenshot();
	Gfx_EndFrame();
}

void Game_Free(void* obj) {
	struct IGameComponent* comp;
	Atlas_Free();

	Event_UnregisterVoid(&WorldEvents.NewMap,         NULL, Game_OnNewMapCore);
	Event_UnregisterVoid(&WorldEvents.MapLoaded,      NULL, Game_OnNewMapLoadedCore);
	Event_UnregisterEntry(&TextureEvents.FileChanged, NULL, Game_TextureChangedCore);
	Event_UnregisterVoid(&GfxEvents.LowVRAMDetected,  NULL, Game_OnLowVRAMDetected);

	Event_UnregisterVoid(&WindowEvents.Resized,       NULL, Game_OnResize);
	Event_UnregisterVoid(&WindowEvents.Closing,       NULL, Game_Free);

	for (comp = comps_head; comp; comp = comp->Next) {
		if (comp->Free) comp->Free();
	}

	Logger_WarnFunc = Logger_DialogWarn;
	Gfx_Free();

	if (!Options_ChangedCount()) return;
	Options_Load();
	Options_Save();
}

#define Game_DoFrameBody() \
	Window_ProcessEvents();\
	if (!Window_Exists) return;\
	\
	render = Stopwatch_Measure();\
	time   = Stopwatch_ElapsedMicroseconds(lastRender, render) / (1000.0 * 1000.0);\
	\
	if (time > 1.0) time = 1.0; /* avoid large delta with suspended process */ \
	if (time > 0.0) { lastRender = Stopwatch_Measure(); Game_RenderFrame(time); }

#ifdef CC_BUILD_WEB
#include <emscripten.h>
static uint64_t lastRender;

static void Game_DoFrame(void) {
	uint64_t render; 
	double time;
	Game_DoFrameBody()
}

static void Game_RunLoop(void) {
	lastRender = Stopwatch_Measure();
	emscripten_set_main_loop(Game_DoFrame, 0, false);
}
#else
static void Game_RunLoop(void) {
	uint64_t lastRender, render; 
	double time;
	lastRender = Stopwatch_Measure();
	for (;;) { Game_DoFrameBody() }
}
#endif

void Game_Run(int width, int height, const String* title) {
	Window_CreateSimple(width, height);
	Window_SetTitle(title);
	Window_SetVisible(true);

	Game_Load();
	Event_RaiseVoid(&WindowEvents.Resized);
	Game_RunLoop();
}
