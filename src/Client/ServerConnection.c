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
UChar ServerConnection_ServerNameBuffer[String_BufferSize(STRING_SIZE)];
String ServerConnection_ServerName = String_FromEmptyArray(ServerConnection_ServerNameBuffer);
UChar ServerConnection_ServerMOTDBuffer[String_BufferSize(STRING_SIZE)];
String ServerConnection_ServerMOTD = String_FromEmptyArray(ServerConnection_ServerMOTDBuffer);
UChar ServerConnection_AppNameBuffer[String_BufferSize(STRING_SIZE)];
String ServerConnection_AppName = String_FromEmptyArray(ServerConnection_AppNameBuffer);
Int32 ServerConnection_Ticks;

static void ServerConnection_ResetState(void) {
	ServerConnection_Disconnected = false;
	ServerConnection_SupportsExtPlayerList = false;
	ServerConnection_SupportsPlayerClick = false;
	ServerConnection_SupportsPartialMessages = false;
	ServerConnection_SupportsFullCP437 = false;
}

void ServerConnection_RetrieveTexturePack(STRING_PURE String* url) {
	if (!TextureCache_HasAccepted(url) && !TextureCache_HasDenied(url)) {
		struct Screen* warning = TexPackOverlay_MakeInstance(url);
		Gui_ShowOverlay(warning, false);
	} else {
		ServerConnection_DownloadTexturePack(url);
	}
}

void ServerConnection_DownloadTexturePack(STRING_PURE String* url) {
	if (TextureCache_HasDenied(url)) return;
	UInt8 etagBuffer[String_BufferSize(STRING_SIZE)];
	String etag = String_InitAndClearArray(etagBuffer);
	DateTime lastModified = { 0 };

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
	} else {
		Int32 status = item.StatusCode;
		if (status == 0 || status == 304) return;

		UChar msgBuffer[String_BufferSize(STRING_SIZE)];
		String msg = String_InitAndClearArray(msgBuffer);
		String_Format1(&msg, "&c%i error when trying to download texture pack", &status);
		Chat_Add(&msg);
	}
}


/*########################################################################################################################*
*--------------------------------------------------------PingList---------------------------------------------------------*
*#########################################################################################################################*/
struct PingEntry {
	Int64 TimeSent, TimeReceived;
	UInt16 Data;
};
struct PingEntry PingList_Entries[10];

UInt16 PingList_Set(Int32 i, UInt16 prev) {
	DateTime now; DateTime_CurrentUTC(&now);
	PingList_Entries[i].Data = (UInt16)(prev + 1);
	PingList_Entries[i].TimeSent = DateTime_TotalMs(&now);
	PingList_Entries[i].TimeReceived = 0;
	return (UInt16)(prev + 1);
}

UInt16 PingList_NextPingData(void) {
	/* Find free ping slot */
	Int32 i;
	for (i = 0; i < Array_Elems(PingList_Entries); i++) {
		if (PingList_Entries[i].TimeSent != 0) continue;
		UInt16 prev = i > 0 ? PingList_Entries[i - 1].Data : 0;
		return PingList_Set(i, prev);
	}

	/* Remove oldest ping slot */
	for (i = 0; i < Array_Elems(PingList_Entries) - 1; i++) {
		PingList_Entries[i] = PingList_Entries[i + 1];
	}
	Int32 j = Array_Elems(PingList_Entries) - 1;
	return PingList_Set(j, PingList_Entries[j].Data);
}

void PingList_Update(UInt16 data) {
	Int32 i;
	for (i = 0; i < Array_Elems(PingList_Entries); i++) {
		if (PingList_Entries[i].Data != data) continue;
		DateTime now; DateTime_CurrentUTC(&now);
		PingList_Entries[i].TimeReceived = DateTime_TotalMs(&now);
		return;
	}
}

Int32 PingList_AveragePingMs(void) {
	Real64 totalMs = 0.0;
	Int32 measures = 0;
	Int32 i;
	for (i = 0; i < Array_Elems(PingList_Entries); i++) {
		struct PingEntry entry = PingList_Entries[i];
		if (!entry.TimeSent || !entry.TimeReceived) continue;

		/* Half, because received->reply time is actually twice time it takes to send data */
		totalMs += (entry.TimeReceived - entry.TimeSent) * 0.5;
		measures++;
	}
	return measures == 0 ? 0 : (Int32)(totalMs / measures);
}


/*########################################################################################################################*
*-------------------------------------------------Singleplayer connection-------------------------------------------------*
*#########################################################################################################################*/
static void SPConnection_BeginConnect(void) {
	String logName = String_FromConst("Singleplayer");
	Chat_SetLogName(&logName);
	Game_UseCPEBlocks = Game_UseCPE;

	Int32 i, count = Game_UseCPEBlocks ? BLOCK_CPE_COUNT : BLOCK_ORIGINAL_COUNT;
	for (i = 1; i < count; i++) {
		Block_CanPlace[i]  = true;
		Block_CanDelete[i] = true;
	}
	Event_RaiseVoid(&BlockEvents_PermissionsChanged);

	/* For when user drops a map file onto ClassicalSharp.exe */
	String path = Game_Username;
	if (String_IndexOf(&path, Directory_Separator, 0) >= 0 && File_Exists(&path)) {
		LoadLevelScreen_LoadMap(&path);
		Gui_ReplaceActive(NULL);
		return;
	}

	Random rnd; Random_InitFromCurrentTime(&rnd);
	Gen_SetDimensions(128, 64, 128); Gen_Vanilla = true;
	Gen_Seed = Random_Next(&rnd, Int32_MaxValue);
	Gui_ReplaceActive(GeneratingScreen_MakeInstance());
}

UChar SPConnection_LastCol = '\0';
static void SPConnection_AddPortion(STRING_PURE String* text) {
	UChar tmpBuffer[String_BufferSize(STRING_SIZE * 2)];
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
	String_UNSAFE_TrimEnd(&tmp);

	UChar col = Drawer2D_LastCol(&tmp, tmp.length);
	if (col) SPConnection_LastCol = col;
	Chat_Add(&tmp);
}

static void SPConnection_SendChat(STRING_PURE String* text) {
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

static void SPConnection_SendPosition(Vector3 pos, Real32 rotY, Real32 headX) { }
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
Int32 net_ticks;
DateTime net_lastPacket;
UInt8 net_lastOpcode;
Real64 net_discAccumulator;

bool net_connecting;
Int64 net_connectTimeout;
#define NET_TIMEOUT_MS (15 * 1000)

static void MPConnection_BlockChanged(void* obj, Vector3I coords, BlockID oldBlock, BlockID block) {
	Vector3I p = coords;
	if (block == BLOCK_AIR) {
		block = Inventory_SelectedBlock;
		Classic_WriteSetBlock(p.X, p.Y, p.Z, false, block);
	} else {
		Classic_WriteSetBlock(p.X, p.Y, p.Z, true, block);
	}
	Net_SendPacket();
}

static void ServerConnection_Free(void);
static void MPConnection_FinishConnect(void) {
	net_connecting = false;
	Event_RaiseReal(&WorldEvents_Loading, 0.0f);
	net_readCurrent = net_readBuffer;
	ServerConnection_WriteBuffer = net_writeBuffer;

	Handlers_Reset();
	Classic_WriteLogin(&Game_Username, &Game_Mppass);
	Net_SendPacket();
	DateTime_CurrentUTC(&net_lastPacket);
}

static void MPConnection_FailConnect(ReturnCode result) {
	net_connecting = false;
	UChar msgBuffer[String_BufferSize(STRING_SIZE * 2)];
	String msg = String_InitAndClearArray(msgBuffer);

	if (result != 0) {
		String_Format3(&msg, "Error connecting to %s:%i: %i", &Game_IPAddress, &Game_Port, &result);
		ErrorHandler_Log(&msg);
		String_Clear(&msg);
	}

	String_Format2(&msg, "Failed to connect to %s:%i", &Game_IPAddress, &Game_Port);
	String reason = String_FromConst("You failed to connect to the server. It's probably down!");
	Game_Disconnect(&msg, &reason);

	ServerConnection_Free();
}

static void MPConnection_TickConnect(void) {
	bool poll_error = false;
	Socket_Select(net_socket, SOCKET_SELECT_ERROR, &poll_error);
	if (poll_error) {
		ReturnCode err = 0; Socket_GetError(net_socket, &err);
		MPConnection_FailConnect(err);
		return;
	}

	DateTime now; DateTime_CurrentUTC(&now);
	Int64 nowMS = DateTime_TotalMs(&now);

	bool poll_write = false;
	Socket_Select(net_socket, SOCKET_SELECT_WRITE, &poll_write);

	if (poll_write) {
		Socket_SetBlocking(net_socket, true);
		MPConnection_FinishConnect();
	} else if (nowMS > net_connectTimeout) {
		MPConnection_FailConnect(0);
	} else {
		Int64 leftMS = net_connectTimeout - nowMS;
		Event_RaiseReal(&WorldEvents_Loading, (Real32)leftMS / NET_TIMEOUT_MS);
	}
}

static void MPConnection_BeginConnect(void) {
	Socket_Create(&net_socket);
	Event_RegisterBlock(&UserEvents_BlockChanged, NULL, MPConnection_BlockChanged);
	ServerConnection_Disconnected = false;

	Socket_SetBlocking(net_socket, false);
	net_connecting = true;
	DateTime now; DateTime_CurrentUTC(&now);
	net_connectTimeout = DateTime_TotalMs(&now) + NET_TIMEOUT_MS;

	ReturnCode res = Socket_Connect(net_socket, &Game_IPAddress, Game_Port);
	if (res && res != ReturnCode_SocketInProgess && res != ReturnCode_SocketWouldBlock) {
		MPConnection_FailConnect(res);
	}
}

static void MPConnection_SendChat(STRING_PURE String* text) {
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

static void MPConnection_SendPosition(Vector3 pos, Real32 rotY, Real32 headX) {
	Classic_WritePosition(pos, rotY, headX);
	Net_SendPacket();
}

static void MPConnection_SendPlayerClick(MouseButton button, bool buttonDown, EntityID targetId, struct PickedPos* pos) {
	CPE_WritePlayerClick(button, buttonDown, targetId, pos);
	Net_SendPacket();
}

static void MPConnection_CheckDisconnection(Real64 delta) {
	net_discAccumulator += delta;
	if (net_discAccumulator < 1.0) return;
	net_discAccumulator = 0.0;

	UInt32 available = 0; bool poll_success = false;
	ReturnCode availResult  = Socket_Available(net_socket, &available);
	ReturnCode selectResult = Socket_Select(net_socket, SOCKET_SELECT_READ, &poll_success);

	if (net_writeFailed || availResult != 0 || selectResult != 0 || (available == 0 && poll_success)) {
		String title  = String_FromConst("Disconnected!");
		String reason = String_FromConst("You've lost connection to the server");
		Game_Disconnect(&title, &reason);
	}
}

static void MPConnection_Tick(struct ScheduledTask* task) {
	if (ServerConnection_Disconnected) return;
	if (net_connecting) { MPConnection_TickConnect(); return; }

	DateTime now; DateTime_CurrentUTC(&now);
	if (DateTime_MsBetween(&net_lastPacket, &now) >= 30 * 1000) {
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
		UChar msgBuffer[String_BufferSize(STRING_SIZE * 2)];
		String msg = String_InitAndClearArray(msgBuffer);
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
		DateTime_CurrentUTC(&net_lastPacket);

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
	Int32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		Handlers_RemoveEntity((EntityID)i);
	}
}

static void MPConnection_Reset(void) {
	if (ServerConnection_IsSinglePlayer) return;
	Int32 i;
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
