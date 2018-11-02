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
	struct Screen* warning;
	if (!TextureCache_HasAccepted(url) && !TextureCache_HasDenied(url)) {
		warning = TexPackOverlay_MakeInstance(url);
		Gui_ShowOverlay(warning, false);
	} else {
		ServerConnection_DownloadTexturePack(url);
	}
}

void ServerConnection_DownloadTexturePack(const String* url) {
	static String texPack = String_FromConst("texturePack");
	if (TextureCache_HasDenied(url)) return;

	char etagBuffer[STRING_SIZE];
	String etag = String_FromArray(etagBuffer);
	TimeMS lastModified = 0;

	if (TextureCache_Has(url)) {
		TextureCache_GetLastModified(url, &lastModified);
		TextureCache_GetETag(url, &etag);
	}

	TexturePack_ExtractCurrent(url);
	AsyncDownloader_GetDataEx(url, true, &texPack, &lastModified, &etag);
}

void ServerConnection_CheckAsyncResources(void) {
	static String texPack = String_FromConst("texturePack");
	struct AsyncRequest item;
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
struct PingEntry { int64_t Sent, Recv; uint16_t Data; };
struct PingEntry pingList_entries[10];
int pingList_head;

int PingList_NextPingData(void) {
	int head = pingList_head;
	int next = pingList_entries[head].Data + 1;

	head = (head + 1) % Array_Elems(pingList_entries);
	pingList_entries[head].Data = next;
	pingList_entries[head].Sent = DateTime_CurrentUTC_MS();
	pingList_entries[head].Recv = 0;
	
	pingList_head = head;
	return next;
}

void PingList_Update(int data) {
	int i;
	for (i = 0; i < Array_Elems(pingList_entries); i++) {
		if (pingList_entries[i].Data != data) continue;

		pingList_entries[i].Recv = DateTime_CurrentUTC_MS();
		return;
	}
}

int PingList_AveragePingMs(void) {
	double totalMs = 0.0;
	int i, measures = 0;

	for (i = 0; i < Array_Elems(pingList_entries); i++) {
		struct PingEntry entry = pingList_entries[i];
		if (!entry.Sent || !entry.Recv) continue;

		/* Half, because received->reply time is actually twice time it takes to send data */
		totalMs += (entry.Recv - entry.Sent) * 0.5;
		measures++;
	}
	return measures == 0 ? 0 : (int)(totalMs / measures);
}


/*########################################################################################################################*
*-------------------------------------------------Singleplayer connection-------------------------------------------------*
*#########################################################################################################################*/
static void SPConnection_BeginConnect(void) {
	static String logName = String_FromConst("Singleplayer");
	String path;
	RNGState rnd;
	int i, count;

	Chat_SetLogName(&logName);
	Game_UseCPEBlocks = Game_UseCPE;
	count = Game_UseCPEBlocks ? BLOCK_CPE_COUNT : BLOCK_ORIGINAL_COUNT;

	for (i = 1; i < count; i++) {
		Block_CanPlace[i]  = true;
		Block_CanDelete[i] = true;
	}
	Event_RaiseVoid(&BlockEvents_PermissionsChanged);

	/* For when user drops a map file onto ClassiCube.exe */
	path = Game_Username;
	if (String_IndexOf(&path, Directory_Separator, 0) >= 0 && File_Exists(&path)) {
		LoadLevelScreen_LoadMap(&path);
		Gui_CloseActive();
		return;
	}

	Random_InitFromCurrentTime(&rnd);
	Gen_SetDimensions(128, 64, 128); Gen_Vanilla = true;
	Gen_Seed = Random_Next(&rnd, Int32_MaxValue);

	Gui_FreeActive();
	Gui_SetActive(GeneratingScreen_MakeInstance());
}

char SPConnection_LastCol = '\0';
static void SPConnection_AddPart(const String* text) {
	char tmpBuffer[STRING_SIZE * 2];
	String tmp = String_FromArray(tmpBuffer);
	char col;
	int i;

	/* Prepend colour codes for subsequent lines of multi-line chat */
	if (!Drawer2D_IsWhiteCol(SPConnection_LastCol)) {
		String_Append(&tmp, '&');
		String_Append(&tmp, SPConnection_LastCol);
	}
	String_AppendString(&tmp, text);
	
	/* Replace all % with & */
	for (i = 0; i < tmp.length; i++) {
		if (tmp.buffer[i] == '%') tmp.buffer[i] = '&';
	}
	String_TrimEnd(&tmp);

	col = Drawer2D_LastCol(&tmp, tmp.length);
	if (col) SPConnection_LastCol = col;
	Chat_Add(&tmp);
}

static void SPConnection_SendChat(const String* text) {
	String left, part;
	if (!text->length) return;

	SPConnection_LastCol = '\0';
	left = *text;

	while (left.length > STRING_SIZE) {
		part = String_UNSAFE_Substring(&left, 0, STRING_SIZE);
		SPConnection_AddPart(&part);
		left = String_UNSAFE_SubstringAt(&left, STRING_SIZE);
	}
	SPConnection_AddPart(&left);
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
uint8_t  net_readBuffer[4096 * 5];
uint8_t  net_writeBuffer[131];
uint8_t* net_readCurrent;

bool net_writeFailed;
int net_ticks;
TimeMS net_lastPacket;
uint8_t net_lastOpcode;
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
	static String reason = String_FromConst("You failed to connect to the server. It's probably down!");
	char msgBuffer[STRING_SIZE * 2];
	String msg = String_FromArray(msgBuffer);

	net_connecting = false;
	if (result) {
		String_Format3(&msg, "Error connecting to %s:%i: %i", &Game_IPAddress, &Game_Port, &result);
		ErrorHandler_Log(&msg);
		msg.length = 0;
	}

	String_Format2(&msg, "Failed to connect to %s:%i", &Game_IPAddress, &Game_Port);
	Game_Disconnect(&msg, &reason);
	ServerConnection_Free();
}

static void MPConnection_TickConnect(void) {
	ReturnCode res = 0;
	Socket_GetError(net_socket, &res);
	if (res) { MPConnection_FailConnect(res); return; }

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
	ReturnCode res;
	Event_RegisterBlock(&UserEvents_BlockChanged, NULL, MPConnection_BlockChanged);
	Socket_Create(&net_socket);
	ServerConnection_Disconnected = false;

	Socket_SetBlocking(net_socket, false);
	net_connecting = true;
	net_connectTimeout = DateTime_CurrentUTC_MS() + NET_TIMEOUT_MS;

	res = Socket_Connect(net_socket, &Game_IPAddress, Game_Port);
	if (res && res != ReturnCode_SocketInProgess && res != ReturnCode_SocketWouldBlock) {
		MPConnection_FailConnect(res);
	}
}

static void MPConnection_SendChat(const String* text) {
	String left, part;
	if (!text->length || net_connecting) return;
	left = *text;

	while (left.length > STRING_SIZE) {
		part = String_UNSAFE_Substring(&left, 0, STRING_SIZE);
		Classic_WriteChat(&part, true);
		Net_SendPacket();
		left = String_UNSAFE_SubstringAt(&left, STRING_SIZE);
	}

	Classic_WriteChat(&left, false);
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
	static String title  = String_FromConst("Disconnected!");
	static String reason = String_FromConst("You've lost connection to the server");

	net_discAccumulator += delta;
	if (net_discAccumulator < 1.0) return;
	net_discAccumulator = 0.0;

	uint32_t available = 0; bool poll_success = false;
	ReturnCode availResult  = Socket_Available(net_socket, &available);
	ReturnCode selectResult = Socket_Select(net_socket, SOCKET_SELECT_READ, &poll_success);

	if (net_writeFailed || availResult || selectResult || (available == 0 && poll_success)) {	
		Game_Disconnect(&title, &reason);
	}
}

static void MPConnection_Tick(struct ScheduledTask* task) {
	static String title_lost  = String_FromConst("&eLost connection to the server");
	static String reason_err  = String_FromConst("I/O error when reading packets");
	static String title_disc  = String_FromConst("Disconnected");
	static String msg_invalid = String_FromConst("Server sent invalid packet!");

	if (ServerConnection_Disconnected) return;
	if (net_connecting) { MPConnection_TickConnect(); return; }

	/* over 30 seconds since last packet */
	TimeMS now = DateTime_CurrentUTC_MS();
	if (net_lastPacket + (30 * 1000) < now) {
		MPConnection_CheckDisconnection(task->Interval);
	}
	if (ServerConnection_Disconnected) return;

	uint32_t pending = 0;
	ReturnCode res = Socket_Available(net_socket, &pending);
	uint8_t* readEnd = net_readCurrent;

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
		Game_Disconnect(&title_lost, &reason_err);
		return;
	}

	net_readCurrent = net_readBuffer;
	while (net_readCurrent < readEnd) {
		uint8_t opcode = net_readCurrent[0];

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
			Game_Disconnect(&title_disc, &msg_invalid); return; 
		}

		if (net_readCurrent + Net_PacketSizes[opcode] > readEnd) break;
		net_lastOpcode = opcode;
		net_lastPacket = DateTime_CurrentUTC_MS();

		Net_Handler handler = Net_Handlers[opcode];
		if (!handler) {
			Game_Disconnect(&title_disc, &msg_invalid); return;
		}

		handler(net_readCurrent + 1);  /* skip opcode */
		net_readCurrent += Net_PacketSizes[opcode];
	}

	/* Keep last few unprocessed bytes, don't care about rest since they'll be overwritten on socket read */
	uint32_t i, remaining = (uint32_t)(readEnd - net_readCurrent);
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

void Net_Set(uint8_t opcode, Net_Handler handler, int packetSize) {
	Net_Handlers[opcode]    = handler;
	Net_PacketSizes[opcode] = packetSize;
}

void Net_SendPacket(void) {
	uint32_t left, wrote;
	uint8_t* cur;
	ReturnCode res;

	left = (uint32_t)(ServerConnection_WriteBuffer - net_writeBuffer);
	ServerConnection_WriteBuffer = net_writeBuffer;
	if (ServerConnection_Disconnected) return;

	/* NOTE: Not immediately disconnecting here, as otherwise we sometimes miss out on kick messages */
	cur = net_writeBuffer;
	while (left) {
		res = Socket_Write(net_socket, cur, left, &wrote);
		if (res || !wrote) { net_writeFailed = true; break; }
		cur += wrote; left -= wrote;
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
	int i;
	if (ServerConnection_IsSinglePlayer) return;

	/* wipe all existing entities */
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		Handlers_RemoveEntity((EntityID)i);
	}
}

static void MPConnection_Reset(void) {
	int i;
	if (ServerConnection_IsSinglePlayer) return;
	
	for (i = 0; i < OPCODE_COUNT; i++) {
		Net_Handlers[i]    = NULL;
		Net_PacketSizes[i] = 0;
	}

	net_writeFailed = false;
	Block_SetUsedCount(256);
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
