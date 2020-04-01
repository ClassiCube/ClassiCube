#include "Server.h"
#include "BlockPhysics.h"
#include "Game.h"
#include "Drawer2D.h"
#include "Chat.h"
#include "Block.h"
#include "Event.h"
#include "Http.h"
#include "Funcs.h"
#include "Entity.h"
#include "Graphics.h"
#include "Gui.h"
#include "Screens.h"
#include "Formats.h"
#include "Generator.h"
#include "World.h"
#include "Camera.h"
#include "TexturePack.h"
#include "Menus.h"
#include "Logger.h"
#include "Protocol.h"
#include "Inventory.h"
#include "Platform.h"
#include "GameStructs.h"

static char nameBuffer[STRING_SIZE];
static char motdBuffer[STRING_SIZE];
static char appBuffer[STRING_SIZE];
static int ticks;
struct _ServerConnectionData Server;

/*########################################################################################################################*
*-----------------------------------------------------Common handlers-----------------------------------------------------*
*#########################################################################################################################*/
static void Server_ResetState(void) {
	Server.Disconnected            = false;
	Server.SupportsExtPlayerList   = false;
	Server.SupportsPlayerClick     = false;
	Server.SupportsPartialMessages = false;
	Server.SupportsFullCP437       = false;
}

void Server_RetrieveTexturePack(const String* url) {
	if (!Game_AllowServerTextures || TextureCache_HasDenied(url)) return;

	if (!url->length || TextureCache_HasAccepted(url)) {
		World_ApplyTexturePack(url);
	} else {
		TexPackOverlay_Show(url);
	}
}

static void Server_CheckAsyncResources(void) {
	static const String texPack = String_FromConst("texturePack");
	struct HttpRequest item;
	if (!Http_GetResult(&texPack, &item)) return;

	if (item.success) {
		TexturePack_Apply(&item);
		HttpRequest_Free(&item);
	} else if (item.result) {
		Logger_SysWarn(item.result, "trying to download texture pack", Http_DescribeError);
	} else {
		int status = item.statusCode;
		if (status == 200 || status == 304) return;
		Chat_Add1("&c%i error when trying to download texture pack", &status);
	}
}


/*########################################################################################################################*
*--------------------------------------------------------PingList---------------------------------------------------------*
*#########################################################################################################################*/
struct PingEntry { cc_int64 sent, recv; cc_uint16 id; };
static struct PingEntry ping_entries[10];
static int ping_head;

int Ping_NextPingId(void) {
	int head = ping_head;
	int next = ping_entries[head].id + 1;

	head = (head + 1) % Array_Elems(ping_entries);
	ping_entries[head].id   = next;
	ping_entries[head].sent = DateTime_CurrentUTC_MS();
	ping_entries[head].recv = 0;
	
	ping_head = head;
	return next;
}

void Ping_Update(int id) {
	int i;
	for (i = 0; i < Array_Elems(ping_entries); i++) {
		if (ping_entries[i].id != id) continue;

		ping_entries[i].recv = DateTime_CurrentUTC_MS();
		return;
	}
}

int Ping_AveragePingMS(void) {
	int i, measures = 0, totalMs = 0;

	for (i = 0; i < Array_Elems(ping_entries); i++) {
		struct PingEntry entry = ping_entries[i];
		if (!entry.sent || !entry.recv) continue;
	
		totalMs += (int)(entry.recv - entry.sent);
		measures++;
	}

	if (!measures) return 0;
	/* (recv - send) is time for packet to be sent to server and then sent back. */
	/* However for ping, only want time to send data to server, so half the total. */
	totalMs /= 2;
	return totalMs / measures;
}


/*########################################################################################################################*
*-------------------------------------------------Singleplayer connection-------------------------------------------------*
*#########################################################################################################################*/
#define SP_HasDir(path) (String_IndexOf(&path, '/') >= 0 || String_IndexOf(&path, '\\') >= 0)
static void SPConnection_BeginConnect(void) {
	static const String logName = String_FromConst("Singleplayer");
	String path;
	RNGState rnd;
	int i, count;

	Chat_SetLogName(&logName);
	Game_UseCPEBlocks = Game_UseCPE;
	count = Game_UseCPEBlocks ? BLOCK_CPE_COUNT : BLOCK_ORIGINAL_COUNT;

	for (i = 1; i < count; i++) {
		Blocks.CanPlace[i]  = true;
		Blocks.CanDelete[i] = true;
	}
	Event_RaiseVoid(&BlockEvents.PermissionsChanged);

	/* For when user drops a map file onto ClassiCube.exe */
	path = Game_Username;
	if (SP_HasDir(path) && File_Exists(&path)) {
		Map_LoadFrom(&path);
		return;
	}

	Random_SeedFromCurrentTime(&rnd);
	World_NewMap();
	World_SetDimensions(128, 64, 128);

	Gen_Vanilla = true;
	Gen_Seed    = Random_Next(&rnd, Int32_MaxValue);
	GeneratingScreen_Show();
}

static char sp_lastCol = '\0';
static void SPConnection_AddPart(const String* text) {
	String tmp; char tmpBuffer[STRING_SIZE * 2];
	char col;
	int i;
	String_InitArray(tmp, tmpBuffer);

	/* Prepend colour codes for subsequent lines of multi-line chat */
	if (!Drawer2D_IsWhiteCol(sp_lastCol)) {
		String_Append(&tmp, '&');
		String_Append(&tmp, sp_lastCol);
	}
	String_AppendString(&tmp, text);
	
	/* Replace all % with & */
	for (i = 0; i < tmp.length; i++) {
		if (tmp.buffer[i] == '%') tmp.buffer[i] = '&';
	}
	String_UNSAFE_TrimEnd(&tmp);

	col = Drawer2D_LastCol(&tmp, tmp.length);
	if (col) sp_lastCol = col;
	Chat_Add(&tmp);
}

static void SPConnection_SendBlock(int x, int y, int z, BlockID old, BlockID now) {
	Physics_OnBlockChanged(x, y, z, old, now);
}

static void SPConnection_SendChat(const String* text) {
	String left, part;
	if (!text->length) return;

	sp_lastCol = '\0';
	left = *text;

	while (left.length > STRING_SIZE) {
		part = String_UNSAFE_Substring(&left, 0, STRING_SIZE);
		SPConnection_AddPart(&part);
		left = String_UNSAFE_SubstringAt(&left, STRING_SIZE);
	}
	SPConnection_AddPart(&left);
}

static void SPConnection_SendPosition(Vec3 pos, float yaw, float pitch) { }
static void SPConnection_SendData(const cc_uint8* data, cc_uint32 len) { }

static void SPConnection_Tick(struct ScheduledTask* task) {
	if (Server.Disconnected) return;
	if ((ticks % 3) == 0) { /* 60 -> 20 ticks a second */
		Physics_Tick();
		Server_CheckAsyncResources();
	}
	ticks++;
}

static void SPConnection_Init(void) {
	Server_ResetState();
	Physics_Init();

	Server.BeginConnect = SPConnection_BeginConnect;
	Server.Tick         = SPConnection_Tick;
	Server.SendBlock    = SPConnection_SendBlock;
	Server.SendChat     = SPConnection_SendChat;
	Server.SendPosition = SPConnection_SendPosition;
	Server.SendData     = SPConnection_SendData;
	
	Server.SupportsFullCP437       = !Game_ClassicMode;
	Server.SupportsPartialMessages = true;
	Server.IsSinglePlayer          = true;
	Server.WriteBuffer = NULL;
}


/*########################################################################################################################*
*--------------------------------------------------Multiplayer connection-------------------------------------------------*
*#########################################################################################################################*/
static SocketHandle net_socket;
static cc_uint8  net_readBuffer[4096 * 5];
static cc_uint8  net_writeBuffer[131];
static cc_uint8* net_readCurrent;

static cc_bool net_writeFailed;
static double net_lastPacket;
static cc_uint8 net_lastOpcode;

static cc_bool net_connecting;
static double net_connectTimeout;
#define NET_TIMEOUT_SECS 15

static void Server_Free(void);
static void MPConnection_FinishConnect(void) {
	net_connecting = false;
	Event_RaiseVoid(&NetEvents.Connected);
	Event_RaiseFloat(&WorldEvents.Loading, 0.0f);

	net_readCurrent    = net_readBuffer;
	Server.WriteBuffer = net_writeBuffer;

	Protocol_Reset();
	Classic_SendLogin(&Game_Username, &Game_Mppass);
	net_lastPacket = Game.Time;
}

static void MPConnection_FailConnect(cc_result result) {
	static const String reason = String_FromConst("You failed to connect to the server. It's probably down!");
	String msg; char msgBuffer[STRING_SIZE * 2];

	net_connecting = false;
	String_InitArray(msg, msgBuffer);

	if (result) {
		String_Format3(&msg, "Error connecting to %s:%i: %i" _NL, &Server.IP, &Server.Port, &result);
		Logger_Log(&msg);
		msg.length = 0;
	}

	String_Format2(&msg, "Failed to connect to %s:%i", &Server.IP, &Server.Port);
	Game_Disconnect(&msg, &reason);
	Server_Free();
}

static void MPConnection_TickConnect(void) {
	cc_result res = 0;
	cc_bool poll_write;
	double now;

	Socket_GetError(net_socket, &res);
	if (res) { MPConnection_FailConnect(res); return; }

	Socket_Poll(net_socket, SOCKET_POLL_WRITE, &poll_write);
	now = Game.Time;

	if (poll_write) {
		Socket_SetBlocking(net_socket, true);
		MPConnection_FinishConnect();
	} else if (now > net_connectTimeout) {
		MPConnection_FailConnect(0);
	} else {
		double left = net_connectTimeout - now;
		Event_RaiseFloat(&WorldEvents.Loading, (float)left / NET_TIMEOUT_SECS);
	}
}

static void MPConnection_BeginConnect(void) {
	String title; char titleBuffer[STRING_SIZE];
	cc_result res;
	String_InitArray(title, titleBuffer);
	
	res = Socket_Create(&net_socket);
	if (res) { MPConnection_FailConnect(res); return; }
	Server.Disconnected = false;

	Socket_SetBlocking(net_socket, false);
	net_connecting     = true;
	net_connectTimeout = Game.Time + NET_TIMEOUT_SECS;

	res = Socket_Connect(net_socket, &Server.IP, Server.Port);
	if (res && res != ReturnCode_SocketInProgess && res != ReturnCode_SocketWouldBlock) {
		MPConnection_FailConnect(res);
	} else {
		String_Format2(&title, "Connecting to %s:%i..", &Server.IP, &Server.Port);
		LoadingScreen_Show(&title, &String_Empty);
	}
}

static void MPConnection_SendBlock(int x, int y, int z, BlockID old, BlockID now) {
	if (now == BLOCK_AIR) {
		now = Inventory_SelectedBlock;
		Classic_WriteSetBlock(x, y, z, false, now);
	} else {
		Classic_WriteSetBlock(x, y, z, true, now);
	}
	Net_SendPacket();
}

static void MPConnection_SendChat(const String* text) {
	String left;
	if (!text->length || net_connecting) return;
	left = *text;

	while (left.length > STRING_SIZE) {
		Classic_SendChat(&left, true);
		left = String_UNSAFE_SubstringAt(&left, STRING_SIZE);
	}
	Classic_SendChat(&left, false);
}

static void MPConnection_SendPosition(Vec3 pos, float yaw, float pitch) {
	Classic_WritePosition(pos, yaw, pitch);
	Net_SendPacket();
}

static void MPConnection_CheckDisconnection(void) {
	static const String title  = String_FromConst("Disconnected!");
	static const String reason = String_FromConst("You've lost connection to the server");
	cc_result availRes, selectRes;
	cc_uint32 pending = 0;
	cc_bool poll_read;

	availRes  = Socket_Available(net_socket, &pending);
	/* poll read returns true when socket is closed */
	selectRes = Socket_Poll(net_socket, SOCKET_POLL_READ, &poll_read);

	if (net_writeFailed || availRes || selectRes || (pending == 0 && poll_read)) {	
		Game_Disconnect(&title, &reason);
	}
}

static void MPConnection_Tick(struct ScheduledTask* task) {
	static const String title_lost  = String_FromConst("&eLost connection to the server");
	static const String reason_err  = String_FromConst("I/O error when reading packets");
	static const String title_disc  = String_FromConst("Disconnected");
	static const String msg_invalid = String_FromConst("Server sent invalid packet!");
	String msg; char msgBuffer[STRING_SIZE * 2];

	struct LocalPlayer* p;
	cc_uint32 pending;
	cc_uint8* readEnd;
	Net_Handler handler;
	int i, remaining;
	cc_result res;

	if (Server.Disconnected) return;
	if (net_connecting) { MPConnection_TickConnect(); return; }

	/* Over 30 seconds since last packet, connection likely dropped */
	if (net_lastPacket + 30 < Game.Time) MPConnection_CheckDisconnection();
	if (Server.Disconnected) return;

	pending = 0;
	res     = Socket_Available(net_socket, &pending);
	readEnd = net_readCurrent;

	if (!res && pending) {
		/* NOTE: Always using a read call that is a multiple of 4096 (appears to?) improve read performance */	
		res = Socket_Read(net_socket, net_readCurrent, 4096 * 4, &pending);
		readEnd += pending;
	}

	if (res) {
		String_InitArray(msg, msgBuffer);
		String_Format3(&msg, "Error reading from %s:%i: %i" _NL, &Server.IP, &Server.Port, &res);

		Logger_Log(&msg);
		Game_Disconnect(&title_lost, &reason_err);
		return;
	}

	net_readCurrent = net_readBuffer;
	while (net_readCurrent < readEnd) {
		cc_uint8 opcode = net_readCurrent[0];

		/* Workaround for older D3 servers which wrote one byte too many for HackControl packets */
		if (cpe_needD3Fix && net_lastOpcode == OPCODE_HACK_CONTROL && (opcode == 0x00 || opcode == 0xFF)) {
			Platform_LogConst("Skipping invalid HackControl byte from D3 server");
			net_readCurrent++;

			p = &LocalPlayer_Instance;
			p->Physics.JumpVel = 0.42f; /* assume default jump height */
			p->Physics.ServerJumpVel = p->Physics.JumpVel;
			continue;
		}

		if (opcode >= OPCODE_COUNT) {
			Game_Disconnect(&title_disc, &msg_invalid); return; 
		}

		if (net_readCurrent + Net_PacketSizes[opcode] > readEnd) break;
		net_lastOpcode = opcode;
		net_lastPacket = Game.Time;

		handler = Net_Handlers[opcode];
		if (!handler) {
			Game_Disconnect(&title_disc, &msg_invalid); return;
		}

		handler(net_readCurrent + 1);  /* skip opcode */
		net_readCurrent += Net_PacketSizes[opcode];
	}

	/* Protocol packets might be split up across TCP packets */
	/* If so, copy last few unprocessed bytes back to beginning of buffer */
	/* These bytes are then later combined with subsequently read TCP packet data */
	remaining = (int)(readEnd - net_readCurrent);
	for (i = 0; i < remaining; i++) {
		net_readBuffer[i] = net_readCurrent[i];
	}
	net_readCurrent = net_readBuffer + remaining;

	/* Network is ticked 60 times a second. We only send position updates 20 times a second */
	if ((ticks % 3) == 0) {
		Server_CheckAsyncResources();
		Protocol_Tick();
		/* Have any packets been written? */
		if (Server.WriteBuffer != net_writeBuffer) {
			Net_SendPacket();
		}
	}
	ticks++;
}

static void MPConnection_SendData(const cc_uint8* data, cc_uint32 len) {
	cc_uint32 wrote;
	cc_result res;
	if (Server.Disconnected) return;

	while (len) {
		res = Socket_Write(net_socket, data, len, &wrote);
		/* NOTE: Not immediately disconnecting here, as otherwise we sometimes miss out on kick messages */
		if (res || !wrote) { net_writeFailed = true; return; }
		data += wrote; len -= wrote;
	}
}

void Net_SendPacket(void) {
	cc_uint32 len = (cc_uint32)(Server.WriteBuffer - net_writeBuffer);
	Server.WriteBuffer = net_writeBuffer;
	Server.SendData(net_writeBuffer, len);
}

static void MPConnection_Init(void) {
	Server_ResetState();
	Server.IsSinglePlayer = false;

	Server.BeginConnect = MPConnection_BeginConnect;
	Server.Tick         = MPConnection_Tick;
	Server.SendBlock    = MPConnection_SendBlock;
	Server.SendChat     = MPConnection_SendChat;
	Server.SendPosition = MPConnection_SendPosition;
	Server.SendData     = MPConnection_SendData;

	net_readCurrent    = net_readBuffer;
	Server.WriteBuffer = net_writeBuffer;
}


static void MPConnection_OnNewMap(void) {
	int i;
	if (Server.IsSinglePlayer) return;

	/* wipe all existing entities */
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		Protocol_RemoveEntity((EntityID)i);
	}
}

static void MPConnection_Reset(void) {
	int i;
	if (Server.IsSinglePlayer) return;
	
	for (i = 0; i < OPCODE_COUNT; i++) {
		Net_Handlers[i]    = NULL;
		Net_PacketSizes[i] = 0;
	}

	net_writeFailed = false;
	Protocol_Reset();
	Server_Free();
}

static void Server_Init(void) {
	String_InitArray(Server.Name,    nameBuffer);
	String_InitArray(Server.MOTD,    motdBuffer);
	String_InitArray(Server.AppName, appBuffer);

	if (!Server.IP.length) {
		SPConnection_Init();
	} else {
		MPConnection_Init();
	}

	ScheduledTask_Add(GAME_NET_TICKS, Server.Tick);
	String_AppendConst(&Server.AppName, GAME_APP_NAME);
}

static void Server_Free(void) {
	if (Server.IsSinglePlayer) {
		Physics_Free();
	} else {
		if (Server.Disconnected) return;
		Socket_Close(net_socket);
		Server.Disconnected = true;
	}
}

struct IGameComponent Server_Component = {
	Server_Init, /* Init  */
	Server_Free, /* Free  */
	MPConnection_Reset,    /* Reset */
	MPConnection_OnNewMap  /* OnNewMap */
};
