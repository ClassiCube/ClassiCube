#include "ServerConnection.h"
#include "BlockPhysics.h"
#include "Game.h"
#include "Drawer2D.h"
#include "Chat.h"
#include "Block.h"
#include "Event.h"
#include "AsyncDownloader.h"
#include "Funcs.h"
#include "Entity.h"
#include "Gui.h"
#include "Screens.h"
#include "MapGenerator.h"
#include "World.h"
#include "Camera.h"
#include "TexturePack.h"
#include "Menus.h"
#include "ErrorHandler.h"
#include "PacketHandlers.h"
#include "Inventory.h"
#include "Platform.h"
#include "GameStructs.h"

/*########################################################################################################################*
*-----------------------------------------------------Common handlers-----------------------------------------------------*
*#########################################################################################################################*/
char ServerConnection_ServerNameBuffer[STRING_SIZE];
String ServerConnection_ServerName = String_FromArray(ServerConnection_ServerNameBuffer);
char ServerConnection_ServerMOTDBuffer[STRING_SIZE];
String ServerConnection_ServerMOTD = String_FromArray(ServerConnection_ServerMOTDBuffer);
char ServerConnection_AppNameBuffer[STRING_SIZE];
String ServerConnection_AppName = String_FromArray(ServerConnection_AppNameBuffer);
int ServerConnection_Ticks;

static void ServerConnection_ResetState(void) {
	ServerConnection_Disconnected = false;
	ServerConnection_SupportsExtPlayerList = false;
	ServerConnection_SupportsPlayerClick = false;
	ServerConnection_SupportsPartialMessages = false;
	ServerConnection_SupportsFullCP437 = false;
}

void ServerConnection_RetrieveTexturePack(const String* url) {
	if (!TextureCache_HasAccepted(url) && !TextureCache_HasDenied(url)) {
		struct Screen* warning = TexPackOverlay_MakeInstance(url);
		Gui_ShowOverlay(warning, false);
	} else {
		ServerConnection_DownloadTexturePack(url);
	}
}

void ServerConnection_DownloadTexturePack(const String* url) {
	if (TextureCache_HasDenied(url)) return;
	char etagBuffer[STRING_SIZE];
	String etag = String_FromArray(etagBuffer);
	TimeMS lastModified = 0;

	if (TextureCache_HasUrl(url)) {
		TextureCache_GetLastModified(url, &lastModified);
		TextureCache_GetETag(url, &etag);
	}

	TexturePack_ExtractCurrent(url);
	String texPack = String_FromConst("texturePack");
	AsyncDownloader_GetDataEx(url, true, &texPack, &lastModified, &etag);
}

void ServerConnection_CheckAsyncResources(void) {
	struct AsyncRequest item;
	String texPack = String_FromConst("texturePack");
	if (!AsyncDownloader_Get(&texPack, &item)) return;

	if (item.ResultData) {
		TexturePack_Extract_Req(&item);
	} else if (item.Result) {
		Chat_Add1("&cError %i when trying to download texture pack", &item.Result);
	} else {
		int status = item.StatusCode;
		if (status == 0 || status == 304) return;
		Chat_Add1("&c%i error when trying to download texture pack", &status);
	}
}


/*########################################################################################################################*
*--------------------------------------------------------PingList---------------------------------------------------------*
*#########################################################################################################################*/
struct PingEntry {
	int64_t TimeSent, TimeReceived; UInt16 Data;
};
struct PingEntry PingList_Entries[10];

UInt16 PingList_Set(int i, UInt16 prev) {
	PingList_Entries[i].Data = (UInt16)(prev + 1);
	PingList_Entries[i].TimeSent = DateTime_CurrentUTC_MS();
	PingList_Entries[i].TimeReceived = 0;
	return (UInt16)(prev + 1);
}

UInt16 PingList_NextPingData(void) {
	/* Find free ping slot */
	int i;
	for (i = 0; i < Array_Elems(PingList_Entries); i++) {
		if (PingList_Entries[i].TimeSent) continue;

		UInt16 prev = i > 0 ? PingList_Entries[i - 1].Data : 0;
		return PingList_Set(i, prev);
	}

	/* Remove oldest ping slot */
	for (i = 0; i < Array_Elems(PingList_Entries) - 1; i++) {
		PingList_Entries[i] = PingList_Entries[i + 1];
	}
	int j = Array_Elems(PingList_Entries) - 1;
	return PingList_Set(j, PingList_Entries[j].Data);
}

void PingList_Update(UInt16 data) {
	int i;
	for (i = 0; i < Array_Elems(PingList_Entries); i++) {
		if (PingList_Entries[i].Data != data) continue;

		PingList_Entries[i].TimeReceived = DateTime_CurrentUTC_MS();
		return;
	}
}

int PingList_AveragePingMs(void) {
	double totalMs = 0.0;
	int measures = 0;
	int i;
	for (i = 0; i < Array_Elems(PingList_Entries); i++) {
		struct PingEntry entry = PingList_Entries[i];
		if (!entry.TimeSent || !entry.TimeReceived) continue;

		/* Half, because received->reply time is actually twice time it takes to send data */
		totalMs += (entry.TimeReceived - entry.TimeSent) * 0.5;
		measures++;
	}
	return measures == 0 ? 0 : (int)(totalMs / measures);
}


/*########################################################################################################################*
*-------------------------------------------------Singleplayer connection-------------------------------------------------*
*#########################################################################################################################*/
static void SPConnection_BeginConnect(void) {
	String logName = String_FromConst("Singleplayer");
	Chat_SetLogName(&logName);
	Game_UseCPEBlocks = Game_UseCPE;

	int i, count = Game_UseCPEBlocks ? BLOCK_CPE_COUNT : BLOCK_ORIGINAL_COUNT;
	for (i = 1; i < count; i++) {
		Block_CanPlace[i]  = true;
		Block_CanDelete[i] = true;
	}
	Event_RaiseVoid(&BlockEvents_PermissionsChanged);

	/* For when user drops a map file onto ClassiCube.exe */
	String path = Game_Username;
	if (String_IndexOf(&path, Directory_Separator, 0) >= 0 && File_Exists(&path)) {
		LoadLevelScreen_LoadMap(&path);
		Gui_CloseActive();
		return;
	}

	Random rnd; Random_InitFromCurrentTime(&rnd);
	Gen_SetDimensions(128, 64, 128); Gen_Vanilla = true;
	Gen_Seed = Random_Next(&rnd, Int32_MaxValue);

	Gui_FreeActive();
	Gui_SetActive(GeneratingScreen_MakeInstance());
}

char SPConnection_LastCol = '\0';
static void SPConnection_AddPortion(const String* text) {
	char tmpBuffer[STRING_SIZE * 2];
	String tmp = String_FromArray(tmpBuffer);
	/* Prepend colour codes for subsequent lines of multi-line chat */
	if (!Drawer2D_IsWhiteCol(SPConnection_LastCol)) {
		String_Append(&tmp, '&');
		String_Append(&tmp, SPConnection_LastCol);
	}
	String_AppendString(&tmp, text);

	int i;
	/* Replace all % with & */
	for (i = 0; i < tmp.length; i++) {
		if (tmp.buffer[i] == '%') tmp.buffer[i] = '&';
	}
	String_TrimEnd(&tmp);

	char col = Drawer2D_LastCol(&tmp, tmp.length);
	if (col) SPConnection_LastCol = col;
	Chat_Add(&tmp);
}

static void SPConnection_SendChat(const String* text) {
	if (!text->length) return;
	SPConnection_LastCol = '\0';

	String part = *text;
	while (part.length > STRING_SIZE) {
		String portion = String_UNSAFE_Substring(&part, 0, STRING_SIZE);
		SPConnection_AddPortion(&portion);
		part = String_UNSAFE_SubstringAt(&part, STRING_SIZE);
	}
	SPConnection_AddPortion(&part);
}

static void SPConnection_SendPosition(Vector3 pos, float rotY, float headX) { }
static void SPConnection_SendPlayerClick(MouseButton button, bool isDown, EntityID targetId, struct PickedPos* pos) { }

static void SPConnection_Tick(struct ScheduledTask* task) {
	if (ServerConnection_Disconnected) return;
	if ((ServerConnection_Ticks % 3) == 0) {
		Physics_Tick();
		ServerConnection_CheckAsyncResources();
	}
	ServerConnection_Ticks++;
}

void ServerConnection_InitSingleplayer(void) {
	ServerConnection_ResetState();
	Physics_Init();
	ServerConnection_SupportsFullCP437 = !Game_ClassicMode;
	ServerConnection_SupportsPartialMessages = true;
	ServerConnection_IsSinglePlayer = true;

	ServerConnection_BeginConnect = SPConnection_BeginConnect;
	ServerConnection_SendChat = SPConnection_SendChat;
	ServerConnection_SendPosition = SPConnection_SendPosition;
	ServerConnection_SendPlayerClick = SPConnection_SendPlayerClick;
	ServerConnection_Tick = SPConnection_Tick;

	ServerConnection_WriteBuffer = NULL;
}


/*########################################################################################################################*
*--------------------------------------------------Multiplayer connection-------------------------------------------------*
*#########################################################################################################################*/
SocketPtr net_socket;
UInt8 net_readBuffer[4096 * 5];
UInt8 net_writeBuffer[131];
UInt8* net_readCurrent;

bool net_writeFailed;
int net_ticks;
TimeMS net_lastPacket;
UInt8 net_lastOpcode;
double net_discAccumulator;

bool net_connecting;
TimeMS net_connectTimeout;
#define NET_TIMEOUT_MS (15 * 1000)

static void MPConnection_BlockChanged(void* obj, Vector3I p, BlockID old, BlockID now) {
	if (now == BLOCK_AIR) {
		now = Inventory_SelectedBlock;
		Classic_WriteSetBlock(p.X, p.Y, p.Z, false, now);
	} else {
		Classic_WriteSetBlock(p.X, p.Y, p.Z, true, now);
	}
	Net_SendPacket();
}

static void ServerConnection_Free(void);
static void MPConnection_FinishConnect(void) {
	net_connecting = false;
	Event_RaiseFloat(&WorldEvents_Loading, 0.0f);
	net_readCurrent = net_readBuffer;
	ServerConnection_WriteBuffer = net_writeBuffer;

	Handlers_Reset();
	Classic_WriteLogin(&Game_Username, &Game_Mppass);
	Net_SendPacket();
	net_lastPacket = DateTime_CurrentUTC_MS();
}

static void MPConnection_FailConnect(ReturnCode result) {
	net_connecting = false;
	char msgBuffer[STRING_SIZE * 2];
	String msg = String_FromArray(msgBuffer);

	if (result) {
		String_Format3(&msg, "Error connecting to %s:%i: %i", &Game_IPAddress, &Game_Port, &result);
		ErrorHandler_Log(&msg);
		msg.length = 0;
	}

	String_Format2(&msg, "Failed to connect to %s:%i", &Game_IPAddress, &Game_Port);
	String reason = String_FromConst("You failed to connect to the server. It's probably down!");
	Game_Disconnect(&msg, &reason);

	ServerConnection_Free();
}

static void MPConnection_TickConnect(void) {
	ReturnCode err = 0; Socket_GetError(net_socket, &err);
	if (err) {
		MPConnection_FailConnect(err); return;
	}

	TimeMS now = DateTime_CurrentUTC_MS();
	bool poll_write = false;
	Socket_Select(net_socket, SOCKET_SELECT_WRITE, &poll_write);

	if (poll_write) {
		Socket_SetBlocking(net_socket, true);
		MPConnection_FinishConnect();
	} else if (now > net_connectTimeout) {
		MPConnection_FailConnect(0);
	} else {
		int leftMS = (int)(net_connectTimeout - now);
		Event_RaiseFloat(&WorldEvents_Loading, (float)leftMS / NET_TIMEOUT_MS);
	}
}

static void MPConnection_BeginConnect(void) {
	Event_RegisterBlock(&UserEvents_BlockChanged, NULL, MPConnection_BlockChanged);
	Socket_Create(&net_socket);
	ServerConnection_Disconnected = false;

	Socket_SetBlocking(net_socket, false);
	net_connecting = true;
	net_connectTimeout = DateTime_CurrentUTC_MS() + NET_TIMEOUT_MS;

	ReturnCode res = Socket_Connect(net_socket, &Game_IPAddress, Game_Port);
	if (res && res != ReturnCode_SocketInProgess && res != ReturnCode_SocketWouldBlock) {
		MPConnection_FailConnect(res);
	}
}

static void MPConnection_SendChat(const String* text) {
	if (!text->length || net_connecting) return;
	String remaining = *text;

	while (remaining.length > STRING_SIZE) {
		String portion = String_UNSAFE_Substring(&remaining, 0, STRING_SIZE);
		Classic_WriteChat(&portion, true);
		Net_SendPacket();
		remaining = String_UNSAFE_SubstringAt(&remaining, STRING_SIZE);
	}

	Classic_WriteChat(&remaining, false);
	Net_SendPacket();
}

static void MPConnection_SendPosition(Vector3 pos, float rotY, float headX) {
	Classic_WritePosition(pos, rotY, headX);
	Net_SendPacket();
}

static void MPConnection_SendPlayerClick(MouseButton button, bool buttonDown, EntityID targetId, struct PickedPos* pos) {
	CPE_WritePlayerClick(button, buttonDown, targetId, pos);
	Net_SendPacket();
}

static void MPConnection_CheckDisconnection(double delta) {
	net_discAccumulator += delta;
	if (net_discAccumulator < 1.0) return;
	net_discAccumulator = 0.0;

	UInt32 available = 0; bool poll_success = false;
	ReturnCode availResult  = Socket_Available(net_socket, &available);
	ReturnCode selectResult = Socket_Select(net_socket, SOCKET_SELECT_READ, &poll_success);

	if (net_writeFailed || availResult || selectResult || (available == 0 && poll_success)) {
		String title  = String_FromConst("Disconnected!");
		String reason = String_FromConst("You've lost connection to the server");
		Game_Disconnect(&title, &reason);
	}
}

static void MPConnection_Tick(struct ScheduledTask* task) {
	if (ServerConnection_Disconnected) return;
	if (net_connecting) { MPConnection_TickConnect(); return; }

	/* over 30 seconds since last packet */
	TimeMS now = DateTime_CurrentUTC_MS();
	if (net_lastPacket + (30 * 1000) < now) {
		MPConnection_CheckDisconnection(task->Interval);
	}
	if (ServerConnection_Disconnected) return;

	UInt32 pending = 0;
	ReturnCode res = Socket_Available(net_socket, &pending);
	UInt8* readEnd = net_readCurrent;

	if (!res && pending) {
		/* NOTE: Always using a read call that is a multiple of 4096 (appears to?) improve read performance */	
		res = Socket_Read(net_socket, net_readCurrent, 4096 * 4, &pending);
		readEnd += pending;
	}

	if (res) {
		char msgBuffer[STRING_SIZE * 2];
		String msg = String_FromArray(msgBuffer);
		String_Format3(&msg, "Error reading from %s:%i: %i", &Game_IPAddress, &Game_Port, &res);
		ErrorHandler_Log(&msg);

		String title  = String_FromConst("&eLost connection to the server");
		String reason = String_FromConst("I/O error when reading packets");
		Game_Disconnect(&title, &reason);
		return;
	}

	net_readCurrent = net_readBuffer;
	while (net_readCurrent < readEnd) {
		UInt8 opcode = net_readCurrent[0];
		/* Workaround for older D3 servers which wrote one byte too many for HackControl packets */
		if (cpe_needD3Fix && net_lastOpcode == OPCODE_HACK_CONTROL && (opcode == 0x00 || opcode == 0xFF)) {
			Platform_LogConst("Skipping invalid HackControl byte from D3 server");
			net_readCurrent++;

			struct LocalPlayer* p = &LocalPlayer_Instance;
			p->Physics.JumpVel = 0.42f; /* assume default jump height */
			p->Physics.ServerJumpVel = p->Physics.JumpVel;
			continue;
		}

		if (opcode >= OPCODE_COUNT) {
			String title = String_FromConst("Disconnected");
			String msg   = String_FromConst("Server sent invalid packet!");
			Game_Disconnect(&title, &msg); return; 
		}

		if (net_readCurrent + Net_PacketSizes[opcode] > readEnd) break;
		net_lastOpcode = opcode;
		net_lastPacket = DateTime_CurrentUTC_MS();

		Net_Handler handler = Net_Handlers[opcode];
		if (!handler) { 
			String title = String_FromConst("Disconnected");
			String msg = String_FromConst("Server sent invalid packet!");
			Game_Disconnect(&title, &msg); return;
		}

		handler(net_readCurrent + 1);  /* skip opcode */
		net_readCurrent += Net_PacketSizes[opcode];
	}

	/* Keep last few unprocessed bytes, don't care about rest since they'll be overwritten on socket read */
	UInt32 i, remaining = (UInt32)(readEnd - net_readCurrent);
	for (i = 0; i < remaining; i++) {
		net_readBuffer[i] = net_readCurrent[i];
	}
	net_readCurrent = net_readBuffer + remaining;

	/* Network is ticked 60 times a second. We only send position updates 20 times a second */
	if ((net_ticks % 3) == 0) {
		ServerConnection_CheckAsyncResources();
		Handlers_Tick();
		/* Have any packets been written? */
		if (ServerConnection_WriteBuffer != net_writeBuffer) {
			Net_SendPacket();
		}
	}
	net_ticks++;
}

void Net_Set(UInt8 opcode, Net_Handler handler, UInt16 packetSize) {
	Net_Handlers[opcode] = handler;
	Net_PacketSizes[opcode] = packetSize;
}

void Net_SendPacket(void) {
	UInt32 count = (UInt32)(ServerConnection_WriteBuffer - net_writeBuffer);
	ServerConnection_WriteBuffer = net_writeBuffer;
	if (ServerConnection_Disconnected) return;

	/* NOTE: Not immediately disconnecting here, as otherwise we sometimes miss out on kick messages */
	UInt32 wrote = 0;
	UInt8* ptr = net_writeBuffer;

	while (count) {
		ReturnCode res = Socket_Write(net_socket, ptr, count, &wrote);
		if (res || !wrote) { net_writeFailed = true; break; }
		ptr += wrote; count -= wrote;
	}
}

void ServerConnection_InitMultiplayer(void) {
	ServerConnection_ResetState();
	ServerConnection_IsSinglePlayer = false;

	ServerConnection_BeginConnect = MPConnection_BeginConnect;
	ServerConnection_SendChat = MPConnection_SendChat;
	ServerConnection_SendPosition = MPConnection_SendPosition;
	ServerConnection_SendPlayerClick = MPConnection_SendPlayerClick;
	ServerConnection_Tick = MPConnection_Tick;

	net_readCurrent = net_readBuffer;
	ServerConnection_WriteBuffer = net_writeBuffer;
}


static void MPConnection_OnNewMap(void) {
	if (ServerConnection_IsSinglePlayer) return;
	/* wipe all existing entity states */
	int i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		Handlers_RemoveEntity((EntityID)i);
	}
}

static void MPConnection_Reset(void) {
	if (ServerConnection_IsSinglePlayer) return;
	int i;
	for (i = 0; i < OPCODE_COUNT; i++) {
		Net_Handlers[i] = NULL;
		Net_PacketSizes[i] = 0;
	}

	net_writeFailed = false;
	Handlers_Reset();
	ServerConnection_Free();
}

static void ServerConnection_Free(void) {
	if (ServerConnection_IsSinglePlayer) {
		Physics_Free();
	} else {
		if (ServerConnection_Disconnected) return;
		Event_UnregisterBlock(&UserEvents_BlockChanged, NULL, MPConnection_BlockChanged);
		Socket_Close(net_socket);
		ServerConnection_Disconnected = true;
	}
}

void ServerConnection_MakeComponent(struct IGameComponent* comp) {
	comp->OnNewMap = MPConnection_OnNewMap;
	comp->Reset    = MPConnection_Reset;
	comp->Free     = ServerConnection_Free;
}
