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
#include "Commands.h"
#include "Drawer2D.h"
#include "Model.h"
#include "Particle.h"
#include "Http.h"
#include "Inventory.h"
#include "Input.h"
#include "InputHandler.h"
#include "Server.h"
#include "TexturePack.h"
#include "Screens.h"
#include "SelectionBox.h"
#include "AxisLinesRenderer.h"
#include "EnvRenderer.h"
#include "HeldBlockRenderer.h"
#include "SelOutlineRenderer.h"
#include "Menus.h"
#include "Audio.h"
#include "Stream.h"
#include "Builder.h"
#include "Protocol.h"
#include "Picking.h"
#include "Animations.h"
#include "SystemFonts.h"
#include "Formats.h"
#include "EntityRenderers.h"

struct _GameData Game;
static cc_uint64 frameStart;
cc_bool Game_UseCPEBlocks;

struct RayTracer Game_SelectedPos;
int Game_ViewDistance     = DEFAULT_VIEWDIST;
int Game_UserViewDistance = DEFAULT_VIEWDIST;
int Game_MaxViewDistance  = DEFAULT_MAX_VIEWDIST;

int     Game_FpsLimit, Game_Vertices;
cc_bool Game_SimpleArmsAnim;
static cc_bool gameRunning;
static float gfx_minFrameMs;

cc_bool Game_ClassicMode, Game_ClassicHacks;
cc_bool Game_AllowCustomBlocks;
cc_bool Game_AllowServerTextures;
cc_bool Game_Anaglyph3D;

cc_bool Game_ViewBobbing, Game_HideGui;
cc_bool Game_BreakableLiquids, Game_ScreenshotRequested;
struct GameVersion Game_Version;

static char usernameBuffer[STRING_SIZE];
static char mppassBuffer[STRING_SIZE];
cc_string Game_Username  = String_FromArray(usernameBuffer);
cc_string Game_Mppass    = String_FromArray(mppassBuffer);
#ifdef CC_BUILD_SPLITSCREEN
int Game_NumStates = 1;
#endif

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

static const short normalDists[]  = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };
static const short classicDists[] = { 8, 32, 128, 512 };
void Game_CycleViewDistance(void) {
	const short* dists = Gui.ClassicMenu ? classicDists : normalDists;
	int count = Gui.ClassicMenu ? Array_Elems(classicDists) : Array_Elems(normalDists);

	if (Input_IsShiftPressed()) {
		CycleViewDistanceBackwards(dists, count);
	} else {
		CycleViewDistanceForwards(dists,  count);
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

cc_bool Game_UpdateTexture(GfxResourceID* texId, struct Stream* src, const cc_string* file,
							cc_uint8* skinType, int* heightDivisor) {
	struct Bitmap bmp;
	cc_bool success;
	cc_result res;
	
	res = Png_Decode(&bmp, src);
	if (res) { Logger_SysWarn2(res, "decoding", file); }
	
	/* E.g. gui.png only need top half of the texture loaded */
	if (heightDivisor && bmp.height >= *heightDivisor)
		bmp.height /= *heightDivisor;

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
	float texSize, maxSize;

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

	if (Gfx.MaxTexSize && (bmp->width * bmp->height > Gfx.MaxTexSize)) {
		Chat_Add1("&cUnable to use %s from the texture pack.", file);
		texSize = (bmp->width * bmp->height) / (1024.0f * 1024.0f);
		maxSize = Gfx.MaxTexSize             / (1024.0f * 1024.0f);

		Chat_Add2("&c Its size is %f3 MB, your GPU supports %f3 MB at most.",
				&texSize, &maxSize);
		return false;
	}

	return Game_ValidateBitmapPow2(file, bmp);
}

cc_bool Game_ValidateBitmapPow2(const cc_string* file, struct Bitmap* bmp) {
	if (!Math_IsPowOf2(bmp->width) || !Math_IsPowOf2(bmp->height)) {
		Chat_Add1("&cUnable to use %s from the texture pack.", file);

		Chat_Add2("&c Its size is (%i,%i), which is not a power of two size.",
			&bmp->width, &bmp->height);
		return false;
	}
	return true;
}

void Game_UpdateDimensions(void) {
	Game.Width  = max(Window_Main.Width,  1);
	Game.Height = max(Window_Main.Height, 1);
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
	if (Window_Main.Inactive) {
		Chat_AddOf(&Gfx_LowPerfMessage, MSG_TYPE_EXTRASTATUS_2);
		Gfx_SetVSync(false);
		Game_SetMinFrameTime(1000 / 1.0f);
		Gfx.ReducedPerfMode = true;
	} else {
		Chat_AddOf(&String_Empty,       MSG_TYPE_EXTRASTATUS_2);
		Game_SetFpsLimit(Game_FpsLimit);

		Gfx.ReducedPerfMode         = false;
		Gfx.ReducedPerfModeCooldown = 2;
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
	Game_ClassicMode  = Options_GetBool(OPT_CLASSIC_MODE,  false);
	Game_ClassicHacks = Options_GetBool(OPT_CLASSIC_HACKS, false);
	Game_Anaglyph3D   = Options_GetBool(OPT_ANAGLYPH3D,    false);
#if defined CC_BUILD_PS1 || defined CC_BUILD_SATURN
	/* View bobbing requires per-frame matrix multiplications - costly on FPU less systems */
	Game_ViewBobbing  = Options_GetBool(OPT_VIEW_BOBBING,  false);
#else
	Game_ViewBobbing  = Options_GetBool(OPT_VIEW_BOBBING,  true);
#endif
	
	Game_AllowCustomBlocks   = !Game_ClassicMode && Options_GetBool(OPT_CUSTOM_BLOCKS,      true);
	Game_SimpleArmsAnim      = !Game_ClassicMode && Options_GetBool(OPT_SIMPLE_ARMS_ANIM,   false);
	Game_BreakableLiquids    = !Game_ClassicMode && Options_GetBool(OPT_MODIFIABLE_LIQUIDS, false);
	Game_AllowServerTextures = !Game_ClassicMode && Options_GetBool(OPT_SERVER_TEXTURES,    true);

	Game_ViewDistance     = Options_GetInt(OPT_VIEW_DISTANCE, 8, 4096, DEFAULT_VIEWDIST);
	Game_UserViewDistance = Game_ViewDistance;
	/* TODO: Do we need to support option to skip SSL */
	/*cc_bool skipSsl = Options_GetBool("skip-ssl-check", false);
	if (skipSsl) {
		ServicePointManager.ServerCertificateValidationCallback = delegate { return true; };
		Options.Set("skip-ssl-check", false);
	}*/
}

#ifdef CC_BUILD_PLUGINS
static void LoadPlugin(const cc_string* path, void* obj, int isDirectory) {
	void* lib;
	void* verSym;  /* EXPORT int Plugin_ApiVersion = GAME_API_VER; */
	void* compSym; /* EXPORT struct IGameComponent Plugin_Component = { (whatever) } */
	int ver;
	if (isDirectory) return;

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
#else
static void LoadPlugins(void) { }
#endif

static void Game_PendingClose(void* obj) { gameRunning = false; }
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
	Event_Register_(&WindowEvents.Closing,         NULL, Game_PendingClose);
	Event_Register_(&WindowEvents.InactiveChanged, NULL, HandleInactiveChanged);

	Game_AddComponent(&World_Component);
	Game_AddComponent(&Textures_Component);
	Game_AddComponent(&Input_Component);
	Game_AddComponent(&InputHandler_Component);
	Game_AddComponent(&Camera_Component);
	Game_AddComponent(&Gfx_Component);
	Game_AddComponent(&Blocks_Component);
	Game_AddComponent(&Drawer2D_Component);
	Game_AddComponent(&SystemFonts_Component);

	Game_AddComponent(&Chat_Component);
	Game_AddComponent(&Commands_Component);
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
	Game_AddComponent(&SelOutlineRenderer_Component);
	Game_AddComponent(&Audio_Component);
	Game_AddComponent(&AxisLinesRenderer_Component);
	Game_AddComponent(&Formats_Component);
	Game_AddComponent(&EntityRenderers_Component);

	LoadPlugins();
	for (comp = comps_head; comp; comp = comp->next) {
		if (comp->Init) comp->Init();
	}

	TexturePack_ExtractCurrent(true);
	if (TexturePack_DefaultMissing) {
		Window_ShowDialog("Missing file",
			"Both default.zip and classicube.zip are missing,\n try downloading resources first.\n\nClassiCube will still run, but without any textures.");
	}

	entTaskI = ScheduledTask_Add(GAME_DEF_TICKS, Entities_Tick);
	Gfx_WarnIfNecessary();

	if (Gfx.Limitations & GFX_LIMIT_VERTEX_ONLY_FOG)
		EnvRenderer_SetMode(EnvRenderer_Minimal | ENV_LEGACY);
	if (Gfx.BackendType == CC_GFX_BACKEND_SOFTGPU)
		EnvRenderer_SetMode(ENV_MINIMAL);

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
	Gfx_SetVSync(method == FPS_LIMIT_VSYNC);
	Game_SetMinFrameTime(minFrameTime);
}

#ifdef CC_BUILD_WEB
extern void Window_SetMinFrameTime(float timeMS);

void Game_SetMinFrameTime(float frameTimeMS) {
	if (frameTimeMS) Window_SetMinFrameTime(frameTimeMS);
}
#else
void Game_SetMinFrameTime(float frameTimeMS) {
	gfx_minFrameMs = frameTimeMS;
}
#endif

static void Render3DFrame(float delta, float t) {
	struct Matrix mvp;
	Vec3 pos;

	Camera.Active->GetView(&Gfx.View);
	/*Gfx_LoadMatrix(MATRIX_PROJ, &Gfx.Projection);
	Gfx_LoadMatrix(MATRIX_VIEW, &Gfx.View);
	FrustumCulling_CalcFrustumEquations(&Gfx.Projection, &Gfx.View);*/
	Gfx_LoadMVP(&Gfx.View, &Gfx.Projection, &mvp);
	FrustumCulling_CalcFrustumEquations(&mvp);

	if (EnvRenderer_ShouldRenderSkybox()) EnvRenderer_RenderSkybox();
	AxisLinesRenderer_Render();
	Entities_RenderModels(delta, t);
	EntityNames_Render();

	Particles_Render(t);
	EnvRenderer_RenderSky();
	EnvRenderer_RenderClouds();

	MapRenderer_Update(delta);
	MapRenderer_RenderNormal(delta);
	EnvRenderer_RenderMapSides();

	EntityShadows_Render();
	if (Game_SelectedPos.valid && !Game_HideGui) {
		SelOutlineRenderer_Render(&Game_SelectedPos, true);
	}

	/* Render water over translucent blocks when under the water outside the map for proper alpha blending */
	pos = Camera.CurrentPos;
	if (pos.y < Env.EdgeHeight && (pos.x < 0 || pos.z < 0 || pos.x > World.Width || pos.z > World.Length)) {
		MapRenderer_RenderTranslucent(delta);
		EnvRenderer_RenderMapEdges();
	} else {
		EnvRenderer_RenderMapEdges();
		MapRenderer_RenderTranslucent(delta);
	}

	/* Need to render again over top of translucent block, as the selection outline */
	/* is drawn without writing to the depth buffer */
	if (Game_SelectedPos.valid && !Game_HideGui && Blocks.Draw[Game_SelectedPos.block] == DRAW_TRANSLUCENT) {
		SelOutlineRenderer_Render(&Game_SelectedPos, false);
	}

	Selections_Render();
	EntityNames_RenderHovered();
	if (!Game_HideGui) HeldBlockRenderer_Render(delta);
}

static void Render3D_Anaglyph(float delta, float t) {
	struct Matrix proj = Gfx.Projection;
	struct Matrix view = Gfx.View;

	Gfx_Set3DLeft(&proj, &view);
	Render3DFrame(delta, t);

	Gfx_Set3DRight(&proj, &view);
	Render3DFrame(delta, t);

	Gfx_End3D(&proj, &view);
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
	struct cc_datetime now;
	cc_result res;
#ifdef CC_BUILD_WEB
	cc_filepath str;
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
	Platform_EncodePath(&str, &filename);
	interop_TakeScreenshot(&str);
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


#ifdef CC_BUILD_WEB
static void LimitFPS(void) {
	/* Can't use Thread_Sleep on the web. (spinwaits instead of sleeping) */
	/* Instead the web browser manages the frame timing */
}
#else
static float gfx_targetTime, gfx_actualTime;

static CC_INLINE float ElapsedMilliseconds(cc_uint64 beg, cc_uint64 end) {
	cc_uint64 elapsed = Stopwatch_ElapsedMicroseconds(beg, end);
	if (elapsed > 5000000) elapsed = 5000000;
	
	/* Avoid uint64 / float division, as that typically gets implemented */
	/* using a library function rather than a direct CPU instruction */
	return (int)elapsed / 1000.0f;
}

/* Examines difference between expected and actual frame times, */
/*  then sleeps if actual frame time is too fast */
static void LimitFPS(void) {
	cc_uint64 frameEnd, sleepEnd;
	
	frameEnd = Stopwatch_Measure();
	gfx_actualTime += ElapsedMilliseconds(frameStart, frameEnd);
	gfx_targetTime += gfx_minFrameMs;

	/* going faster than FPS limit - sleep to slow down */
	if (gfx_actualTime < gfx_targetTime) {
		float cooldown = gfx_targetTime - gfx_actualTime;
		Thread_Sleep((int)(cooldown + 0.5f));

		/* also accumulate Thread_Sleep duration, as actual sleep */
		/*  duration can significantly deviate from requested time */
		/*  (e.g. requested 4ms, but actually slept for 8ms) */
		sleepEnd = Stopwatch_Measure();
		gfx_actualTime += ElapsedMilliseconds(frameEnd, sleepEnd);
	}

	/* reset accumulated time to avoid excessive FPS drift */
	if (gfx_targetTime >= 1000) { gfx_actualTime = 0; gfx_targetTime = 0; }
}
#endif

static CC_INLINE void Game_DrawFrame(float delta, float t) {
	int i;

	if (!Gui_GetBlocksWorld()) {
		Camera.Active->GetPickedBlock(&Game_SelectedPos); /* TODO: only pick when necessary */
		Camera_KeyLookUpdate(delta);
		InputHandler_Tick();

		if (Game_Anaglyph3D) {
			Render3D_Anaglyph(delta, t);
		} else {
			Render3DFrame(delta, t);
		}
	} else {
		RayTracer_SetInvalid(&Game_SelectedPos);
	}

	Gfx_Begin2D(Game.Width, Game.Height);
	Gui_RenderGui(delta);
	for (i = 0; i < Array_Elems(Game.Draw2DHooks); i++)
	{
		if (Game.Draw2DHooks[i]) Game.Draw2DHooks[i](delta);
	}

/* TODO find a better solution than this */
#ifdef CC_BUILD_3DS
	if (Game_Anaglyph3D) {
		extern void Gfx_SetTopRight(void);
		Gfx_SetTopRight();
		Gui_RenderGui(delta);
	}
	for (i = 0; i < Array_Elems(Game.Draw2DHooks); i++)
	{
		if (Game.Draw2DHooks[i]) Game.Draw2DHooks[i](delta);
	}
#endif
	Gfx_End2D();
}

#ifdef CC_BUILD_SPLITSCREEN
static void DrawSplitscreen(float delta, float t, int i, int x, int y, int w, int h) {
	Gfx_SetViewport(x, y, w, h);
	Gfx_SetScissor( x, y, w, h);
	Game.CurrentState = i;
	
	Entities.CurPlayer = &LocalPlayer_Instances[i];
	LocalPlayer_SetInterpPosition(Entities.CurPlayer, t);
	Camera.CurrentPos = Camera.Active->GetPosition(t);
	
	Game_DrawFrame(delta, t);
}

int Game_MapState(int deviceIndex) {
	if (Game_NumStates >= 4 && deviceIndex == 3) return 3;
	if (Game_NumStates >= 3 && deviceIndex == 2) return 2;
	if (Game_NumStates >= 2 && deviceIndex == 1) return 1;

	return 0;
}
#endif

static CC_INLINE void Game_RenderFrame(void) {
	struct ScheduledTask entTask;
	double deltaD;
	float t, delta;

	cc_uint64 render  = Stopwatch_Measure();
	cc_uint64 elapsed = Stopwatch_ElapsedMicroseconds(frameStart, render);
	/* avoid large delta with suspended process */
	if (elapsed > 5000000) elapsed = 5000000;
	
	deltaD = (int)elapsed / (1000.0 * 1000.0);
	delta  = (float)deltaD;
	Window_ProcessEvents(delta);

	if (delta <= 0.0f) return;
	frameStart = render;

	/* TODO: Should other tasks get called back too? */
	/* Might not be such a good idea for the http_clearcache, */
	/* don't really want all skins getting lost */
	if (Gfx.LostContext) {
		if (Gfx_TryRestoreContext()) {
			Gfx_RecreateContext();
			/* all good, context is back */
		} else {
			Game.Time += delta; /* TODO: Not set in two places? */
			Server.Tick(NULL);
			Thread_Sleep(16);
			return;
		}
	}

	Gfx_BeginFrame();
	Gfx_BindIb(Gfx.DefaultIb);
	Game.Time += deltaD;
	Game_Vertices = 0;
	Gamepad_Tick(delta);

#ifdef CC_BUILD_SPLITSCREEN
	/* TODO: find a better solution */
	for (int i = 0; i < Game_NumStates; i++)
	{
		Game.CurrentState  = i;
		Entities.CurPlayer = &LocalPlayer_Instances[i];
		Camera.Active->UpdateMouse(Entities.CurPlayer, delta);
	}
	Game.CurrentState  = 0;
	Entities.CurPlayer = &LocalPlayer_Instances[0];
#else
	Camera.Active->UpdateMouse(Entities.CurPlayer, delta);
#endif

	if (!Window_Main.Focused && !Gui.InputGrab) Gui_ShowPauseMenu();

	if (Bind_IsTriggered[BIND_ZOOM_SCROLL] && !Gui.InputGrab) {
		InputHandler_SetFOV(Camera.ZoomFov);
	}

	PerformScheduledTasks(deltaD);
	entTask = tasks[entTaskI];
	t = (float)(entTask.accumulator / entTask.interval);
	LocalPlayer_SetInterpPosition(Entities.CurPlayer, t);

	Camera.CurrentPos = Camera.Active->GetPosition(t);
	/* NOTE: EnvRenderer_UpdateFog also also sets clear color */
	EnvRenderer_UpdateFog();
	AudioBackend_Tick();

	/* TODO: Not calling Gfx_EndFrame doesn't work with Direct3D9 */
	if (Window_Main.Inactive) return;
	Gfx_ClearBuffers(GFX_BUFFER_COLOR | GFX_BUFFER_DEPTH);
	
#ifdef CC_BUILD_SPLITSCREEN
	switch (Game_NumStates) {
		case 1:
			Game_DrawFrame(delta, t); break;
		case 2:
			DrawSplitscreen(delta, t, 0,  0,               0, Game.Width, Game.Height / 2);
			DrawSplitscreen(delta, t, 1,  0, Game.Height / 2, Game.Width, Game.Height / 2);
			break;
		case 3:
			DrawSplitscreen(delta, t, 0,              0,               0, Game.Width    , Game.Height / 2);
			DrawSplitscreen(delta, t, 1,              0, Game.Height / 2, Game.Width / 2, Game.Height / 2);
			DrawSplitscreen(delta, t, 2, Game.Width / 2, Game.Height / 2, Game.Width / 2, Game.Height / 2);
			break;
		case 4:
			DrawSplitscreen(delta, t, 0,              0,               0, Game.Width / 2, Game.Height / 2);
			DrawSplitscreen(delta, t, 1, Game.Width / 2,               0, Game.Width / 2, Game.Height / 2);
			DrawSplitscreen(delta, t, 2,              0, Game.Height / 2, Game.Width / 2, Game.Height / 2);
			DrawSplitscreen(delta, t, 3, Game.Width / 2, Game.Height / 2, Game.Width / 2, Game.Height / 2);
			break;
	}
#else
	Game_DrawFrame(delta, t);
#endif

	if (Game_ScreenshotRequested) Game_TakeScreenshot();
	Gfx_EndFrame();
	if (gfx_minFrameMs) LimitFPS();
}


static void Game_Free(void) {
	struct IGameComponent* comp;
	/* Most components will call OnContextLost in their Free functions */
	/* Set to false so components will always free managed textures too */
	Gfx.ManagedTextures = false;
	Event_UnregisterAll();
	tasksCount = 0;

	for (comp = comps_head; comp; comp = comp->next)
	{
		if (comp->Free) comp->Free();
	}

	gameRunning     = false;
	Logger_WarnFunc = Logger_DialogWarn;
	Gfx_Free();
	Options_SaveIfChanged();
	Window_DisableRawMouse();
}

#ifdef CC_BUILD_WEB
void Game_DoFrame(void) {
	if (gameRunning) {
		Game_RenderFrame();
	} else if (tasksCount) {
		Game_Free();
		Window_Free();
	}	
}

static void Game_RunLoop(void) {
	/* Window_Web.c sets Game_DoFrame as the main loop callback function */
	/* (i.e. web browser is in charge of calling Game_DoFrame, not us) */
}

cc_bool Game_ShouldClose(void) {
	if (!gameRunning) return true;

	if (Server.IsSinglePlayer) {
		/* Close if map was saved within last 5 seconds */
		return World.LastSave + 5 >= Game.Time;
	}

	/* Try to intercept Ctrl+W or Cmd+W for multiplayer */
	if (Input_IsCtrlPressed() || Input_IsWinPressed()) return false;
	/* Also try to intercept mouse back button (Mouse4) */
	return !Input.Pressed[CCMOUSE_X1];
}
#else
static void Game_RunLoop(void) {
	while (gameRunning)
	{
		Game_RenderFrame();
	}
	Game_Free();
}
#endif

void Game_Run(int width, int height, const cc_string* title) {
	Window_Create3D(width, height);
	Window_SetTitle(title);
	Window_Show();
	gameRunning = true;
	Game.CurrentState = 0;

	Game_Load();
	Event_RaiseVoid(&WindowEvents.Resized);

	frameStart = Stopwatch_Measure();
	Game_RunLoop();
	Window_Destroy();
}
