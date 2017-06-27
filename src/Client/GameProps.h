#ifndef CS_GAMEPROPS_H
#define CS_GAMEPROPS_H
#include "Typedefs.h"
#include "Options.h"
#include "String.h"
#include "PickedPos.h"
#include "Vectors.h"

/* Total rendering time(in seconds) elapsed since the client was started. */
Real64 Game_Accumulator;

/* Accumulator for the number of chunk updates performed. Reset every second. */
Int32 Game_ChunkUpdates;

/* Whether the third person camera should have their camera position clipped so as to not intersect blocks.*/
bool Game_CameraClipping;

/* Whether colour clear gfx call should be skipped. */
bool Game_SkipClear;

/* Picked pos instance used for block selection. */
PickedPos Game_SelectedPos;

/* Picked pos instance used for camera clipping. */
PickedPos Game_CameraClipPos;

/* Default index buffer ID. */
Int32 Game_DefaultIb;


/* Whether cobblestone slab to stone brick tiles should be used. */
bool Game_UseCPEBlocks;

/* Account username of the player.*/
String Game_Username;

/* Verification key used for multiplayer, unique for the username and individual server. */
String Game_Mppass;

/* IP address of multiplayer server connected to, null if singleplayer. */
String Game_IPAddress;

/* Port of multiplayer server connected to, 0 if singleplayer. */
Int32 Game_Port;

/* Radius of the sphere the player can see around the position of the current camera. */
Real32 Game_ViewDistance;
/* Maximum view distance player can view up to. */
Real32 Game_MaxViewDistance;
/* 'Real' view distance the player has selected. */
Real32 Game_UserViewDistance;

/* Field of view for the current camera in degrees. */
Int32 Game_Fov;
Int32 Game_DefaultFov, Game_ZoomFov;

/* Strategy used to limit how many frames should be displayed at most each second. */
FpsLimitMethod FpsLimit;

/* Whether lines should be rendered for each axis. */
bool Game_ShowAxisLines;

/* Whether players should animate using simple swinging parallel to their bodies. */
bool Game_SimpleArmsAnim;

/* Whether mouse rotation on the y axis should be inverted. */
bool Game_InvertMouse;

/* Number of vertices used for rendering terrain this frame. */
UInt32 Game_Vertices;

/* Model view matrix. */
Matrix Game_View;
/* Projection matrix. */
Matrix Game_Projection;

/* How sensitive the client is to changes in the player's mouse position. */
Int32 Game_MouseSensitivity;

/* Whether tab names autocomplete. */
bool Game_TabAutocomplete;

bool Game_UseClassicGui;
bool Game_UseClassicTabList;
bool Game_UseClassicOptions;
bool Game_ClassicMode;
bool Game_ClassicHacks;
#define Game_PureClassic (Game_ClassicMode && !Game_ClassicHacks)

/* Whether suport for custom blocks is indicated to the server. */
bool Game_AllowCustomBlocks;
/* Whether CPE is used at all. */
bool Game_UseCPE;
/* Whether all texture packs are ignored from the server or not. */
bool Game_AllowServerTextures;

/* Whether advanced/smooth lighting mesh builder is used. */
bool Game_SmoothLighting;

/* Whether chat logging to disc is enabled. */
bool Game_ChatLogging;

/* Whether auto rotation of blocks with special suffixes is enabled. */
bool Game_AutoRotate;

/* Whether camera has smooth rotation. */
bool Game_SmoothCamera;

/* Font name used for text rendering. */
String Game_FontName;

/* Max number of chatlines drawn in chat. */
Int32 Game_ChatLines;
/* Whether clicking on chatlines copies into chat input is enabled. */
bool Game_ClickableChat;
/* Whether the GUI is completely hidden.*/
bool Game_HideGui;
/* Whether FPS status in top left is hidden. */
bool Game_ShowFPS;

/* Raw hotbar scale.*/
static Real32 Game_RawHotbarScale;
/* Raw chat scale. */
static Real32 Game_RawChatScale;
/* Raw inventory scale*/
static Real32 Game_RawInventoryScale;

/* Whether view bobbing is enabled. */
bool Game_ViewBobbing;
/* Whether block in hand is shown in bottom right. */
bool Game_ShowBlockInHand;
/* Volume of dig and step sounds. 0 means disabled. */
Int32 Game_SoundsVolume;
/* Volume of background music played. 0 means disabled */
Int32 Game_MusicVolume;
/* Whether water/lava can be placed/deleted like normal blocks. */
bool Game_ModifiableLiquids;

/* Current position of the camera. */
Vector3 Game_CurrentCameraPos;
/* Whether a screenshot is requested. */
bool Game_ScreenshotRequested;

/* Calculates the amount that the hotbar widget should be scaled by when rendered.
Affected by current resolution of the window, as scaling specified by the user */
Real32 Game_GetGuiHotbarScale(void);

/* Calculates the amount that the block inventory menu should be scaled by when rendered.
Affected by current resolution of the window, and scaling specified by the user */
Real32 Game_GetGuiInventoryScale(void);

/* alculates the amount that 2D chat widgets should be scaled by when rendered.
Affected by current resolution of the window, and scaling specified by the user*/
Real32 Game_GetGuiChatScale(void);

/* Gets path of default texture pack path.
NOTE: If texture pack specified by user can't be found, returns 'default.zip' */
String GetDefaultTexturePack(void);

/* Sets the default texture pack path. */
void SetDefaultTexturePack(String value);

static bool Game_CursorVisible;
static bool Game_realCursorVisible;
/* Sets whether the cursor is visible. */
void Game_SetCursorVisible(bool visible);
#endif