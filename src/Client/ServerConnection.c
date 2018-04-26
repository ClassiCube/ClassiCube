#include "ServerConnection.h"
#include "BlockPhysics.h"
#include "Game.h"
#include "Drawer2D.h"
#include "Chat.h"
#include "Block.h"
#include "Random.h"
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

/*########################################################################################################################*
*-----------------------------------------------------Common handlers-----------------------------------------------------*
*#########################################################################################################################*/
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

void ServerConnection_RetrieveTexturePack(STRING_PURE String* url) {
	if (!TextureCache_HasAccepted(url) && !TextureCache_HasDenied(url)) {
		Screen* warning = TexPackOverlay_MakeInstance(url);
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

	String zip = String_FromConst(".zip");
	if (String_ContainsString(url, &zip)) {
		String texPack = String_FromConst("texturePack");
		AsyncDownloader_Download2(url, true, REQUEST_TYPE_DATA, &texPack, &lastModified, &etag);
	} else {
		String terrain = String_FromConst("terrain");
		AsyncDownloader_Download2(url, true, REQUEST_TYPE_BITMAP, &terrain, &lastModified, &etag);
	}
}

void ServerConnection_CheckAsyncResources(void) {
	AsyncRequest item;

	String terrain = String_FromConst("terrain");
	if (AsyncDownloader_Get(&terrain, &item)) {
		TexturePack_ExtractTerrainPng_Req(&item);
	}

	String texPack = String_FromConst("texturePack");
	if (AsyncDownloader_Get(&texPack, &item)) {
		TexturePack_ExtractTexturePack_Req(&item);
	}
}

void ServerConnection_BeginGeneration(Int32 width, Int32 height, Int32 length, Int32 seed, bool vanilla) {
	World_Reset();
	Event_RaiseVoid(&WorldEvents_NewMap);
	Gen_Done = false;

	Gui_FreeActive();
	Gui_SetActive(GeneratingScreen_MakeInstance());
	Gen_Width = width; Gen_Height = height; Gen_Length = length; Gen_Seed = seed;

	void* threadHandle;
	if (vanilla) {
		threadHandle = Platform_ThreadStart(&NotchyGen_Generate);
	} else {
		threadHandle = Platform_ThreadStart(&FlatgrassGen_Generate);
	}
	/* don't leak thread handle here */
	Platform_ThreadFreeHandle(threadHandle);
}

void ServerConnection_EndGeneration(void) {
	Gui_ReplaceActive(NULL);
	Gen_Done = false;

	if (Gen_Blocks == NULL) {
		String m = String_FromConst("&cFailed to generate the map."); Chat_Add(&m);
	} else {
		World_BlocksSize = Gen_Width * Gen_Height * Gen_Length;
		World_SetNewMap(Gen_Blocks, World_BlocksSize, Gen_Width, Gen_Height, Gen_Length);
		Gen_Blocks = NULL;

		LocalPlayer* p = &LocalPlayer_Instance;
		Real32 x = (World_Width / 2) + 0.5f, z = (World_Length / 2) + 0.5f;
		p->Spawn = Respawn_FindSpawnPosition(x, z, p->Base.Size);

		LocationUpdate update; LocationUpdate_MakePosAndOri(&update, p->Spawn, 0.0f, 0.0f, false);
		p->Base.VTABLE->SetLocation(&p->Base, &update, false);
		Game_CurrentCameraPos = Camera_Active->GetCameraPos(0.0f);
		Event_RaiseVoid(&WorldEvents_MapLoaded);
	}
}


/*########################################################################################################################*
*--------------------------------------------------------PingList---------------------------------------------------------*
*#########################################################################################################################*/
typedef struct PingEntry_ {
	Int64 TimeSent, TimeReceived;
	UInt16 Data;
} PingEntry;
PingEntry PingList_Entries[10];

UInt16 PingList_Set(Int32 i, UInt16 prev) {
	DateTime now; Platform_CurrentUTCTime(&now);
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
		DateTime now; Platform_CurrentUTCTime(&now);
		PingList_Entries[i].TimeReceived = DateTime_TotalMs(&now);
		return;
	}
}

Int32 PingList_AveragePingMs(void) {
	Real64 totalMs = 0.0;
	Int32 measures = 0;
	Int32 i;
	for (i = 0; i < Array_Elems(PingList_Entries); i++) {
		PingEntry entry = PingList_Entries[i];
		if (entry.TimeSent == 0 || entry.TimeReceived == 0) continue;

		/* Half, because received->reply time is actually twice time it takes to send data */
		totalMs += (entry.TimeReceived - entry.TimeSent) * 0.5;
		measures++;
	}
	return measures == 0 ? 0 : (Int32)(totalMs / measures);
}


/*########################################################################################################################*
*-------------------------------------------------Singleplayer connection-------------------------------------------------*
*#########################################################################################################################*/
void SPConnection_Connect(STRING_PURE String* ip, Int32 port) {
	String logName = String_FromConst("Singleplayer");
	Chat_SetLogName(&logName);
	Game_UseCPEBlocks = Game_UseCPE;

	Int32 i, count = Game_UseCPEBlocks ? BLOCK_CPE_COUNT : BLOCK_ORIGINAL_COUNT;
	for (i = 1; i < count; i++) {
		Block_CanPlace[i]  = true;
		Block_CanDelete[i] = true;
	}

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
	String_UNSAFE_TrimEnd(&tmp);

	UInt8 col = Drawer2D_LastCol(&tmp, tmp.length);
	if (col != NULL) SPConnection_LastCol = col;
	Chat_Add(&tmp);
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

	ServerConnection_ReadStream  = NULL;
	ServerConnection_WriteStream = NULL;
}


/*########################################################################################################################*
*--------------------------------------------------Multiplayer connection-------------------------------------------------*
*#########################################################################################################################*/
void* net_socket;
DateTime net_lastPacket;
UInt8 net_lastOpcode;
Stream net_readStream;
Stream net_writeStream;
UInt8 net_readBuffer[4096 * 5];
UInt8 net_writeBuffer[131];
Int32 net_maxHandledPacket;
bool net_writeFailed;
Int32 net_ticks;
Real64 net_discAccumulator;

void MPConnection_BlockChanged(void* obj, Vector3I coords, BlockID oldBlock, BlockID block) {
	Vector3I p = coords;
	if (block == BLOCK_AIR) {
		block = Inventory_SelectedBlock;
		Classic_WriteSetBlock(&net_writeStream, p.X, p.Y, p.Z, false, block);
	} else {
		Classic_WriteSetBlock(&net_writeStream, p.X, p.Y, p.Z, true, block);
	}
	Net_SendPacket();
}

void ServerConnection_Free(void);
void MPConnection_Connect(STRING_PURE String* ip, Int32 port) {
	Platform_SocketCreate(&net_socket);
	Event_RegisterBlock(&UserEvents_BlockChanged, NULL, MPConnection_BlockChanged);
	ServerConnection_Disconnected = false;

	ReturnCode result = Platform_SocketConnect(net_socket, &Game_IPAddress, Game_Port);
	if (result != 0) {
		UInt8 msgBuffer[String_BufferSize(STRING_SIZE * 2)];
		String msg = String_InitAndClearArray(msgBuffer);
		String_Format3(&msg, "Error connecting to %s:%i: %i", ip, &port, &result);
		ErrorHandler_Log(&msg);

		String_Clear(&msg);
		String_Format2(&msg, "Failed to connect to %s:%i", ip, &port);
		String reason = String_FromConst("You failed to connect to the server. It's probably down!");
		Game_Disconnect(&msg, &reason);

		ServerConnection_Free();
		return;
	}

	String streamName = String_FromConst("network socket");
	Stream_ReadonlyMemory(&net_readStream, net_readBuffer, sizeof(net_readBuffer), &streamName);
	Stream_WriteonlyMemory(&net_writeStream, net_writeBuffer, sizeof(net_writeBuffer), &streamName);
	net_readStream.Meta_Mem_Count = 0; /* initally no memory to read */

	Handlers_Reset();
	Classic_WriteLogin(&net_writeStream, &Game_Username, &Game_Mppass);
	Net_SendPacket();
	Platform_CurrentUTCTime(&net_lastPacket);
}

void MPConnection_SendChat(STRING_PURE String* text) {
	if (text->length == 0) return;
	String remaining = *text;

	while (remaining.length > STRING_SIZE) {
		String portion = String_UNSAFE_Substring(&remaining, 0, STRING_SIZE);
		Classic_WriteChat(&net_writeStream, &portion, true);
		Net_SendPacket();
		remaining = String_UNSAFE_SubstringAt(&remaining, STRING_SIZE);
	}

	Classic_WriteChat(&net_writeStream, &remaining, false);
	Net_SendPacket();
}

void MPConnection_SendPosition(Vector3 pos, Real32 rotY, Real32 headX) {
	Classic_WritePosition(&net_writeStream, pos, rotY, headX);
	Net_SendPacket();
}

void MPConnection_SendPlayerClick(MouseButton button, bool buttonDown, EntityID targetId, PickedPos* pos) {
	CPE_WritePlayerClick(&net_writeStream, button, buttonDown, targetId, pos);
	Net_SendPacket();
}

void MPConnection_CheckDisconnection(Real64 delta) {
	net_discAccumulator += delta;
	if (net_discAccumulator < 1.0) return;
	net_discAccumulator = 0.0;

	UInt32 available = 0; bool selected = false;
	ReturnCode availResult  = Platform_SocketAvailable(net_socket, &available);
	ReturnCode selectResult = Platform_SocketSelectRead(net_socket, 1000, &selected);

	if (net_writeFailed || availResult != 0 || selectResult != 0 || (selected && available == 0)) {
		String title  = String_FromConst("Disconnected!");
		String reason = String_FromConst("You've lost connection to the server");
		Game_Disconnect(&title, &reason);
	}
}

void MPConnection_Tick(ScheduledTask* task) {
	if (ServerConnection_Disconnected) return;
	DateTime now; Platform_CurrentUTCTime(&now);
	if (DateTime_MsBetween(&net_lastPacket, &now) >= 30 * 1000) {
		MPConnection_CheckDisconnection(task->Interval);
	}
	if (ServerConnection_Disconnected) return;

	UInt32 modified = 0;
	ReturnCode recvResult = Platform_SocketAvailable(net_socket, &modified);
	if (recvResult == 0 && modified > 0) {
		/* NOTE: Always using a read call that is a multiple of 4096 (appears to?) improve read performance */
		UInt8* src = net_readBuffer + net_readStream.Meta_Mem_Count;
		recvResult = Platform_SocketRead(net_socket, src, 4096 * 4, &modified);
		net_readStream.Meta_Mem_Count += modified;
	}

	if (recvResult != 0) {
		UInt8 msgBuffer[String_BufferSize(STRING_SIZE * 2)];
		String msg = String_InitAndClearArray(msgBuffer);
		String_Format3(&msg, "Error reading from %s:%i: %i", &Game_IPAddress, &Game_Port, &recvResult);
		ErrorHandler_Log(&msg);

		String title  = String_FromConst("&eLost connection to the server");
		String reason = String_FromConst("I/O error when reading packets");
		Game_Disconnect(&title, &reason);
		return;
	}

	while (net_readStream.Meta_Mem_Count > 0) {
		UInt8 opcode = net_readStream.Meta_Mem_Buffer[0];
		/* Workaround for older D3 servers which wrote one byte too many for HackControl packets */
		if (cpe_needD3Fix && net_lastOpcode == OPCODE_CPE_HACK_CONTROL && (opcode == 0x00 || opcode == 0xFF)) {
			Platform_LogConst("Skipping invalid HackControl byte from D3 server");
			Stream_Skip(&net_readStream, 1);

			LocalPlayer* p = &LocalPlayer_Instance;
			p->Physics.JumpVel = 0.42f; /* assume default jump height */
			p->Physics.ServerJumpVel = p->Physics.JumpVel;
			continue;
		}

		if (opcode > net_maxHandledPacket) { ErrorHandler_Fail("Invalid opcode"); }
		if (net_readStream.Meta_Mem_Count < Net_PacketSizes[opcode]) break;

		Stream_Skip(&net_readStream, 1); /* remove opcode */
		net_lastOpcode = opcode;
		Net_Handler handler = Net_Handlers[opcode];
		Platform_CurrentUTCTime(&net_lastPacket);

		if (handler == NULL) { ErrorHandler_Fail("Unsupported opcode"); }
		handler(&net_readStream);
	}

	/* Keep last few unprocessed bytes, don't care about rest since they'll be overwritten on socket read */
	Int32 i;
	for (i = 0; i < net_readStream.Meta_Mem_Count; i++) {
		net_readBuffer[i] = net_readStream.Meta_Mem_Buffer[i];
	}
	net_readStream.Meta_Mem_Buffer = net_readBuffer;

	/* Network is ticked 60 times a second. We only send position updates 20 times a second */
	if ((net_ticks % 3) == 0) {
		ServerConnection_CheckAsyncResources();
		Handlers_Tick();
		/* Have any packets been written? */
		if (net_writeStream.Meta_Mem_Buffer != net_writeBuffer) Net_SendPacket();
	}
	net_ticks++;
}

void Net_Set(UInt8 opcode, Net_Handler handler, UInt16 packetSize) {
	Net_Handlers[opcode] = handler;
	Net_PacketSizes[opcode] = packetSize;
	net_maxHandledPacket = max(opcode, net_maxHandledPacket);
}

void Net_SendPacket(void) {
	if (!ServerConnection_Disconnected) {
		/* NOTE: Not immediately disconnecting here, as otherwise we sometimes miss out on kick messages */
		UInt32 count = (UInt32)(net_writeStream.Meta_Mem_Buffer - net_writeBuffer), modified = 0;

		while (count > 0) {
			ReturnCode result = Platform_SocketWrite(net_socket, net_writeBuffer, count, &modified);
			if (result != 0 || modified == 0) { net_writeFailed = true; break; }
			count -= modified;
		}
	}
	
	net_writeStream.Meta_Mem_Buffer = net_writeBuffer;
	net_writeStream.Meta_Mem_Count  = sizeof(net_writeBuffer);
}

Stream* MPConnection_ReadStream(void)  { return &net_readStream; }
Stream* MPConnection_WriteStream(void) { return &net_writeStream; }
void ServerConnection_InitMultiplayer(void) {
	ServerConnection_ResetState();
	ServerConnection_IsSinglePlayer = false;

	ServerConnection_Connect = MPConnection_Connect;
	ServerConnection_SendChat = MPConnection_SendChat;
	ServerConnection_SendPosition = MPConnection_SendPosition;
	ServerConnection_SendPlayerClick = MPConnection_SendPlayerClick;
	ServerConnection_Tick = MPConnection_Tick;

	ServerConnection_ReadStream  = MPConnection_ReadStream;
	ServerConnection_WriteStream = MPConnection_WriteStream;
}


void MPConnection_OnNewMap(void) {
	if (!ServerConnection_IsSinglePlayer) return;
	/* wipe all existing entity states */
	Int32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		Handlers_RemoveEntity((EntityID)i);
	}
}

void MPConnection_Reset(void) {
	if (!ServerConnection_IsSinglePlayer) return;
	Int32 i;
	for (i = 0; i < OPCODE_COUNT; i++) {
		Net_Handlers[i] = NULL;
		Net_PacketSizes[i] = 0;
	}

	net_writeFailed = false;
	net_maxHandledPacket = 0;
	Handlers_Reset();
	ServerConnection_Free();
}

void ServerConnection_Free(void) {
	if (ServerConnection_IsSinglePlayer) {
		Physics_Free();
	} else {
		if (ServerConnection_Disconnected) return;
		Event_UnregisterBlock(&UserEvents_BlockChanged, NULL, MPConnection_BlockChanged);
		Platform_SocketClose(net_socket);
		ServerConnection_Disconnected = true;
	}
}

IGameComponent ServerConnection_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.OnNewMap = MPConnection_OnNewMap;
	comp.Reset    = MPConnection_Reset;
	comp.Free     = ServerConnection_Free;
	return comp;
}
