#include "ServerConnection.h"
#include "Physics.h"
#include "Game.h"
#include "Drawer2D.h"
#include "Chat.h"
#include "Block.h"
#include "Random.h"
#include "Event.h"
#include "AsyncDownloader.h"
#include "Player.h"

UInt8 ServerConnection_ServerNameBuffer[String_BufferSize(STRING_SIZE)];
String ServerConnection_ServerName = String_FromEmptyArray(ServerConnection_ServerNameBuffer);
UInt8 ServerConnection_ServerMOTDBuffer[String_BufferSize(STRING_SIZE)];
String ServerConnection_ServerMOTD = String_FromEmptyArray(ServerConnection_ServerMOTDBuffer);
UInt8 ServerConnection_AppNameBuffer[String_BufferSize(STRING_SIZE)];
String ServerConnection_AppName = String_FromEmptyArray(ServerConnection_AppNameBuffer);
Int32 ServerConnection_Ticks;

void ServerConnection_ResetState(void) {
	ServerConnection_Disconnected = false;
	ServerConnection_SupportsExtPlayerList = false;
	ServerConnection_SupportsPlayerClick = false;
	ServerConnection_SupportsPartialMessages = false;
	ServerConnection_SupportsFullCP437 = false;
}


void SPConnection_Connect(STRING_PURE String* ip, Int32 port) {
	String logName = String_FromConst("Singleplayer");
	Chat_SetLogName(&logName);
	Game_UseCPEBlocks = Game_UseCPE;

	Int32 i, count = Game_UseCPEBlocks ? BLOCK_CPE_COUNT : BLOCK_ORIGINAL_COUNT;
	for (i = 1; i < count; i++) {
		Block_CanPlace[i]  = true;
		Block_CanDelete[i] = true;
	}

	Player* player = &LocalPlayer_Instance.Base;
	String skin = String_FromRawArray(player->SkinNameRaw);
	AsyncDownloader_DownloadSkin(&skin, &skin);
	Event_RaiseVoid(&BlockEvents_PermissionsChanged);
	
	Random rnd; Random_InitFromCurrentTime(&rnd);
	Int32 seed = Random_Next(&rnd, Int32_MaxValue);
	ServerConnection_BeginGeneration(128, 64, 128, seed, true);
}

UInt8 SPConnection_LastCol = NULL;
void SPConnection_AddPortion(STRING_PURE String* text) {
	UInt8 tmpBuffer[String_BufferSize(STRING_SIZE * 2)];
	String tmp = String_InitAndClearArray(tmpBuffer);
	/* Prepend colour codes for subsequent lines of multi-line chat */
	if (!Drawer2D_IsWhiteCol(SPConnection_LastCol)) {
		String_Append(&tmp, '&');
		String_Append(&tmp, SPConnection_LastCol);
	}
	String_AppendString(&tmp, text);

	Int32 i;
	/* Replace all % with & */
	for (i = 0; i < tmp.length; i++) {
		if (tmp.buffer[i] == '%') tmp.buffer[i] = '&';
	}
	/* Trim the end of the string of trailing spaces */
	for (i = tmp.length - 1; i >= 0; i--) {
		if (tmp.buffer[i] != ' ') break;
		String_DeleteAt(&tmp, i);
	}

	UInt8 col = Drawer2D_LastCol(text, text->length);
	if (col != NULL) SPConnection_LastCol = col;
	Chat_Add(text);
}

void SPConnection_SendChat(STRING_PURE String* text) {
	if (text->length == 0) return;
	SPConnection_LastCol = NULL;

	String part = *text;
	while (part.length > STRING_SIZE) {
		String portion = String_UNSAFE_Substring(&part, 0, STRING_SIZE);
		SPConnection_AddPortion(&portion);
		part = String_UNSAFE_SubstringAt(&part, STRING_SIZE);
	}
	SPConnection_AddPortion(&part);
}

void SPConnection_SendPosition(Vector3 pos, Real32 rotY, Real32 headX) { }
void SPConnection_SendPlayerClick(MouseButton button, bool isDown, EntityID targetId, PickedPos* pos) { }

void SPConnection_Tick(ScheduledTask* task) {
	if (ServerConnection_Disconnected) return;
	if ((ServerConnection_Ticks % 3) == 0) {
		Physics_Tick();
		ServerConnection_CheckAsyncResources();
	}
	ServerConnection_Ticks++;
}

void SPConnection_Free(void) { Physics_Free(); }

void ServerConnection_InitSingleplayer(void) {
	ServerConnection_ResetState();
	Physics_Init();
	ServerConnection_SupportsFullCP437 = !Game_ClassicMode;
	ServerConnection_SupportsPartialMessages = true;
	ServerConnection_IsSinglePlayer = true;

	ServerConnection_Connect = SPConnection_Connect;
	ServerConnection_SendChat = SPConnection_SendChat;
	ServerConnection_SendPosition = SPConnection_SendPosition;
	ServerConnection_SendPlayerClick = SPConnection_SendPlayerClick;
	ServerConnection_Tick = SPConnection_Tick;
	ServerConnection_Free = SPConnection_Free;
}