#ifndef CC_LAUNCHER_H
#define CC_LAUNCHER_H
#include "Bitmap.h"
#include "String.h"

struct LScreen;
TimeMS Launcher_PatchTime;

extern BitmapCol Launcher_BackgroundCol       = BITMAPCOL_CONST(153, 127, 172, 255);
extern BitmapCol Launcher_ButtonBorderCol     = BITMAPCOL_CONST( 97,  81, 110, 255);
extern BitmapCol Launcher_ButtonForeActiveCol = BITMAPCOL_CONST(189, 168, 206, 255);
extern BitmapCol Launcher_ButtonForeCol       = BITMAPCOL_CONST(141, 114, 165, 255);
extern BitmapCol Launcher_ButtonHighlightCol  = BITMAPCOL_CONST(162, 131, 186, 255);

void Launcher_ResetSkin(void);
void Launcher_LoadSkin(void);
void Launcher_SaveSkin(void);

void Launcher_UpdateAsync(void);

extern bool Launcher_ClassicBackground;

void Launcher_TryLoadTexturePack(void);
void Launcher_RedrawBackground(void);

/* Redraws the specified region with the background pixels. */
void Launcher_ResetArea(int x, int y, int width, int height);

extern struct LSCreen* Launcher_Screen;
/* Whether the client drawing area needs to be redrawn/presented to the screen. */
extern bool Launcher_Dirty, Launcher_PendingRedraw;
/* The specific area/region of the window that needs to be redrawn. */
extern Rect2D Launcher_DirtyArea;
String Username;
extern int Launcher_Width, Launcher_Height;
extern Bitmap Launcher_Framebuffer;

/* Whether at the next tick, the launcher window should proceed to stop displaying frames and subsequently exit. */
extern bool Launcher_ShouldExit;
/* Whether update script should be asynchronously run on exit. */
extern bool Launcher_ShouldUpdate;

struct ServerListEntry {
	String Hash, Name, Players, MaxPlayers, Flag;
	String Uptime, IPAddress, Port, Mppass, Software;
	bool Featured;
};
extern struct ServerListEntry* Launcher_Servers;

extern String Launcher_FontName;
void Launcher_SetScreen(struct LScreen* screen);
bool Launcher_ConnectToServer(const String* hash);
void Launcher_Run();

bool Launcher_StartGame(const String* user, const String* mppass, const String* ip, const String* port, const String* server);
#endif
