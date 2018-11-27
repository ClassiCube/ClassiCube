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
#include "Drawer2D.h"
#include "Model.h"
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
#include "Menus.h"
#include "Audio.h"
#include "Stream.h"
#include "TerrainAtlas.h"

int Game_Width, Game_Height;
double Game_Accumulator;
int Game_ChunkUpdates, Game_Port;
bool Game_CameraClipping, Game_UseCPEBlocks;

struct PickedPos Game_SelectedPos, Game_CameraClipPos;
int Game_ViewDistance, Game_MaxViewDistance, Game_UserViewDistance;
int Game_Fov, Game_DefaultFov, Game_ZoomFov;

float game_limitMs;
int  Game_FpsLimit, Game_Vertices;
bool Game_ShowAxisLines, Game_SimpleArmsAnim;
bool Game_ClassicArmModel, Game_InvertMouse;

int  Game_MouseSensitivity, Game_ChatLines;
bool Game_TabAutocomplete, Game_UseClassicGui;
bool Game_UseClassicTabList, Game_UseClassicOptions;
bool Game_ClassicMode, Game_ClassicHacks;
bool Game_AllowCustomBlocks, Game_UseCPE;
bool Game_AllowServerTextures, Game_SmoothLighting;
bool Game_ChatLogging, Game_AutoRotate;
bool Game_SmoothCamera, Game_ClickableChat;
bool Game_HideGui, Game_ShowFPS;

bool Game_ViewBobbing, Game_ShowBlockInHand;
int  Game_SoundsVolume, Game_MusicVolume;
bool Game_BreakableLiquids, Game_ScreenshotRequested;
int  Game_MaxChunkUpdates;
float Game_RawHotbarScale, Game_RawChatScale, Game_RawInventoryScale;

static struct ScheduledTask Game_Tasks[6];
static int Game_TasksCount, entTaskI;

static char Game_UsernameBuffer[FILENAME_SIZE];
static char Game_MppassBuffer[STRING_SIZE];
static char Game_IPAddressBuffer[STRING_SIZE];
static char Game_FontNameBuffer[STRING_SIZE];

String Game_Username  = String_FromArray(Game_UsernameBuffer);
String Game_Mppass    = String_FromArray(Game_MppassBuffer);
String Game_IPAddress = String_FromArray(Game_IPAddressBuffer);
String Game_FontName  = String_FromArray(Game_FontNameBuffer);

static struct IGameComponent* comps_head;
static struct IGameComponent* comps_tail;
void Game_AddComponent(struct IGameComponent* comp) {
	if (!comps_head) {
		comps_head = comp;
	} else {
		comps_tail->Next = comp;
	}
	comps_tail = comp;
	comp->Next = NULL;
}

int ScheduledTask_Add(double interval, ScheduledTaskCallback callback) {
	struct ScheduledTask task;
	task.Accumulator = 0.0;
	task.Interval    = interval;
	task.Callback    = callback;

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

static char game_defTexPackBuffer[STRING_SIZE];
static String game_defTexPack = String_FromArray(game_defTexPackBuffer);

void Game_GetDefaultTexturePack(String* texPack) {
	String texPath; char texPathBuffer[STRING_SIZE];

	String_InitArray(texPath, texPathBuffer);
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

	Gfx_LoadMatrix(MATRIX_PROJECTION, &Gfx_Projection);
	Event_RaiseVoid(&GfxEvents_ProjectionChanged);
}

void Game_Disconnect(const String* title, const String* reason) {
	World_Reset();
	Event_RaiseVoid(&WorldEvents_NewMap);
	Gui_FreeActive();
	Gui_SetActive(DisconnectScreen_MakeInstance(title, reason));
	Game_Reset();
}

void Game_Reset(void) {
	struct IGameComponent* comp;
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
	chunk->AllAir &= Block_Draw[block] == DRAW_GAS;
	MapRenderer_RefreshChunk(cx, cy, cz);
}

void Game_ChangeBlock(int x, int y, int z, BlockID block) {
	BlockID old = World_GetBlock(x, y, z);
	Game_UpdateBlock(x, y, z, block);
	ServerConnection.SendBlock(x, y, z, old, block);
}

bool Game_CanPick(BlockID block) {
	if (Block_Draw[block] == DRAW_GAS)    return false;
	if (Block_Draw[block] == DRAW_SPRITE) return true;

	if (Block_Collide[block] != COLLIDE_LIQUID) return true;
	return Game_BreakableLiquids && Block_CanPlace[block] && Block_CanDelete[block];
}

bool Game_UpdateTexture(GfxResourceID* texId, struct Stream* src, const String* file, uint8_t* skinType) {
	Bitmap bmp;
	bool success;
	ReturnCode res;
	
	res = Png_Decode(&bmp, src);
	if (res) { Chat_LogError2(res, "decoding", file); }

	success = !res && Game_ValidateBitmap(file, &bmp);
	if (success) {
		Gfx_DeleteTexture(texId);
		if (skinType) { *skinType = Utils_GetSkinType(&bmp); }
		*texId = Gfx_CreateTexture(&bmp, true, false);
	}

	Mem_Free(bmp.Scan0);
	return success;
}

bool Game_ValidateBitmap(const String* file, Bitmap* bmp) {
	int maxWidth = Gfx_MaxTexWidth, maxHeight = Gfx_MaxTexHeight;
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
			Chat_LogError2(res, "decoding", name);
			Mem_Free(bmp.Scan0);
		} else if (!Game_ChangeTerrainAtlas(&bmp)) {
			Mem_Free(bmp.Scan0);
		}		
	} else if (String_CaselessEqualsConst(name, "default.png")) {
		res = Png_Decode(&bmp, src);

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

	MapRenderer_Refresh();
	Game_SetViewDistance(Game_UserViewDistance);
	Chat_AddRaw("&cOut of VRAM! Halving view distance..");
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
	Game_ChatLogging       = Options_GetBool(OPT_CHAT_LOGGING, true);
	Game_ClassicArmModel   = Options_GetBool(OPT_CLASSIC_ARM_MODEL, Game_ClassicMode);

	Game_ViewBobbing    = Options_GetBool(OPT_VIEW_BOBBING, true);
	Game_SmoothLighting = Options_GetBool(OPT_SMOOTH_LIGHTING, false);

	method = Options_GetEnum(OPT_FPS_LIMIT, 0, FpsLimit_Names, FPS_LIMIT_COUNT);
	Game_SetFpsLimit(method);
	Game_ViewDistance     = Options_GetInt(OPT_VIEW_DISTANCE, 16, 4096, 512);
	Game_UserViewDistance = Game_ViewDistance;

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

void Game_Free(void* obj);
void Game_Load(void) {
	String title;      char titleBuffer[STRING_SIZE];
	struct IGameComponent* comp;

	Game_ViewDistance     = 512;
	Game_MaxViewDistance  = 32768;
	Game_UserViewDistance = 512;
	Game_Fov = 70;
	Game_AutoRotate = true;

	Gfx_Init();
	Gfx_SetVSync(true);
	Gfx_MakeApiInfo();
	Gfx_Mipmaps = Options_GetBool(OPT_MIPMAPS, false);

	Game_UpdateClientSize();
	Game_LoadOptions();
	Game_LoadGuiOptions();

	Event_RegisterVoid(&WorldEvents_NewMap,         NULL, Game_OnNewMapCore);
	Event_RegisterVoid(&WorldEvents_MapLoaded,      NULL, Game_OnNewMapLoadedCore);
	Event_RegisterEntry(&TextureEvents_FileChanged, NULL, Game_TextureChangedCore);
	Event_RegisterVoid(&GfxEvents_LowVRAMDetected,  NULL, Game_OnLowVRAMDetected);

	Event_RegisterVoid(&WindowEvents_Resized,       NULL, Game_OnResize);
	Event_RegisterVoid(&WindowEvents_Closed,        NULL, Game_Free);

	TextureCache_Init();
	/* TODO: Survival vs Creative game mode */

	InputHandler_Init();
	Game_AddComponent(&Blocks_Component);
	Game_AddComponent(&Drawer2D_Component);

	Game_AddComponent(&Particles_Component);
	Game_AddComponent(&TabList_Component);
	Game_AddComponent(&Chat_Component);

	Game_AddComponent(&Models_Component);
	Game_AddComponent(&Entities_Component);
	Game_AddComponent(&AsyncDownloader_Component);
	Game_AddComponent(&Lighting_Component);

	Game_AddComponent(&Animations_Component);
	Game_AddComponent(&Inventory_Component);
	Env_Reset();

	Game_AddComponent(&MapRenderer_Component);
	Game_AddComponent(&EnvRenderer_Component);
	Game_AddComponent(&ServerConnection_Component);
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

	/* TODO: plugin dll support */
	/* List<string> nonLoaded = PluginLoader.LoadAll(); */

	for (comp = comps_head; comp; comp = comp->Next) {
		if (comp->Init) comp->Init();
	}
	Game_ExtractInitialTexturePack();

	for (comp = comps_head; comp; comp = comp->Next) {
		if (comp->Ready) comp->Ready();
	}

	entTaskI = ScheduledTask_Add(GAME_DEF_TICKS, Entities_Tick);
	/* TODO: plugin dll support */
	/* if (nonLoaded != null) {
		for (int i = 0; i < nonLoaded.Count; i++) {
			Overlay warning = new PluginOverlay(this, nonLoaded[i]);
			Gui_ShowOverlay(warning, false);
		}
	}*/

	if (Gfx_WarnIfNecessary()) EnvRenderer_UseLegacyMode(true);
	String_InitArray(title, titleBuffer);
	String_Format2(&title, "Connecting to %s:%i..", &Game_IPAddress, &Game_Port);

	Gui_FreeActive();
	Gui_SetActive(LoadingScreen_MakeInstance(&title, &String_Empty));
	ServerConnection.BeginConnect();
}

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

static void Game_LimitFPS(uint64_t frameStart) {
	uint64_t frameEnd = Stopwatch_Measure();
	float elapsedMs = Stopwatch_ElapsedMicroseconds(frameStart, frameEnd) / 1000.0f;
	float leftOver  = game_limitMs - elapsedMs;

	/* going faster than limit */
	if (leftOver > 0.001f) {
		Thread_Sleep((int)(leftOver + 0.5f));
	}
}

static void Game_UpdateViewMatrix(void) {
	Camera_Active->GetView(&Gfx_View);
	Gfx_LoadMatrix(MATRIX_VIEW, &Gfx_View);
	FrustumCulling_CalcFrustumEquations(&Gfx_Projection, &Gfx_View);
}

static void Game_Render3D(double delta, float t) {
	Vector3 pos;
	bool left, middle, right;

	if (EnvRenderer_ShouldRenderSkybox()) EnvRenderer_RenderSkybox(delta);
	AxisLinesRenderer_Render(delta);
	Entities_RenderModels(delta, t);
	Entities_RenderNames(delta);

	Particles_Render(delta, t);
	Camera_Active->GetPickedBlock(&Game_SelectedPos); /* TODO: only pick when necessary */

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
	if (Camera_CurrentPos.Y < Env_EdgeHeight && (pos.X < 0 || pos.Z < 0 || pos.X > World_Width || pos.Z > World_Length)) {
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
	String_Format2(&path, "screenshots%r%s", &Directory_Separator, &filename);

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
	struct ScheduledTask entTask;
	uint64_t frameStart;
	bool allowZoom, visible;
	float t;

	frameStart = Stopwatch_Measure();
	Gfx_BeginFrame();
	Gfx_BindIb(Gfx_defaultIb);
	Game_Accumulator += delta;
	Game_Vertices = 0;

	Camera_Active->UpdateMouse();
	if (!Window_Focused && !Gui_GetActiveScreen()->HandlesAllInput) {
		Gui_FreeActive();
		Gui_SetActive(PauseScreen_MakeInstance());
	}

	allowZoom = !Gui_Active && !Gui_HUD->HandlesAllInput;
	if (allowZoom && KeyBind_IsPressed(KEYBIND_ZOOM_SCROLL)) {
		InputHandler_SetFOV(Game_ZoomFov, false);
	}

	Game_DoScheduledTasks(delta);
	entTask = Game_Tasks[entTaskI];
	t = (float)(entTask.Accumulator / entTask.Interval);
	LocalPlayer_SetInterpPosition(t);

	Gfx_Clear();
	Camera_CurrentPos = Camera_Active->GetPosition(t);
	Game_UpdateViewMatrix();

	visible = !Gui_Active || !Gui_Active->BlocksWorld;
	if (visible && World_Blocks) {
		Game_Render3D(delta, t);
	} else {
		PickedPos_SetAsInvalid(&Game_SelectedPos);
	}

	Gui_RenderGui(delta);
	if (Game_ScreenshotRequested) Game_TakeScreenshot();

	Gfx_EndFrame();
	if (game_limitMs) Game_LimitFPS(frameStart);
}

void Game_Free(void* obj) {
	struct IGameComponent* comp;
	Atlas2D_Free();
	Atlas1D_Free();

	Event_UnregisterVoid(&WorldEvents_NewMap,         NULL, Game_OnNewMapCore);
	Event_UnregisterVoid(&WorldEvents_MapLoaded,      NULL, Game_OnNewMapLoadedCore);
	Event_UnregisterEntry(&TextureEvents_FileChanged, NULL, Game_TextureChangedCore);
	Event_UnregisterVoid(&GfxEvents_LowVRAMDetected,  NULL, Game_OnLowVRAMDetected);

	Event_UnregisterVoid(&WindowEvents_Resized,       NULL, Game_OnResize);
	Event_UnregisterVoid(&WindowEvents_Closed,        NULL, Game_Free);

	for (comp = comps_head; comp; comp = comp->Next) {
		if (comp->Free) comp->Free();
	}
	Gfx_Free();

	if (!Options_HasAnyChanged()) return;
	Options_Load();
	Options_Save();
}

void Game_Run(int width, int height, const String* title, struct DisplayDevice* device) {
	struct GraphicsMode mode;
	uint64_t lastRender, render;
	double time;
	int x, y;

	x = device->Bounds.X + (device->Bounds.Width  - width)  / 2;
	y = device->Bounds.Y + (device->Bounds.Height - height) / 2;
	GraphicsMode_MakeDefault(&mode);

	Window_Create(x, y, width, height, &mode);
	Window_SetTitle(title);
	Window_SetVisible(true);

	Game_Load();
	Event_RaiseVoid(&WindowEvents_Resized);

	lastRender = Stopwatch_Measure();
	for (;;) {
		Window_ProcessEvents();
		if (!Window_Exists) break;

		/* Limit maximum render to 1 second (for suspended process) */
		render = Stopwatch_Measure();
		time   = Stopwatch_ElapsedMicroseconds(lastRender, render) / (1000.0 * 1000.0);
		if (time > 1.0) time = 1.0;
		if (time <= 0.0) continue;

		lastRender = Stopwatch_Measure();
		Game_RenderFrame(time);
	}
}
