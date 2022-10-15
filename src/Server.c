#include "Server.h"
#include "String.h"
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
#include "Input.h"
#include "Errors.h"

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

void Server_RetrieveTexturePack(const cc_string* url) {
	if (!Game_AllowServerTextures || TextureCache_HasDenied(url)) return;

	if (!url->length || TextureCache_HasAccepted(url)) {
		TexturePack_Extract(url);
	} else {
		TexPackOverlay_Show(url);
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

static void Ping_Reset(void) {
	Mem_Set(ping_entries, 0, sizeof(ping_entries));
	ping_head = 0;
}


/*########################################################################################################################*
*-------------------------------------------------Singleplayer connection-------------------------------------------------*
*#########################################################################################################################*/
#define SP_HasDir(path) (String_IndexOf(&path, '/') >= 0 || String_IndexOf(&path, '\\') >= 0)
static void SPConnection_BeginConnect(void) {
	static const cc_string logName = String_FromConst("Singleplayer");
	cc_string path;
	RNGState rnd;
	Chat_SetLogName(&logName);
	Game_UseCPEBlocks = Game_UseCPE;

	/* For when user drops a map file onto ClassiCube.exe */
	path = Game_Username;
	if (SP_HasDir(path) && File_Exists(&path)) {
		Map_LoadFrom(&path); return;
	}

	Random_SeedFromCurrentTime(&rnd);
	World_NewMap();
	World_SetDimensions(128, 64, 128);

	Gen_Vanilla = true;
	Gen_Seed    = Random_Next(&rnd, Int32_MaxValue);
	GeneratingScreen_Show();
}

static char sp_lastCol;
static void SPConnection_AddPart(const cc_string* text) {
	cc_string tmp; char tmpBuffer[STRING_SIZE * 2];
	char col;
	int i;
	String_InitArray(tmp, tmpBuffer);

	/* Prepend color codes for subsequent lines of multi-line chat */
	if (!Drawer2D_IsWhiteColor(sp_lastCol)) {
		String_Append(&tmp, '&');
		String_Append(&tmp, sp_lastCol);
	}
	String_AppendString(&tmp, text);
	
	/* Replace all % with & */
	for (i = 0; i < tmp.length; i++) {
		if (tmp.buffer[i] == '%') tmp.buffer[i] = '&';
	}
	String_UNSAFE_TrimEnd(&tmp);

	col = Drawer2D_LastColor(&tmp, tmp.length);
	if (col) sp_lastCol = col;
	Chat_Add(&tmp);
}

static void SPConnection_SendChat(const cc_string* text) {
	cc_string left, part;
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

static void SPConnection_SendBlock(int x, int y, int z, BlockID old, BlockID now) {
	Physics_OnBlockChanged(x, y, z, old, now);
}

static void SPConnection_SendPosition(Vec3 pos, float yaw, float pitch) { }
static void SPConnection_SendData(const cc_uint8* data, cc_uint32 len) { }

static void SPConnection_Tick(struct ScheduledTask* task) {
	if (Server.Disconnected) return;
	if ((ticks % 3) == 0) { /* 60 -> 20 ticks a second */
		Physics_Tick();
		TexturePack_CheckPending();
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
static cc_socket net_socket;
static cc_uint8  net_readBuffer[4096 * 5];
static cc_uint8  net_writeBuffer[131];
static cc_uint8* net_readCurrent;

static cc_result net_writeFailure;
static double lastPacket;
static cc_uint8 lastOpcode;

static cc_bool net_connecting;
static double net_connectTimeout;
#define NET_TIMEOUT_SECS 15

static void OnClose(void);
static void MPConnection_FinishConnect(void) {
	net_connecting = false;
	Event_RaiseVoid(&NetEvents.Connected);
	Event_RaiseFloat(&WorldEvents.Loading, 0.0f);

	net_readCurrent    = net_readBuffer;
	Server.WriteBuffer = net_writeBuffer;

	Classic_SendLogin();
	lastPacket = Game.Time;
}

static void MPConnection_Fail(const cc_string* reason) {
	cc_string msg; char msgBuffer[STRING_SIZE * 2];
	String_InitArray(msg, msgBuffer);
	net_connecting = false;

	String_Format2(&msg, "Failed to connect to %s:%i", &Server.Address, &Server.Port);
	Game_Disconnect(&msg, reason);
	OnClose();
}

static void MPConnection_FailConnect(cc_result result) {
	static const cc_string reason = String_FromConst("You failed to connect to the server. It's probably down!");
	cc_string msg; char msgBuffer[STRING_SIZE * 2];
	String_InitArray(msg, msgBuffer);

	if (result) {
		String_Format3(&msg, "Error connecting to %s:%i: %i" _NL, &Server.Address, &Server.Port, &result);
		Logger_Log(&msg);
	}
	MPConnection_Fail(&reason);
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
		MPConnection_FinishConnect();
	} else if (now > net_connectTimeout) {
		MPConnection_FailConnect(0);
	} else {
		double left = net_connectTimeout - now;
		Event_RaiseFloat(&WorldEvents.Loading, (float)left / NET_TIMEOUT_SECS);
	}
}

static void MPConnection_BeginConnect(void) {
	cc_string title; char titleBuffer[STRING_SIZE];
	cc_result res;
	String_InitArray(title, titleBuffer);

	/* Default block permissions (in case server supports SetBlockPermissions but doesn't send) */
	Blocks.CanPlace[BLOCK_AIR] = false;
	Blocks.CanPlace[BLOCK_LAVA] = false;        Blocks.CanDelete[BLOCK_LAVA] = false;
	Blocks.CanPlace[BLOCK_WATER] = false;       Blocks.CanDelete[BLOCK_WATER] = false;
	Blocks.CanPlace[BLOCK_STILL_LAVA] = false;  Blocks.CanDelete[BLOCK_STILL_LAVA] = false;
	Blocks.CanPlace[BLOCK_STILL_WATER] = false; Blocks.CanDelete[BLOCK_STILL_WATER] = false;
	Blocks.CanPlace[BLOCK_BEDROCK] = false;     Blocks.CanDelete[BLOCK_BEDROCK] = false;
	
	res = Socket_Connect(&net_socket, &Server.Address, Server.Port);
	if (res == ERR_INVALID_ARGUMENT) {
		static const cc_string reason = String_FromConst("Invalid IP address");
		MPConnection_Fail(&reason);
	} else if (res && res != ReturnCode_SocketInProgess && res != ReturnCode_SocketWouldBlock) {
		MPConnection_FailConnect(res);
	} else {
		Server.Disconnected = false;
		net_connecting      = true;
		net_connectTimeout  = Game.Time + NET_TIMEOUT_SECS;

		String_Format2(&title, "Connecting to %s:%i..", &Server.Address, &Server.Port);
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

static void MPConnection_SendChat(const cc_string* text) {
	cc_string left;
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

static void MPConnection_Disconnect(void) {
	static const cc_string title  = String_FromConst("Disconnected!");
	static const cc_string reason = String_FromConst("You've lost connection to the server");
	Game_Disconnect(&title, &reason);
}

static void MPConnection_CheckDisconnection(void) {
	cc_result availRes, selectRes;
	int pending = 0;
	cc_bool poll_read;

	availRes  = Socket_Available(net_socket, &pending);
	/* poll read returns true when socket is closed */
	selectRes = Socket_Poll(net_socket, SOCKET_POLL_READ, &poll_read);

	if (net_writeFailure || availRes || selectRes || (pending == 0 && poll_read)) {
		MPConnection_Disconnect();
	}
}

static void DisconnectInvalidOpcode(cc_uint8 opcode) {
	static const cc_string title = String_FromConst("Disconnected");
	cc_string tmp; char tmpBuffer[STRING_SIZE];
	String_InitArray(tmp, tmpBuffer);

	String_Format2(&tmp, "Server sent invalid packet %b! (prev %b)", &opcode, &lastOpcode);
	Game_Disconnect(&title, &tmp); return;
}

static void MPConnection_Tick(struct ScheduledTask* task) {
	static const cc_string title_lost  = String_FromConst("&eLost connection to the server");
	static const cc_string reason_err  = String_FromConst("I/O error when reading packets");
	cc_string msg; char msgBuffer[STRING_SIZE * 2];
	cc_uint32 pending;
	cc_uint8* readEnd;
	Net_Handler handler;
	int i, remaining;
	cc_result res;

	if (Server.Disconnected) return;
	if (net_connecting) { MPConnection_TickConnect(); return; }

	/* Over 30 seconds since last packet, connection likely dropped */
	if (lastPacket + 30 < Game.Time) MPConnection_CheckDisconnection();
	if (Server.Disconnected) return;

	pending = 0;
	res     = Socket_Available(net_socket, &pending);
	readEnd = net_readCurrent; /* todo change to int remaining instead */

	if (!res && pending) {
		/* NOTE: Always using a read call that is a multiple of 4096 (appears to?) improve read performance */	
		res = Socket_Read(net_socket, net_readCurrent, 4096 * 4, &pending);
		/* Ignore errors for 'no data available for non-blocking read' */
		if (res) {
			if (res == ReturnCode_SocketInProgess)  return;
			if (res == ReturnCode_SocketWouldBlock) return;
		}
		readEnd += pending;
	}

	if (res) {
		String_InitArray(msg, msgBuffer);
		String_Format3(&msg, "Error reading from %s:%i: %i" _NL, &Server.Address, &Server.Port, &res);

		Logger_Log(&msg);
		Game_Disconnect(&title_lost, &reason_err);
		return;
	}

	net_readCurrent = net_readBuffer;
	while (net_readCurrent < readEnd) {
		cc_uint8 opcode = net_readCurrent[0];

		/* Workaround for older D3 servers which wrote one byte too many for HackControl packets */
		if (cpe_needD3Fix && lastOpcode == OPCODE_HACK_CONTROL && (opcode == 0x00 || opcode == 0xFF)) {
			Platform_LogConst("Skipping invalid HackControl byte from D3 server");
			net_readCurrent++;
			LocalPlayer_ResetJumpVelocity();
			continue;
		}

		if (net_readCurrent + Protocol.Sizes[opcode] > readEnd) break;
		handler = Protocol.Handlers[opcode];
		if (!handler) { DisconnectInvalidOpcode(opcode); return; }

		lastOpcode = opcode;
		lastPacket = Game.Time;
		handler(net_readCurrent + 1); /* skip opcode */
		net_readCurrent += Protocol.Sizes[opcode];
	}

	/* Protocol packets might be split up across TCP packets */
	/* If so, copy last few unprocessed bytes back to beginning of buffer */
	/* These bytes are then later combined with subsequently read TCP packet data */
	remaining = (int)(readEnd - net_readCurrent);
	for (i = 0; i < remaining; i++) {
		net_readBuffer[i] = net_readCurrent[i];
	}
	net_readCurrent = net_readBuffer + remaining;

	if (net_writeFailure) {
		Platform_Log1("Error from send: %i", &net_writeFailure);
		MPConnection_Disconnect();
	}

	/* Network is ticked 60 times a second. We only send position updates 20 times a second */
	if ((ticks % 3) == 0) {
		TexturePack_CheckPending();
		Protocol_Tick();
		/* Have any packets been written? */
		if (Server.WriteBuffer != net_writeBuffer) Net_SendPacket();
	}
	ticks++;
}

static void MPConnection_SendData(const cc_uint8* data, cc_uint32 len) {
	cc_uint32 wrote;
	cc_result res;
	int tries = 0;
	if (Server.Disconnected) return;

	while (len) {
		res = Socket_Write(net_socket, data, len, &wrote);
		/* If sending would block (send buffer full), retry for a bit up to 10 seconds */
		/* TODO: Avoid doing this and manually buffer data when this happens */
		if (res && tries < 1000 && (res == ReturnCode_SocketInProgess || res == ReturnCode_SocketWouldBlock)) {
			Thread_Sleep(10);
			tries++;
			continue;
		}

		/* NOTE: Not immediately disconnecting here, as otherwise we sometimes miss out on kick messages */
		if (res)    { net_writeFailure = res;                  return; }
		if (!wrote) { net_writeFailure = ERR_INVALID_ARGUMENT; return; }

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


static void OnNewMap(void) {
	int i;
	if (Server.IsSinglePlayer) return;

	/* wipe all existing entities */
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		Protocol_RemoveEntity((EntityID)i);
	}
}

static void OnInit(void) {
	String_InitArray(Server.Name,    nameBuffer);
	String_InitArray(Server.MOTD,    motdBuffer);
	String_InitArray(Server.AppName, appBuffer);

	if (!Server.Address.length) {
		SPConnection_Init();
	} else {
		MPConnection_Init();
	}

	ScheduledTask_Add(GAME_NET_TICKS, Server.Tick);
	String_AppendConst(&Server.AppName, GAME_APP_NAME);

#ifdef CC_BUILD_WEB
	if (!Input_TouchMode) return;
	Server.AppName.length = 0;
	String_AppendConst(&Server.AppName, GAME_APP_ALT);
#endif
}

static void OnReset(void) {
	if (Server.IsSinglePlayer) return;
	net_writeFailure = 0;
	OnClose();
}

static void OnFree(void) {
	Server.Address.length = 0;
	OnClose();
}

static void OnClose(void) {
	if (Server.IsSinglePlayer) {
		Physics_Free();
	} else {
		Ping_Reset();
		if (Server.Disconnected) return;

		Socket_Close(net_socket);
		Server.Disconnected = true;
	}
}

struct IGameComponent Server_Component = {
	OnInit,  /* Init  */
	OnFree,  /* Free  */
	OnReset, /* Reset */
	OnNewMap /* OnNewMap */
};
