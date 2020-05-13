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
#include "Input.h"
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
#include "Builder.h"
#include "GameStructs.h"
#include "Protocol.h"

struct _GameData Game;
int     Game_Port;
cc_bool Game_UseCPEBlocks;

struct RayTracer Game_SelectedPos;
int Game_ViewDistance, Game_MaxViewDistance, Game_UserViewDistance;
int Game_Fov, Game_DefaultFov, Game_ZoomFov;

int     Game_FpsLimit, Game_Vertices;
cc_bool Game_SimpleArmsAnim;

cc_bool Game_ClassicMode, Game_ClassicHacks;
cc_bool Game_AllowCustomBlocks, Game_UseCPE;
cc_bool Game_AllowServerTextures;

cc_bool Game_ViewBobbing, Game_HideGui;
cc_bool Game_BreakableLiquids, Game_ScreenshotRequested;
float   Game_RawHotbarScale, Game_RawChatScale, Game_RawInventoryScale;

static struct ScheduledTask Game_Tasks[6];
static int Game_TasksCount, entTaskI;

static char Game_UsernameBuffer[FILENAME_SIZE];
static char Game_MppassBuffer[STRING_SIZE];
static char Game_HashBuffer[STRING_SIZE];

String Game_Username = String_FromArray(Game_UsernameBuffer);
String Game_Mppass   = String_FromArray(Game_MppassBuffer);
String Game_Hash     = String_FromArray(Game_HashBuffer);

const char* const FpsLimit_Names[FPS_LIMIT_COUNT] = {
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
	float windowScale = min(WindowInfo.Width / 640.0f, WindowInfo.Height / 480.0f);
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

cc_bool Game_ChangeTerrainAtlas(Bitmap* atlas) {
	static const String terrain = String_FromConst("terrain.png");
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
	Game_Reset();
	DisconnectScreen_Show(title, reason);
}

void Game_Reset(void) {
	struct IGameComponent* comp;
	World_NewMap();

	if (World_TextureUrl.length) {
		World_TextureUrl.length = 0;
		TexturePack_ExtractCurrent(false);
	}

	for (comp = comps_head; comp; comp = comp->next) {
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

cc_bool Game_CanPick(BlockID block) {
	if (Blocks.Draw[block] == DRAW_GAS)    return false;
	if (Blocks.Draw[block] == DRAW_SPRITE) return true;
	return Blocks.Collide[block] != COLLIDE_LIQUID || Game_BreakableLiquids;
}

cc_bool Game_UpdateTexture(GfxResourceID* texId, struct Stream* src, const String* file, cc_uint8* skinType) {
	Bitmap bmp;
	cc_bool success;
	cc_result res;
	
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

cc_bool Game_ValidateBitmap(const String* file, Bitmap* bmp) {
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
	Game.Width  = max(WindowInfo.Width,  1);
	Game.Height = max(WindowInfo.Height, 1);
}

static void Game_OnResize(void* obj) {
	Game_UpdateDimensions();
	Gfx_OnWindowResize();
	Game_UpdateProjection();
	Gui_Layout();
}

static void HandleOnNewMap(void* obj) {
	struct IGameComponent* comp;
	for (comp = comps_head; comp; comp = comp->next) {
		if (comp->OnNewMap) comp->OnNewMap();
	}
}

static void HandleOnNewMapLoaded(void* obj) {
	struct IGameComponent* comp;
	for (comp = comps_head; comp; comp = comp->next) {
		if (comp->OnNewMapLoaded) comp->OnNewMapLoaded();
	}
}

static void HandleTextureChanged(void* obj, struct Stream* src, const String* name) {
	Bitmap bmp;
	cc_result res;

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

static void HandleLowVRAMDetected(void* obj) {
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

static void LoadOptions(void) {
	Game_ClassicMode       = Options_GetBool(OPT_CLASSIC_MODE, false);
	Game_ClassicHacks      = Options_GetBool(OPT_CLASSIC_HACKS, false);
	Game_AllowCustomBlocks = Options_GetBool(OPT_CUSTOM_BLOCKS, true);
	Game_UseCPE            = Options_GetBool(OPT_CPE, true);
	Game_SimpleArmsAnim    = Options_GetBool(OPT_SIMPLE_ARMS_ANIM, false);
	Game_ViewBobbing       = Options_GetBool(OPT_VIEW_BOBBING, true);

	Game_ViewDistance     = Options_GetInt(OPT_VIEW_DISTANCE, 8, 4096, 512);
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
	/*cc_bool skipSsl = Options_GetBool("skip-ssl-check", false);
	if (skipSsl) {
		ServicePointManager.ServerCertificateValidationCallback = delegate { return true; };
		Options.Set("skip-ssl-check", false);
	}*/
}

#if defined CC_BUILD_WEB
static void LoadPlugins(void) { }
#else
static void LoadPlugin(const String* path, void* obj) {
	void* lib;
	void* verSym;  /* EXPORT int Plugin_ApiVersion = GAME_API_VER; */
	void* compSym; /* EXPORT struct IGameComponent Plugin_Component = { (whatever) } */
	int ver;

	/* ignore accepted.txt, deskop.ini, .pdb files, etc */
	if (!String_CaselessEnds(path, &DynamicLib_Ext)) return;
	lib = DynamicLib_Load2(path);
	if (!lib) { Logger_DynamicLibWarn("loading", path); return; }

	verSym  = DynamicLib_Get2(lib, "Plugin_ApiVersion");
	if (!verSym)  { Logger_DynamicLibWarn("getting version of", path); return; }
	compSym = DynamicLib_Get2(lib, "Plugin_Component");
	if (!compSym) { Logger_DynamicLibWarn("initing", path); return; }

	ver = *((int*)verSym);
	if (ver < GAME_API_VER) {
		Chat_Add1("&c%s plugin is outdated! Try getting a more recent version.", path);
		return;
	} else if (ver > GAME_API_VER) {
		Chat_Add1("&cYour game is too outdated to use %s plugin! Try updating it.", path);
		return;
	}

	Game_AddComponent((struct IGameComponent*)compSym);
}

static void LoadPlugins(void) {
	static const String dir = String_FromConst("plugins");
	cc_result res;

	res = Directory_Enum(&dir, NULL, LoadPlugin);
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
	Game_UpdateDimensions();
	LoadOptions();

	Event_RegisterVoid(&WorldEvents.NewMap,         NULL, HandleOnNewMap);
	Event_RegisterVoid(&WorldEvents.MapLoaded,      NULL, HandleOnNewMapLoaded);
	Event_RegisterEntry(&TextureEvents.FileChanged, NULL, HandleTextureChanged);
	Event_RegisterVoid(&GfxEvents.LowVRAMDetected,  NULL, HandleLowVRAMDetected);

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

	Game_AddComponent(&Builder_Component);
	Game_AddComponent(&MapRenderer_Component);
	Game_AddComponent(&EnvRenderer_Component);
	Game_AddComponent(&Server_Component);
	Game_AddComponent(&Protocol_Component);
	Camera_Init();
	Game_UpdateProjection();

	Game_AddComponent(&Gui_Component);
	Game_AddComponent(&Selections_Component);
	Game_AddComponent(&HeldBlockRenderer_Component);

	Gfx_SetDepthTest(true);
	/* Gfx_SetDepthWrite(true) */

	Game_AddComponent(&PickedPosRenderer_Component);
	Game_AddComponent(&Audio_Component);
	Game_AddComponent(&AxisLinesRenderer_Component);

	LoadPlugins();
	for (comp = comps_head; comp; comp = comp->next) {
		if (comp->Init) comp->Init();
	}

	TexturePack_ExtractInitial();
	entTaskI = ScheduledTask_Add(GAME_DEF_TICKS, Entities_Tick);

	/* set vsync after because it causes a context loss depending on backend */
	Gfx_SetFpsLimit(true, 0);
	Game_SetFpsLimit(Options_GetEnum(OPT_FPS_LIMIT, 0, FpsLimit_Names, FPS_LIMIT_COUNT));

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

static void UpdateViewMatrix(void) {
	Camera.Active->GetView(&Gfx.View);
	Gfx_LoadMatrix(MATRIX_VIEW, &Gfx.View);
	FrustumCulling_CalcFrustumEquations(&Gfx.Projection, &Gfx.View);
}

static void Game_Render3D(double delta, float t) {
	Vec3 pos;

	EnvRenderer_UpdateFog();
	if (EnvRenderer_ShouldRenderSkybox()) EnvRenderer_RenderSkybox();

	AxisLinesRenderer_Render();
	Entities_RenderModels(delta, t);
	Entities_RenderNames();

	Particles_Render(t);
	Camera.Active->GetPickedBlock(&Game_SelectedPos); /* TODO: only pick when necessary */
	EnvRenderer_RenderSky();
	EnvRenderer_RenderClouds();

	MapRenderer_Update(delta);
	MapRenderer_RenderNormal(delta);
	EnvRenderer_RenderMapSides();

	Entities_DrawShadows();
	if (Game_SelectedPos.Valid && !Game_HideGui) {
		PickedPosRenderer_Render(&Game_SelectedPos, true);
	}

	/* Render water over translucent blocks when under the water outside the map for proper alpha blending */
	pos = Camera.CurrentPos;
	if (pos.Y < Env.EdgeHeight && (pos.X < 0 || pos.Z < 0 || pos.X > World.Width || pos.Z > World.Length)) {
		MapRenderer_RenderTranslucent(delta);
		EnvRenderer_RenderMapEdges();
	} else {
		EnvRenderer_RenderMapEdges();
		MapRenderer_RenderTranslucent(delta);
	}

	/* Need to render again over top of translucent block, as the selection outline */
	/* is drawn without writing to the depth buffer */
	if (Game_SelectedPos.Valid && !Game_HideGui && Blocks.Draw[Game_SelectedPos.block] == DRAW_TRANSLUCENT) {
		PickedPosRenderer_Render(&Game_SelectedPos, false);
	}

	Selections_Render();
	Entities_RenderHoveredNames();
	InputHandler_PickBlocks();
	if (!Game_HideGui) HeldBlockRenderer_Render(delta);
}

static void PerformScheduledTasks(double time) {
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
	cc_result res;

	Game_ScreenshotRequested = false;
	if (!Utils_EnsureDirectory("screenshots")) return;
	DateTime_CurrentLocal(&now);

	String_InitArray(filename, fileBuffer);
	String_Format3(&filename, "screenshot_%p2-%p2-%p4", &now.day, &now.month, &now.year);
	String_Format3(&filename, "-%p2-%p2-%p2.png", &now.hour, &now.minute, &now.second);
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
	float t;

	/* TODO: Should other tasks get called back too? */
	/* Might not be such a good idea for the http_clearcache, */
	/* don't really want all skins getting lost */
	if (Gfx.LostContext && !Gfx_TryRestoreContext()) {
		Server.Tick(NULL);
		Thread_Sleep(16);
		return;
	}

	Gfx_BeginFrame();
	Gfx_BindIb(Gfx_defaultIb);
	Game.Time += delta;
	Game_Vertices = 0;

	Camera.Active->UpdateMouse(delta);
	if (!WindowInfo.Focused && !Gui_GetInputGrab()) PauseScreen_Show();

	if (KeyBind_IsPressed(KEYBIND_ZOOM_SCROLL) && !Gui_GetInputGrab()) {
		InputHandler_SetFOV(Game_ZoomFov);
	}

	PerformScheduledTasks(delta);
	entTask = Game_Tasks[entTaskI];
	t = (float)(entTask.Accumulator / entTask.Interval);
	LocalPlayer_SetInterpPosition(t);

	Gfx_Clear();
	Camera.CurrentPos = Camera.Active->GetPosition(t);
	UpdateViewMatrix();

	if (!Gui_GetBlocksWorld()) {
		Game_Render3D(delta, t);
	} else {
		RayTracer_SetInvalid(&Game_SelectedPos);
	}

	Gui_RenderGui(delta);
	if (Game_ScreenshotRequested) Game_TakeScreenshot();
	Gfx_EndFrame();
}

void Game_Free(void* obj) {
	struct IGameComponent* comp;
	Atlas_Free();

	Event_UnregisterVoid(&WorldEvents.NewMap,         NULL, HandleOnNewMap);
	Event_UnregisterVoid(&WorldEvents.MapLoaded,      NULL, HandleOnNewMapLoaded);
	Event_UnregisterEntry(&TextureEvents.FileChanged, NULL, HandleTextureChanged);
	Event_UnregisterVoid(&GfxEvents.LowVRAMDetected,  NULL, HandleLowVRAMDetected);

	Event_UnregisterVoid(&WindowEvents.Resized,       NULL, Game_OnResize);
	Event_UnregisterVoid(&WindowEvents.Closing,       NULL, Game_Free);

	for (comp = comps_head; comp; comp = comp->next) {
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
	if (!WindowInfo.Exists) return;\
	\
	render = Stopwatch_Measure();\
	time   = Stopwatch_ElapsedMicroseconds(lastRender, render) / (1000.0 * 1000.0);\
	\
	if (time > 1.0) time = 1.0; /* avoid large delta with suspended process */ \
	if (time > 0.0) { lastRender = Stopwatch_Measure(); Game_RenderFrame(time); }

#ifdef CC_BUILD_WEB
#include <emscripten.h>
static cc_uint64 lastRender;

static void Game_DoFrame(void) {
	cc_uint64 render; 
	double time;
	Game_DoFrameBody()
}

static void Game_RunLoop(void) {
	lastRender = Stopwatch_Measure();
	emscripten_set_main_loop(Game_DoFrame, 0, false);
}
#else
static void Game_RunLoop(void) {
	cc_uint64 lastRender, render; 
	double time;
	lastRender = Stopwatch_Measure();
	for (;;) { Game_DoFrameBody() }
}
#endif

void Game_Run(int width, int height, const String* title) {
	Window_Create(width, height);
	Window_SetTitle(title);
	Window_Show();

	Game_Load();
	Event_RaiseVoid(&WindowEvents.Resized);
	Game_RunLoop();
}
