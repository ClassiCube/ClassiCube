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
#include "Protocol.h"
#include "Picking.h"
#include "Animations.h"

struct _GameData Game;
cc_uint64 Game_FrameStart;
cc_bool Game_UseCPEBlocks;

struct RayTracer Game_SelectedPos;
int Game_ViewDistance = 512, Game_UserViewDistance = 512;
int Game_MaxViewDistance = DEFAULT_MAX_VIEWDIST;

int     Game_FpsLimit, Game_Vertices;
cc_bool Game_SimpleArmsAnim;

cc_bool Game_ClassicMode, Game_ClassicHacks;
cc_bool Game_AllowCustomBlocks, Game_UseCPE;
cc_bool Game_AllowServerTextures;

cc_bool Game_ViewBobbing, Game_HideGui, Game_DefaultZipMissing;
cc_bool Game_BreakableLiquids, Game_ScreenshotRequested;
struct GameVersion Game_Version;

static char usernameBuffer[STRING_SIZE];
static char mppassBuffer[STRING_SIZE];
cc_string Game_Username = String_FromArray(usernameBuffer);
cc_string Game_Mppass   = String_FromArray(mppassBuffer);

const char* const FpsLimit_Names[FPS_LIMIT_COUNT] = {
	"LimitVSync", "Limit30FPS", "Limit60FPS", "Limit120FPS", "Limit144FPS", "LimitNone",
};

static struct IGameComponent* comps_head;
static struct IGameComponent* comps_tail;
void Game_AddComponent(struct IGameComponent* comp) {
	LinkedList_Append(comp, comps_head, comps_tail);
}

#define TASKS_DEF_ELEMS 6
static struct ScheduledTask defaultTasks[TASKS_DEF_ELEMS];
static int tasksCapacity = TASKS_DEF_ELEMS, tasksCount, entTaskI;
static struct ScheduledTask* tasks = defaultTasks;

int ScheduledTask_Add(double interval, ScheduledTaskCallback callback) {
	struct ScheduledTask task;
	task.accumulator = 0.0;
	task.interval    = interval;
	task.Callback    = callback;

	if (tasksCount == tasksCapacity) {
		Utils_Resize((void**)&tasks, &tasksCapacity,
			sizeof(struct ScheduledTask), TASKS_DEF_ELEMS, TASKS_DEF_ELEMS);
	}

	tasks[tasksCount++] = task;
	return tasksCount - 1;
}


void Game_ToggleFullscreen(void) {
	int state = Window_GetWindowState();
	cc_result res;

	if (state == WINDOW_STATE_FULLSCREEN) {
		res = Window_ExitFullscreen();
		if (res) Logger_SysWarn(res, "leaving fullscreen");
	} else {
		res = Window_EnterFullscreen();
		if (res) Logger_SysWarn(res, "going fullscreen");
	}
}

static void CycleViewDistanceForwards(const short* viewDists, int count) {
	int i, dist;
	for (i = 0; i < count; i++) {
		dist = viewDists[i];

		if (dist > Game_UserViewDistance) {
			Game_UserSetViewDistance(dist); return;
		}
	}
	Game_UserSetViewDistance(viewDists[0]);
}

static void CycleViewDistanceBackwards(const short* viewDists, int count) {
	int i, dist;
	for (i = count - 1; i >= 0; i--) {
		dist = viewDists[i];

		if (dist < Game_UserViewDistance) {
			Game_UserSetViewDistance(dist); return;
		}
	}
	Game_UserSetViewDistance(viewDists[count - 1]);
}

static const short normDists[10]   = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };
static const short classicDists[4] = { 8, 32, 128, 512 };
void Game_CycleViewDistance(void) {
	const short* dists = Gui.ClassicMenu ? classicDists : normDists;
	int count = Gui.ClassicMenu ? Array_Elems(classicDists) : Array_Elems(normDists);

	if (Key_IsShiftPressed()) {
		CycleViewDistanceBackwards(dists, count);
	} else {
		CycleViewDistanceForwards(dists, count);
	}
}

cc_bool Game_ReduceVRAM(void) {
	if (Game_UserViewDistance <= 16) return false;
	Game_UserViewDistance /= 2;
	Game_UserViewDistance = max(16, Game_UserViewDistance);

	MapRenderer_Refresh();
	Game_SetViewDistance(Game_UserViewDistance);
	Chat_AddRaw("&cOut of VRAM! Halving view distance..");
	return true;
}


void Game_SetViewDistance(int distance) {
	distance = min(distance, Game_MaxViewDistance);
	if (distance == Game_ViewDistance) return;
	Game_ViewDistance = distance;

	Event_RaiseVoid(&GfxEvents.ViewDistanceChanged);
	Camera_UpdateProjection();
}

void Game_UserSetViewDistance(int distance) {
	Game_UserViewDistance = distance;
	Options_SetInt(OPT_VIEW_DISTANCE, distance);
	Game_SetViewDistance(distance);
}

void Game_Disconnect(const cc_string* title, const cc_string* reason) {
	Event_RaiseVoid(&NetEvents.Disconnected);
	Game_Reset();
	DisconnectScreen_Show(title, reason);
}

void Game_Reset(void) {
	struct IGameComponent* comp;
	World_NewMap();

	for (comp = comps_head; comp; comp = comp->next) {
		if (comp->Reset) comp->Reset();
	}
}

void Game_UpdateBlock(int x, int y, int z, BlockID block) {
	BlockID old = World_GetBlock(x, y, z);
	World_SetBlock(x, y, z, block);

	if (Weather_Heightmap) {
		EnvRenderer_OnBlockChanged(x, y, z, old, block);
	}
	Lighting.OnBlockChanged(x, y, z, old, block);
	MapRenderer_OnBlockChanged(x, y, z, block);
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

cc_bool Game_UpdateTexture(GfxResourceID* texId, struct Stream* src, const cc_string* file, cc_uint8* skinType) {
	struct Bitmap bmp;
	cc_bool success;
	cc_result res;
	
	res = Png_Decode(&bmp, src);
	if (res) { Logger_SysWarn2(res, "decoding", file); }

	success = !res && Game_ValidateBitmap(file, &bmp);
	if (success) {
		if (skinType) { *skinType = Utils_CalcSkinType(&bmp); }
		Gfx_RecreateTexture(texId, &bmp, TEXTURE_FLAG_MANAGED, false);
	}

	Mem_Free(bmp.scan0);
	return success;
}

cc_bool Game_ValidateBitmap(const cc_string* file, struct Bitmap* bmp) {
	int maxWidth = Gfx.MaxTexWidth, maxHeight = Gfx.MaxTexHeight;
	if (!bmp->scan0) {
		Chat_Add1("&cError loading %s from the texture pack.", file);
		return false;
	}
	
	if (bmp->width > maxWidth || bmp->height > maxHeight) {
		Chat_Add1("&cUnable to use %s from the texture pack.", file);

		Chat_Add4("&c Its size is (%i,%i), your GPU supports (%i,%i) at most.", 
			&bmp->width, &bmp->height, &maxWidth, &maxHeight);
		return false;
	}

	if (!Math_IsPowOf2(bmp->width) || !Math_IsPowOf2(bmp->height)) {
		Chat_Add1("&cUnable to use %s from the texture pack.", file);

		Chat_Add2("&c Its size is (%i,%i), which is not a power of two size.", 
			&bmp->width, &bmp->height);
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
	Camera_UpdateProjection();
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

static void HandleInactiveChanged(void* obj) {
	if (WindowInfo.Inactive) {
		Chat_AddOf(&Gfx_LowPerfMessage, MSG_TYPE_EXTRASTATUS_2);
		Gfx_SetFpsLimit(false, 1000 / 1.0f);
	} else {
		Chat_AddOf(&String_Empty,       MSG_TYPE_EXTRASTATUS_2);
		Game_SetFpsLimit(Game_FpsLimit);
		Chat_AddRaw(LOWPERF_EXIT_MESSAGE);
	}

#ifdef CC_BUILD_WEB
	extern void emscripten_resume_main_loop(void);
	emscripten_resume_main_loop();
#endif
}

static void Game_WarnFunc(const cc_string* msg) {
	cc_string str = *msg, line;
	while (str.length) {
		String_UNSAFE_SplitBy(&str, '\n', &line);
		Chat_Add1("&c%s", &line);
	}
}

static void LoadOptions(void) {
	Game_ClassicMode       = Options_GetBool(OPT_CLASSIC_MODE, false);
	Game_ClassicHacks      = Options_GetBool(OPT_CLASSIC_HACKS, false);
	Game_AllowCustomBlocks = Options_GetBool(OPT_CUSTOM_BLOCKS, true);
	Game_UseCPE            = !Game_ClassicMode && Options_GetBool(OPT_CPE, true);
	Game_SimpleArmsAnim    = Options_GetBool(OPT_SIMPLE_ARMS_ANIM, false);
	Game_ViewBobbing       = Options_GetBool(OPT_VIEW_BOBBING, true);

	Game_ViewDistance     = Options_GetInt(OPT_VIEW_DISTANCE, 8, 4096, 512);
	Game_UserViewDistance = Game_ViewDistance;
	Game_BreakableLiquids = !Game_ClassicMode && Options_GetBool(OPT_MODIFIABLE_LIQUIDS, false);
	Game_AllowServerTextures = Options_GetBool(OPT_SERVER_TEXTURES, true);
	/* TODO: Do we need to support option to skip SSL */
	/*cc_bool skipSsl = Options_GetBool("skip-ssl-check", false);
	if (skipSsl) {
		ServicePointManager.ServerCertificateValidationCallback = delegate { return true; };
		Options.Set("skip-ssl-check", false);
	}*/
}

#ifdef CC_BUILD_MINFILES
static void LoadPlugins(void) { }
#else
static void LoadPlugin(const cc_string* path, void* obj) {
	void* lib;
	void* verSym;  /* EXPORT int Plugin_ApiVersion = GAME_API_VER; */
	void* compSym; /* EXPORT struct IGameComponent Plugin_Component = { (whatever) } */
	int ver;

	/* ignore accepted.txt, deskop.ini, .pdb files, etc */
	if (!String_CaselessEnds(path, &DynamicLib_Ext)) return;
	/* don't try to load 32 bit plugins on 64 bit OS or vice versa */
	if (sizeof(void*) == 4 && String_ContainsConst(path, "_64.")) return;
	if (sizeof(void*) == 8 && String_ContainsConst(path, "_32.")) return;

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
	static const cc_string dir = String_FromConst("plugins");
	cc_result res;

	Utils_EnsureDirectory("plugins");
	res = Directory_Enum(&dir, NULL, LoadPlugin);
	if (res) Logger_SysWarn(res, "enumerating plugins directory");
}
#endif

void Game_Free(void* obj);
static void Game_Load(void) {
	struct IGameComponent* comp;
	Game_UpdateDimensions();
	Game_SetFpsLimit(Options_GetEnum(OPT_FPS_LIMIT, 0, FpsLimit_Names, FPS_LIMIT_COUNT));
	Gfx_Create();
	Logger_WarnFunc = Game_WarnFunc;
	LoadOptions();
	GameVersion_Load();
	Utils_EnsureDirectory("maps");

	Event_Register_(&WorldEvents.NewMap,           NULL, HandleOnNewMap);
	Event_Register_(&WorldEvents.MapLoaded,        NULL, HandleOnNewMapLoaded);
	Event_Register_(&WindowEvents.Resized,         NULL, Game_OnResize);
	Event_Register_(&WindowEvents.Closing,         NULL, Game_Free);
	Event_Register_(&WindowEvents.InactiveChanged, NULL, HandleInactiveChanged);

	Game_AddComponent(&World_Component);
	Game_AddComponent(&Textures_Component);
	Game_AddComponent(&Input_Component);
	Game_AddComponent(&Camera_Component);
	Game_AddComponent(&Gfx_Component);
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
	Game_AddComponent(&Builder_Component);
	Game_AddComponent(&MapRenderer_Component);
	Game_AddComponent(&EnvRenderer_Component);
	Game_AddComponent(&Server_Component);
	Game_AddComponent(&Protocol_Component);

	Game_AddComponent(&Gui_Component);
	Game_AddComponent(&Selections_Component);
	Game_AddComponent(&HeldBlockRenderer_Component);
	/* Gfx_SetDepthWrite(true) */
	Game_AddComponent(&PickedPosRenderer_Component);
	Game_AddComponent(&Audio_Component);
	Game_AddComponent(&AxisLinesRenderer_Component);

	LoadPlugins();
	for (comp = comps_head; comp; comp = comp->next) {
		if (comp->Init) comp->Init();
	}

	Game_DefaultZipMissing = false;
	TexturePack_ExtractCurrent(true);
	if (Game_DefaultZipMissing) {
		Window_ShowDialog("Missing file",
			"default.zip is missing, try downloading resources first.\n\nThe game will still run, but without any textures");
	}

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

static void UpdateViewMatrix(void) {
	Camera.Active->GetView(&Gfx.View);
	FrustumCulling_CalcFrustumEquations(&Gfx.Projection, &Gfx.View);
}

static void Game_Render3D(double delta, float t) {
	Vec3 pos;
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
	InputHandler_Tick();
	if (!Game_HideGui) HeldBlockRenderer_Render(delta);
}

static void PerformScheduledTasks(double time) {
	struct ScheduledTask* task;
	int i;

	for (i = 0; i < tasksCount; i++) {
		task = &tasks[i];
		task->accumulator += time;

		while (task->accumulator >= task->interval) {
			task->Callback(task);
			task->accumulator -= task->interval;
		}
	}
}

void Game_TakeScreenshot(void) {
	cc_string filename; char fileBuffer[STRING_SIZE];
	cc_string path;     char pathBuffer[FILENAME_SIZE];
	struct DateTime now;
	cc_result res;
#ifdef CC_BUILD_WEB
	char str[NATIVE_STR_LEN];
#else
	struct Stream stream;
#endif
	Game_ScreenshotRequested = false;
	DateTime_CurrentLocal(&now);

	String_InitArray(filename, fileBuffer);
	String_Format3(&filename, "screenshot_%p4-%p2-%p2", &now.year, &now.month, &now.day);
	String_Format3(&filename, "-%p2-%p2-%p2.png", &now.hour, &now.minute, &now.second);

#ifdef CC_BUILD_WEB
	extern void interop_TakeScreenshot(const char* path);
	String_EncodeUtf8(str, &filename);
	interop_TakeScreenshot(str);
#else
	if (!Utils_EnsureDirectory("screenshots")) return;
	String_InitArray(path, pathBuffer);
	String_Format1(&path, "screenshots/%s", &filename);

	res = Stream_CreateFile(&stream, &path);
	if (res) { Logger_SysWarn2(res, "creating", &path); return; }

	res = Gfx_TakeScreenshot(&stream);
	if (res) { 
		Logger_SysWarn2(res, "saving to", &path); stream.Close(&stream); return;
	}

	res = stream.Close(&stream);
	if (res) { Logger_SysWarn2(res, "closing", &path); return; }
	Chat_Add1("&eTaken screenshot as: %s", &filename);

#ifdef CC_BUILD_MOBILE
	Platform_ShareScreenshot(&filename);
#endif
#endif
}

static void Game_RenderFrame(double delta) {
	struct ScheduledTask entTask;
	float t;

	/* TODO: Should other tasks get called back too? */
	/* Might not be such a good idea for the http_clearcache, */
	/* don't really want all skins getting lost */
	if (Gfx.LostContext) {
		if (Gfx_TryRestoreContext()) {
			Gfx_RecreateContext();
			/* all good, context is back */
		} else {
			Server.Tick(NULL);
			Thread_Sleep(16);
			return;
		}
	}

	Gfx_BeginFrame();
	Gfx_BindIb(Gfx_defaultIb);
	Game.Time += delta;
	Game_Vertices = 0;

	Camera.Active->UpdateMouse(delta);
	if (!WindowInfo.Focused && !Gui.InputGrab) Gui_ShowPauseMenu();

	if (KeyBind_IsPressed(KEYBIND_ZOOM_SCROLL) && !Gui.InputGrab) {
		InputHandler_SetFOV(Camera.ZoomFov);
	}

	PerformScheduledTasks(delta);
	entTask = tasks[entTaskI];
	t = (float)(entTask.accumulator / entTask.interval);
	LocalPlayer_SetInterpPosition(t);

	Camera.CurrentPos = Camera.Active->GetPosition(t);
	/* NOTE: EnvRenderer_UpdateFog also also sets clear color */
	EnvRenderer_UpdateFog();
	UpdateViewMatrix();

	/* TODO: Not calling Gfx_EndFrame doesn't work with Direct3D9 */
	if (WindowInfo.Inactive) return;
	Gfx_Clear();

	Gfx_LoadMatrix(MATRIX_PROJECTION, &Gfx.Projection);
	Gfx_LoadMatrix(MATRIX_VIEW,       &Gfx.View);

	if (!Gui_GetBlocksWorld()) {
		Game_Render3D(delta, t);
	} else {
		RayTracer_SetInvalid(&Game_SelectedPos);
	}

	Gfx_Begin2D(Game.Width, Game.Height);
	Gui_RenderGui(delta);
	Gfx_End2D();

	if (Game_ScreenshotRequested) Game_TakeScreenshot();
	Gfx_EndFrame();
}

void Game_Free(void* obj) {
	struct IGameComponent* comp;
	/* Most components will call OnContextLost in their Free functions */
	/* Set to false so components will always free managed textures too */
	Gfx.ManagedTextures = false;
	Event_UnregisterAll();
	tasksCount = 0;

	for (comp = comps_head; comp; comp = comp->next) {
		if (comp->Free) comp->Free();
	}

	Logger_WarnFunc = Logger_DialogWarn;
	Gfx_Free();
	Options_SaveIfChanged();
	Window_DisableRawMouse();
}

#define Game_DoFrameBody() \
	render = Stopwatch_Measure();\
	Window_ProcessEvents();\
	if (!WindowInfo.Exists) return;\
	\
	delta  = Stopwatch_ElapsedMicroseconds(Game_FrameStart, render) / (1000.0 * 1000.0);\
	\
	if (delta > 1.0) delta = 1.0; /* avoid large delta with suspended process */ \
	if (delta > 0.0) { Game_FrameStart = render; Game_RenderFrame(delta); }

#ifdef CC_BUILD_WEB
void Game_DoFrame(void) {
	cc_uint64 render; 
	double delta;
	Game_DoFrameBody()
}

static void Game_RunLoop(void) {
	Game_FrameStart = Stopwatch_Measure();
	/* Window_Web.c sets Game_DoFrame as the main loop callback function */
	/* (i.e. web browser is in charge of calling Game_DoFrame, not us) */
}

cc_bool Game_ShouldClose(void) {
	if (Server.IsSinglePlayer) {
		/* Close if map was saved within last 5 seconds */
		return World.LastSave + 5 >= Game.Time;
	}

	/* Try to intercept Ctrl+W or Cmd+W for multiplayer */
	if (Key_IsCtrlPressed() || Key_IsWinPressed()) return false;
	/* Also try to intercept mouse back button (Mouse4) */
	return !Input_Pressed[KEY_XBUTTON1];
}
#else
static void Game_RunLoop(void) {
	cc_uint64 render;
	double delta;

	Game_FrameStart = Stopwatch_Measure();
	for (;;) { Game_DoFrameBody() }
}
#endif

void Game_Run(int width, int height, const cc_string* title) {
	Window_Create3D(width, height);
	Window_SetTitle(title);
	Window_Show();

	Game_Load();
	Event_RaiseVoid(&WindowEvents.Resized);
	Game_RunLoop();
}
