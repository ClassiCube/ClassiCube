#include "PacketHandlers.h"
#include "Deflate.h"
#include "Utils.h"
#include "ServerConnection.h"
#include "Stream.h"
#include "Game.h"
#include "Entity.h"
#include "Platform.h"
#include "Screens.h"
#include "World.h"
#include "Event.h"
#include "ExtMath.h"
#include "SelectionBox.h"
#include "Chat.h"
#include "Inventory.h"
#include "Block.h"
#include "Model.h"
#include "Funcs.h"
#include "Lighting.h"
#include "AsyncDownloader.h"
#include "Drawer2D.h"
#include "ErrorHandler.h"
#include "TexturePack.h"
#include "Gui.h"
#include "Errors.h"
#include "TerrainAtlas.h"	

/* Classic state */
uint8_t classic_tabList[ENTITIES_MAX_COUNT >> 3];
struct Screen* classic_prevScreen;
bool classic_receivedFirstPos;

/* Map state */
bool map_begunLoading;
TimeMS map_receiveStart;
struct InflateState map_inflateState;
struct Stream map_stream, map_part;
struct GZipHeader map_gzHeader;
int map_sizeIndex, map_index, map_volume;
uint8_t map_size[4];
BlockRaw* map_blocks;

#ifdef EXTENDED_BLOCKS
struct InflateState map2_inflateState;
struct Stream map2_stream;
int map2_index;
BlockRaw* map2_blocks;
#endif

/* CPE state */
int cpe_serverExtensionsCount, cpe_pingTicks;
int cpe_envMapVer = 2, cpe_blockDefsExtVer = 2;
bool cpe_sendHeldBlock, cpe_useMessageTypes, cpe_extEntityPos, cpe_blockPerms, cpe_fastMap;
bool cpe_twoWayPing, cpe_extTextures, cpe_extBlocks;

/*########################################################################################################################*
*-----------------------------------------------------Common handlers-----------------------------------------------------*
*#########################################################################################################################*/


#ifndef EXTENDED_BLOCKS
#define Handlers_ReadBlock(data, value) value = *data++;
#else
#define Handlers_ReadBlock(data, value)\
if (cpe_extBlocks) {\
	value = Stream_GetU16_BE(data); data += 2;\
} else { value = *data++; }
#endif

#ifndef EXTENDED_BLOCKS
#define Handlers_WriteBlock(data, value) *data++ = value;
#else
#define Handlers_WriteBlock(data, value)\
if (cpe_extBlocks) {\
	Stream_SetU16_BE(data, value); data += 2;\
} else { *data++ = (BlockRaw)value; }
#endif

static void Handlers_ReadString(uint8_t** ptr, String* str) {
	int i, length = 0;
	uint8_t* data = *ptr;
	for (i = STRING_SIZE - 1; i >= 0; i--) {
		char code = data[i];
		if (code == '\0' || code == ' ') continue;
		length = i + 1; break;
	}

	for (i = 0; i < length; i++) { String_Append(str, data[i]); }
	*ptr = data + STRING_SIZE;
}

static void Handlers_WriteString(uint8_t* data, const String* value) {
	int i, count = min(value->length, STRING_SIZE);
	for (i = 0; i < count; i++) {
		char c = value->buffer[i];
		if (c == '&') c = '%'; /* escape colour codes */
		data[i] = c;
	}

	for (; i < STRING_SIZE; i++) { data[i] = ' '; }
}

static void Handlers_RemoveEndPlus(String* value) {
	/* Workaround for MCDzienny (and others) use a '+' at the end to distinguish classicube.net accounts */
	/* from minecraft.net accounts. Unfortunately they also send this ending + to the client. */
	if (!value->length || value->buffer[value->length - 1] != '+') return;
	value->length--;
}

static void Handlers_AddTablistEntry(EntityID id, const String* playerName, const String* listName, const String* groupName, uint8_t groupRank) {
	/* Only redraw the tab list if something changed. */
	if (TabList_Valid(id)) {
		String oldPlayerName = TabList_UNSAFE_GetPlayer(id);
		String oldListName   = TabList_UNSAFE_GetList(id);
		String oldGroupName  = TabList_UNSAFE_GetList(id);
		uint8_t oldGroupRank = TabList_GroupRanks[id];

		bool changed = !String_Equals(playerName, &oldPlayerName) || !String_Equals(listName, &oldListName) 
			|| !String_Equals(groupName, &oldGroupName) || groupRank != oldGroupRank;	
		if (changed) {
			TabList_Set(id, playerName, listName, groupName, groupRank);
			Event_RaiseInt(&TabListEvents_Changed, id);
		}
	} else {
		TabList_Set(id, playerName, listName, groupName, groupRank);
		Event_RaiseInt(&TabListEvents_Added, id);
	}
}

static void Handlers_RemoveTablistEntry(EntityID id) {
	Event_RaiseInt(&TabListEvents_Removed, id);
	TabList_Remove(id);
}

static void Handlers_CheckName(EntityID id, String* displayName, String* skinName) {
	String_StripCols(skinName);
	Handlers_RemoveEndPlus(displayName);
	Handlers_RemoveEndPlus(skinName);
	/* Server is only allowed to change our own name colours. */
	if (id != ENTITIES_SELF_ID) return;

	char nameNoColsBuffer[STRING_SIZE];
	String nameNoCols = String_FromArray(nameNoColsBuffer);
	String_AppendColorless(&nameNoCols, displayName);

	if (!String_Equals(&nameNoCols, &Game_Username)) { String_Copy(displayName, &Game_Username); }
	if (!skinName->length) { String_Copy(skinName, &Game_Username); }
}

static void Classic_ReadAbsoluteLocation(uint8_t* data, EntityID id, bool interpolate);
static void Handlers_AddEntity(uint8_t* data, EntityID id, const String* displayName, const String* skinName, bool readPosition) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (id != ENTITIES_SELF_ID) {
		struct Entity* oldEntity = Entities_List[id];
		if (oldEntity) Entities_Remove(id);

		struct NetPlayer* player = &NetPlayers_List[id];
		NetPlayer_Init(player, displayName, skinName);
		Entities_List[id] = &player->Base;
		Event_RaiseInt(&EntityEvents_Added, id);
	} else {
		p->Base.VTABLE->Despawn(&p->Base);
		/* Always reset the texture here, in case other network players are using the same skin as us */
		/* In that case, we don't want the fetching of new skin for us to delete the texture used by them */
		Player_ResetSkin((struct Player*)p);
		p->FetchedSkin = false;

		Player_SetName((struct Player*)p, displayName, skinName);
		Player_UpdateNameTex((struct Player*)p);
	}

	if (!readPosition) return;
	Classic_ReadAbsoluteLocation(data, id, false);
	if (id != ENTITIES_SELF_ID) return;

	p->Spawn      = p->Base.Position;
	p->SpawnRotY  = p->Base.HeadY;
	p->SpawnHeadX = p->Base.HeadX;
}

void Handlers_RemoveEntity(EntityID id) {
	struct Entity* entity = Entities_List[id];
	if (!entity) return;
	if (id != ENTITIES_SELF_ID) Entities_Remove(id);

	/* See comment about some servers in Classic_AddEntity */
	int mask = id >> 3, bit = 1 << (id & 0x7);
	if (!(classic_tabList[mask] & bit)) return;

	Handlers_RemoveTablistEntry(id);
	classic_tabList[mask] &= (uint8_t)~bit;
}

static void Handlers_UpdateLocation(EntityID playerId, struct LocationUpdate* update, bool interpolate) {
	struct Entity* entity = Entities_List[playerId];
	if (entity) {
		entity->VTABLE->SetLocation(entity, update, interpolate);
	}
}


/*########################################################################################################################*
*------------------------------------------------------WoM protocol-------------------------------------------------------*
*#########################################################################################################################*/
/* Partially based on information from http://files.worldofminecraft.com/texturing/ */
/* NOTE: http://files.worldofminecraft.com/ has been down for quite a while, so support was removed on Oct 10, 2015 */

char wom_identifierBuffer[STRING_SIZE];
String wom_identifier = String_FromArray(wom_identifierBuffer);
int wom_counter;
bool wom_sendId, wom_sentId;

static void WoM_UpdateIdentifier(void) {
	wom_identifier.length = 0;
	String_Format1(&wom_identifier, "womenv_%i", &wom_counter);
}

static void WoM_CheckMotd(void) {
	static String cfg = String_FromConst("cfg=");
	String motd = ServerConnection_ServerMOTD, host;
	int index;

	char urlBuffer[STRING_SIZE];
	String url = String_FromArray(urlBuffer);	

	if (!motd.length) return;
	index = String_IndexOfString(&motd, &cfg);
	if (Game_PureClassic || index == -1) return;
	
	host = String_UNSAFE_SubstringAt(&motd, index + cfg.length);
	String_Format1(&url, "http://%s", &host);
	/* TODO: Replace $U with username */
	/*url = url.Replace("$U", game.Username); */

	/* Ensure that if the user quickly changes to a different world, env settings from old world aren't
	applied in the new world if the async 'get env request' didn't complete before the old world was unloaded */
	wom_counter++;
	WoM_UpdateIdentifier();
	AsyncDownloader_GetData(&url, true, &wom_identifier);
	wom_sendId = true;
}

static void WoM_CheckSendWomID(void) {
	static String msg = String_FromConst("/womid WoMClient-2.0.7");

	if (wom_sendId && !wom_sentId) {
		Chat_Send(&msg, false);
		wom_sentId = true;
	}
}

static PackedCol WoM_ParseCol(const String* value, PackedCol defaultCol) {
	PackedCol col;
	int argb;
	if (!Convert_TryParseInt(value, &argb)) return defaultCol;

	col.A = 255;
	col.R = (uint8_t)(argb >> 16);
	col.G = (uint8_t)(argb >> 8);
	col.B = (uint8_t)argb;
	return col;
}

static void WoM_ParseConfig(struct AsyncRequest* item) {
	String key, value;
	int start = 0, waterLevel;
	PackedCol col;

	char lineBuffer[STRING_SIZE * 2];
	String line = String_FromArray(lineBuffer);
	struct Stream mem;

	Stream_ReadonlyMemory(&mem, item->ResultData, item->ResultSize);
	while (!Stream_ReadLine(&mem, &line)) {
		Platform_Log(&line);
		if (!String_UNSAFE_Separate(&line, '=', &key, &value)) continue;

		if (String_CaselessEqualsConst(&key, "environment.cloud")) {
			col = WoM_ParseCol(&value, Env_DefaultCloudsCol);
			Env_SetCloudsCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.sky")) {
			col = WoM_ParseCol(&value, Env_DefaultSkyCol);
			Env_SetSkyCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.fog")) {
			col = WoM_ParseCol(&value, Env_DefaultFogCol);
			Env_SetFogCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.level")) {
			if (Convert_TryParseInt(&value, &waterLevel)) {
				Env_SetEdgeHeight(waterLevel);
			}
		} else if (String_CaselessEqualsConst(&key, "user.detail") && !cpe_useMessageTypes) {
			Chat_AddOf(&value, MSG_TYPE_STATUS_2);
		}
	}
}

static void WoM_Reset(void) {
	wom_counter = 0;
	WoM_UpdateIdentifier();
	wom_sendId = false; wom_sentId = false;
}

static void WoM_Tick(void) {
	struct AsyncRequest item;
	if (!AsyncDownloader_Get(&wom_identifier, &item)) return;

	if (item.ResultData) WoM_ParseConfig(&item);
	ASyncRequest_Free(&item);
}


/*########################################################################################################################*
*----------------------------------------------------Classic protocol-----------------------------------------------------*
*#########################################################################################################################*/

void Classic_WriteChat(const String* text, bool partial) {
	uint8_t* data = ServerConnection_WriteBuffer;
	*data++ = OPCODE_MESSAGE;
	{
		*data++ = ServerConnection_SupportsPartialMessages ? partial : ENTITIES_SELF_ID;
		Handlers_WriteString(data, text); data += STRING_SIZE;
	}
	ServerConnection_WriteBuffer = data;
}

void Classic_WritePosition(Vector3 pos, float rotY, float headX) {
	BlockID payload;
	int x, y, z;

	uint8_t* data = ServerConnection_WriteBuffer;
	*data++ = OPCODE_ENTITY_TELEPORT;
	{
		payload = cpe_sendHeldBlock ? Inventory_SelectedBlock : ENTITIES_SELF_ID;
		Handlers_WriteBlock(data, payload);
		x = (int)(pos.X * 32);
		y = (int)(pos.Y * 32) + 51;
		z = (int)(pos.Z * 32);

		if (cpe_extEntityPos) {
			Stream_SetU32_BE(data, x); data += 4;
			Stream_SetU32_BE(data, y); data += 4;
			Stream_SetU32_BE(data, z); data += 4;
		} else {
			Stream_SetU16_BE(data, x); data += 2;
			Stream_SetU16_BE(data, y); data += 2;
			Stream_SetU16_BE(data, z); data += 2;
		}

		*data++ = Math_Deg2Packed(rotY);
		*data++ = Math_Deg2Packed(headX);
	}
	ServerConnection_WriteBuffer = data;
}

void Classic_WriteSetBlock(int x, int y, int z, bool place, BlockID block) {
	uint8_t* data = ServerConnection_WriteBuffer;
	*data++ = OPCODE_SET_BLOCK_CLIENT;
	{
		Stream_SetU16_BE(data, x); data += 2;
		Stream_SetU16_BE(data, y); data += 2;
		Stream_SetU16_BE(data, z); data += 2;
		*data++ = place;
		Handlers_WriteBlock(data, block);
	}
	ServerConnection_WriteBuffer = data;
}

void Classic_WriteLogin(const String* username, const String* verKey) {
	uint8_t* data = ServerConnection_WriteBuffer;
	*data++ = OPCODE_HANDSHAKE;
	{
		*data++ = 7; /* protocol version */
		Handlers_WriteString(data, username); data += STRING_SIZE;
		Handlers_WriteString(data, verKey);   data += STRING_SIZE;
		*data++ = Game_UseCPE ? 0x42 : 0x00;
	}
	ServerConnection_WriteBuffer = data;
}

static void Classic_Handshake(uint8_t* data) {
	ServerConnection_ServerName.length = 0;
	ServerConnection_ServerMOTD.length = 0;
	data++; /* protocol version */

	Handlers_ReadString(&data, &ServerConnection_ServerName);
	Handlers_ReadString(&data, &ServerConnection_ServerMOTD);
	Chat_SetLogName(&ServerConnection_ServerName);

	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	HacksComp_SetUserType(hacks, *data, !cpe_blockPerms);
	
	String_Copy(&hacks->HacksFlags, &ServerConnection_ServerName);
	String_AppendString(&hacks->HacksFlags, &ServerConnection_ServerMOTD);
	HacksComp_UpdateState(hacks);
}

static void Classic_Ping(uint8_t* data) { }

static void Classic_StartLoading(void) {
	World_Reset();
	Event_RaiseVoid(&WorldEvents_NewMap);
	Stream_ReadonlyMemory(&map_part, NULL, 0);

	classic_prevScreen = Gui_Active;
	if (classic_prevScreen == LoadingScreen_UNSAFE_RawPointer) {
		/* otherwise replacing LoadingScreen with LoadingScreen will cause issues */
		Gui_FreeActive();
		classic_prevScreen = NULL;
	}

	Gui_SetActive(LoadingScreen_MakeInstance(&ServerConnection_ServerName, &ServerConnection_ServerMOTD));
	WoM_CheckMotd();
	classic_receivedFirstPos = false;

	GZipHeader_Init(&map_gzHeader);
	Inflate_MakeStream(&map_stream, &map_inflateState, &map_part);
	map_begunLoading = true;

	map_sizeIndex    = 0;
	map_index        = 0;
	map_receiveStart = DateTime_CurrentUTC_MS();

#ifdef EXTENDED_BLOCKS
	Inflate_MakeStream(&map2_stream, &map2_inflateState, &map_part);
	map2_index = 0;
#endif
}

static void Classic_LevelInit(uint8_t* data) {
	if (!map_begunLoading) Classic_StartLoading();

	/* Fast map puts volume in header, doesn't bother with gzip */
	if (cpe_fastMap) {
		map_volume = Stream_GetU32_BE(data);
		map_gzHeader.Done = true;
		map_sizeIndex = sizeof(uint32_t);
		map_blocks = Mem_Alloc(map_volume, 1, "map blocks");
	}
}

static void Classic_LevelDataChunk(uint8_t* data) {
	int usedLength;
	float progress;
	uint32_t left, read;

	/* Workaround for some servers that send LevelDataChunk before LevelInit due to their async sending behaviour */
	if (!map_begunLoading) Classic_StartLoading();
	usedLength = Stream_GetU16_BE(data); data += 2;

	map_part.Meta.Mem.Cur    = data;
	map_part.Meta.Mem.Base   = data;
	map_part.Meta.Mem.Left   = usedLength;
	map_part.Meta.Mem.Length = usedLength;

	data += 1024;
	uint8_t value = *data; /* progress in original classic, but we ignore it */

	if (!map_gzHeader.Done) {
		ReturnCode res = GZipHeader_Read(&map_part, &map_gzHeader);
		if (res && res != ERR_END_OF_STREAM) ErrorHandler_Fail2(res, "reading map data");
	}

	if (map_gzHeader.Done) {
		if (map_sizeIndex < 4) {
			left = 4 - map_sizeIndex;
			map_stream.Read(&map_stream, &map_size[map_sizeIndex], left, &read); 
			map_sizeIndex += read;
		}

		if (map_sizeIndex == 4) {
			if (!map_blocks) {
				map_volume = Stream_GetU32_BE(map_size);
				map_blocks = Mem_Alloc(map_volume, 1, "map blocks");
			}

#ifndef EXTENDED_BLOCKS
			left = map_volume - map_index;
			map_stream.Read(&map_stream, &map_blocks[map_index], left, &read);
			map_index += read;
#else
			if (cpe_extBlocks && value) {
				/* Only allocate map2 when needed */
				if (!map2_blocks) map2_blocks = Mem_Alloc(map_volume, 1, "map blocks upper");

				left = map_volume - map2_index;
				map2_stream.Read(&map2_stream, &map2_blocks[map2_index], left, &read); 
				map2_index += read;
			} else {
				left = map_volume - map_index;
				map_stream.Read(&map_stream, &map_blocks[map_index], left, &read); 
				map_index += read;
			}
#endif
		}
	}

	progress = !map_blocks ? 0.0f : (float)map_index / map_volume;
	Event_RaiseFloat(&WorldEvents_Loading, progress);
}

static void Classic_LevelFinalise(uint8_t* data) {
	int width, height, length;
	int loadingMs;

	Gui_CloseActive();
	Gui_Active = classic_prevScreen;
	classic_prevScreen = NULL;
	Gui_CalcCursorVisible();

	width  = Stream_GetU16_BE(&data[0]);
	height = Stream_GetU16_BE(&data[2]);
	length = Stream_GetU16_BE(&data[4]);

	loadingMs = (int)(DateTime_CurrentUTC_MS() - map_receiveStart);
	Platform_Log1("map loading took: %i", &loadingMs);

	World_SetNewMap(map_blocks, map_volume, width, height, length);
#ifdef EXTENDED_BLOCKS
	if (cpe_extBlocks) {
		/* defer allocation of scond map array if possible */
		World_Blocks2 = map2_blocks ? map2_blocks : map_blocks;
		Block_SetUsedCount(map2_blocks ? 768 : 256);
	}
#endif

	Event_RaiseVoid(&WorldEvents_MapLoaded);
	WoM_CheckSendWomID();

	map_blocks       = NULL;
	map_begunLoading = false;
#ifdef EXTENDED_BLOCKS
	map2_blocks      = NULL;
#endif
}

static void Classic_SetBlock(uint8_t* data) {
	int x, y, z;
	BlockID block;

	x = Stream_GetU16_BE(&data[0]);
	y = Stream_GetU16_BE(&data[2]);
	z = Stream_GetU16_BE(&data[4]);
	data += 6;

	Handlers_ReadBlock(data, block);
	if (World_IsValidPos(x, y, z)) {
		Game_UpdateBlock(x, y, z, block);
	}
}

static void Classic_AddEntity(uint8_t* data) {
	char nameBuffer[STRING_SIZE];
	char skinBuffer[STRING_SIZE];
	String name = String_FromArray(nameBuffer);
	String skin = String_FromArray(skinBuffer);

	EntityID id = *data++;
	Handlers_ReadString(&data, &name);
	Handlers_CheckName(id, &name, &skin);
	Handlers_AddEntity(data, id, &name, &skin, true);

	/* Workaround for some servers that declare support for ExtPlayerList but don't send ExtAddPlayerName */
	String group = String_FromConst("Players");
	Handlers_AddTablistEntry(id, &name, &name, &group, 0);
	classic_tabList[id >> 3] |= (uint8_t)(1 << (id & 0x7));
}

static void Classic_EntityTeleport(uint8_t* data) {
	EntityID id = *data++;
	Classic_ReadAbsoluteLocation(data, id, true);
}

static void Classic_RelPosAndOrientationUpdate(uint8_t* data) {
	EntityID id = *data++; 
	Vector3 pos;
	float rotY, headX;
	struct LocationUpdate update;

	pos.X = (int8_t)(*data++) / 32.0f;
	pos.Y = (int8_t)(*data++) / 32.0f;
	pos.Z = (int8_t)(*data++) / 32.0f;
	rotY  = Math_Packed2Deg(*data++);
	headX = Math_Packed2Deg(*data++);

	LocationUpdate_MakePosAndOri(&update, pos, rotY, headX, true);
	Handlers_UpdateLocation(id, &update, true);
}

static void Classic_RelPositionUpdate(uint8_t* data) {
	EntityID id = *data++; 
	Vector3 pos;
	struct LocationUpdate update;

	pos.X = (int8_t)(*data++) / 32.0f;
	pos.Y = (int8_t)(*data++) / 32.0f;
	pos.Z = (int8_t)(*data++) / 32.0f;

	LocationUpdate_MakePos(&update, pos, true);
	Handlers_UpdateLocation(id, &update, true);
}

static void Classic_OrientationUpdate(uint8_t* data) {
	EntityID id = *data++;
	float rotY, headX;
	struct LocationUpdate update;

	rotY  = Math_Packed2Deg(*data++);
	headX = Math_Packed2Deg(*data++);

	LocationUpdate_MakeOri(&update, rotY, headX);
	Handlers_UpdateLocation(id, &update, true);
}

static void Classic_RemoveEntity(uint8_t* data) {
	EntityID id = *data;
	Handlers_RemoveEntity(id);
}

static void Classic_Message(uint8_t* data) {
	static String detailMsg  = String_FromConst("^detail.user=");
	static String detailUser = String_FromConst("^detail.user");

	char textBuffer[STRING_SIZE + 2];
	String text  = String_FromArray(textBuffer);
	uint8_t type = *data++;

	/* Original vanilla server uses player ids for type, 255 for server messages (&e prefix) */
	bool prepend = !cpe_useMessageTypes && type == 0xFF;
	if (prepend) { String_AppendConst(&text, "&e"); }
	Handlers_ReadString(&data, &text);
	if (!cpe_useMessageTypes) type = MSG_TYPE_NORMAL;

	/* WoM detail messages (used e.g. for fCraft server compass) */
	if (String_CaselessStarts(&text, &detailMsg)) {
		text = String_UNSAFE_SubstringAt(&text, detailMsg.length);
		type = MSG_TYPE_STATUS_3;
	}

	/* Ignore ^detail.user.joined etc */
	if (!String_CaselessStarts(&text, &detailUser)) {
		Chat_AddOf(&text, type); 
	}
}

static void Classic_Kick(uint8_t* data) {
	static String title = String_FromConst("&eLost connection to the server");

	char reasonBuffer[STRING_SIZE];
	String reason = String_FromArray(reasonBuffer);	

	Handlers_ReadString(&data, &reason);
	Game_Disconnect(&title, &reason);
}

static void Classic_SetPermission(uint8_t* data) {
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	HacksComp_SetUserType(hacks, *data, !cpe_blockPerms);
	HacksComp_UpdateState(hacks);
}

static void Classic_ReadAbsoluteLocation(uint8_t* data, EntityID id, bool interpolate) {
	int x, y, z;
	float rotY, headX;
	struct LocationUpdate update;

	if (cpe_extEntityPos) {
		x = (int)Stream_GetU32_BE(&data[0]);
		y = (int)Stream_GetU32_BE(&data[4]);
		z = (int)Stream_GetU32_BE(&data[8]);
		data += 12;
	} else {
		x = (int16_t)Stream_GetU16_BE(&data[0]);
		y = (int16_t)Stream_GetU16_BE(&data[2]);
		z = (int16_t)Stream_GetU16_BE(&data[4]);
		data += 6;
	}

	y -= 51; /* Convert to feet position */
	if (id == ENTITIES_SELF_ID) y += 22;

	Vector3 pos = { x/32.0f, y/32.0f, z/32.0f };
	rotY  = Math_Packed2Deg(*data++);
	headX = Math_Packed2Deg(*data++);

	if (id == ENTITIES_SELF_ID) classic_receivedFirstPos = true;
	LocationUpdate_MakePosAndOri(&update, pos, rotY, headX, false);
	Handlers_UpdateLocation(id, &update, interpolate);
}

static void Classic_Reset(void) {
	map_begunLoading = false;
	classic_receivedFirstPos = false;

	Net_Set(OPCODE_HANDSHAKE, Classic_Handshake, 131);
	Net_Set(OPCODE_PING, Classic_Ping, 1);
	Net_Set(OPCODE_LEVEL_BEGIN, Classic_LevelInit, 1);
	Net_Set(OPCODE_LEVEL_DATA, Classic_LevelDataChunk, 1028);
	Net_Set(OPCODE_LEVEL_END, Classic_LevelFinalise, 7);
	Net_Set(OPCODE_SET_BLOCK, Classic_SetBlock, 8);

	Net_Set(OPCODE_ADD_ENTITY, Classic_AddEntity, 74);
	Net_Set(OPCODE_ENTITY_TELEPORT, Classic_EntityTeleport, 10);
	Net_Set(OPCODE_RELPOS_AND_ORI_UPDATE, Classic_RelPosAndOrientationUpdate, 7);
	Net_Set(OPCODE_RELPOS_UPDATE, Classic_RelPositionUpdate, 5);
	Net_Set(OPCODE_ORI_UPDATE, Classic_OrientationUpdate, 4);
	Net_Set(OPCODE_REMOVE_ENTITY, Classic_RemoveEntity, 2);

	Net_Set(OPCODE_MESSAGE, Classic_Message, 66);
	Net_Set(OPCODE_KICK, Classic_Kick, 65);
	Net_Set(OPCODE_SET_PERMISSION, Classic_SetPermission, 2);
}

static void Classic_Tick(void) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	if (!classic_receivedFirstPos) return;
	Classic_WritePosition(p->Position, p->HeadY, p->HeadX);
}


/*########################################################################################################################*
*------------------------------------------------------CPE protocol-------------------------------------------------------*
*#########################################################################################################################*/
const char* cpe_clientExtensions[30] = {
	"ClickDistance", "CustomBlocks", "HeldBlock", "EmoteFix", "TextHotKey", "ExtPlayerList",
	"EnvColors", "SelectionCuboid", "BlockPermissions", "ChangeModel", "EnvMapAppearance",
	"EnvWeatherType", "MessageTypes", "HackControl", "PlayerClick", "FullCP437", "LongerMessages",
	"BlockDefinitions", "BlockDefinitionsExt", "BulkBlockUpdate", "TextColors", "EnvMapAspect",
	"EntityProperty", "ExtEntityPositions", "TwoWayPing", "InventoryOrder", "InstantMOTD", "FastMap",
	"ExtendedTextures", "ExtendedBlocks",
};
static void CPE_SetMapEnvUrl(uint8_t* data);

#define Ext_Deg2Packed(x) ((int16_t)((x) * 65536.0f / 360.0f))
void CPE_WritePlayerClick(MouseButton button, bool buttonDown, uint8_t targetId, struct PickedPos* pos) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	uint8_t* data = ServerConnection_WriteBuffer;
	*data++ = OPCODE_PLAYER_CLICK;
	{
		*data++ = button;
		*data++ = buttonDown;
		Stream_SetU16_BE(data, Ext_Deg2Packed(p->HeadY)); data += 2;
		Stream_SetU16_BE(data, Ext_Deg2Packed(p->HeadX)); data += 2;

		*data++ = targetId;
		Stream_SetU16_BE(data, pos->BlockPos.X); data += 2;
		Stream_SetU16_BE(data, pos->BlockPos.Y); data += 2;
		Stream_SetU16_BE(data, pos->BlockPos.Z); data += 2;

		*data = 255;
		/* Our own face values differ from CPE block face */
		switch (pos->ClosestFace) {
		case FACE_XMAX: *data = 0; break;
		case FACE_XMIN: *data = 1; break;
		case FACE_YMAX: *data = 2; break;
		case FACE_YMIN: *data = 3; break;
		case FACE_ZMAX: *data = 4; break;
		case FACE_ZMIN: *data = 5; break;
		}
		data++;
	}
	ServerConnection_WriteBuffer += 15;
}

static void CPE_WriteExtInfo(const String* appName, int extensionsCount) {
	uint8_t* data = ServerConnection_WriteBuffer; 
	*data++ = OPCODE_EXT_INFO;
	{
		Handlers_WriteString(data, appName);     data += STRING_SIZE;
		Stream_SetU16_BE(data, extensionsCount); data += 2;
	}
	ServerConnection_WriteBuffer = data;
}

static void CPE_WriteExtEntry(const String* extensionName, int extensionVersion) {
	uint8_t* data = ServerConnection_WriteBuffer;
	*data++ = OPCODE_EXT_ENTRY;
	{
		Handlers_WriteString(data, extensionName); data += STRING_SIZE;
		Stream_SetU32_BE(data, extensionVersion);  data += 4;
	}
	ServerConnection_WriteBuffer = data;
}

static void CPE_WriteCustomBlockLevel(uint8_t version) {
	uint8_t* data = ServerConnection_WriteBuffer;
	*data++ = OPCODE_CUSTOM_BLOCK_LEVEL;
	{
		*data++ = version;
	}
	ServerConnection_WriteBuffer = data;
}

static void CPE_WriteTwoWayPing(bool serverToClient, int payload) {
	uint8_t* data = ServerConnection_WriteBuffer; 
	*data++ = OPCODE_TWO_WAY_PING;
	{
		*data++ = serverToClient;
		Stream_SetU16_BE(data, payload); data += 2;
	}
	ServerConnection_WriteBuffer = data;
}

static void CPE_SendCpeExtInfoReply(void) {
	int count = Array_Elems(cpe_clientExtensions);
	String name;
	int i, ver;

	if (cpe_serverExtensionsCount) return;
	
#ifndef EXTENDED_TEXTURES
	count--;
#endif
#ifndef EXTENDED_BLOCKS
	count--;
#endif

#ifdef EXTENDED_BLOCKS
	if (!Game_AllowCustomBlocks) count -= 3;
#else
	if (!Game_AllowCustomBlocks) count -= 2;
#endif

	CPE_WriteExtInfo(&ServerConnection_AppName, count);
	Net_SendPacket();

	for (i = 0; i < Array_Elems(cpe_clientExtensions); i++) {
		name = String_FromReadonly(cpe_clientExtensions[i]);
		ver = 1;

		if (String_CaselessEqualsConst(&name, "ExtPlayerList"))       ver = 2;
		if (String_CaselessEqualsConst(&name, "EnvMapAppearance"))    ver = cpe_envMapVer;
		if (String_CaselessEqualsConst(&name, "BlockDefinitionsExt")) ver = cpe_blockDefsExtVer;

		if (!Game_AllowCustomBlocks) {
			if (String_CaselessEqualsConst(&name, "BlockDefinitionsExt")) continue;
			if (String_CaselessEqualsConst(&name, "BlockDefinitions"))    continue;
#ifdef EXTENDED_BLOCKS
			if (String_CaselessEqualsConst(&name, "ExtendedBlocks"))      continue;
#endif
		}

#ifndef EXTENDED_TEXTURES
		if (String_CaselessEqualsConst(&name, "ExtendedTextures")) continue;
#endif
#ifndef EXTENDED_BLOCKS
		if (String_CaselessEqualsConst(&name, "ExtendedBlocks")) continue;
#endif

		CPE_WriteExtEntry(&name, ver);
		Net_SendPacket();
	}
}

static void CPE_ExtInfo(uint8_t* data) {
	static String d3Server = String_FromConst("D3 server");
	char appNameBuffer[STRING_SIZE];
	String appName = String_FromArray(appNameBuffer);

	Handlers_ReadString(&data, &appName);
	Chat_Add1("Server software: %s", &appName);
	cpe_needD3Fix = String_CaselessStarts(&appName, &d3Server);

	/* Workaround for old MCGalaxy that send ExtEntry sync but ExtInfo async. This means
	   ExtEntry may sometimes arrive before ExtInfo, thus have to use += instead of = */
	cpe_serverExtensionsCount += Stream_GetU16_BE(data);
	CPE_SendCpeExtInfoReply();
}

static void CPE_ExtEntry(uint8_t* data) {
	char extNameBuffer[STRING_SIZE];
	String ext = String_FromArray(extNameBuffer);
	int extVersion;

	Handlers_ReadString(&data, &ext);
	extVersion = Stream_GetU32_BE(data);
	Platform_Log2("cpe ext: %s, %i", &ext, &extVersion);

	cpe_serverExtensionsCount--;
	CPE_SendCpeExtInfoReply();	

	/* update support state */
	if (String_CaselessEqualsConst(&ext, "HeldBlock")) {
		cpe_sendHeldBlock = true;
	} else if (String_CaselessEqualsConst(&ext, "MessageTypes")) {
		cpe_useMessageTypes = true;
	} else if (String_CaselessEqualsConst(&ext, "ExtPlayerList")) {
		ServerConnection_SupportsExtPlayerList = true;
	} else if (String_CaselessEqualsConst(&ext, "BlockPermissions")) {
		cpe_blockPerms = true;
	} else if (String_CaselessEqualsConst(&ext, "PlayerClick")) {
		ServerConnection_SupportsPlayerClick = true;
	} else if (String_CaselessEqualsConst(&ext, "EnvMapAppearance")) {
		cpe_envMapVer = extVersion;
		if (extVersion == 1) return;
		Net_PacketSizes[OPCODE_ENV_SET_MAP_APPEARANCE] += 4;
	} else if (String_CaselessEqualsConst(&ext, "LongerMessages")) {
		ServerConnection_SupportsPartialMessages = true;
	} else if (String_CaselessEqualsConst(&ext, "FullCP437")) {
		ServerConnection_SupportsFullCP437 = true;
	} else if (String_CaselessEqualsConst(&ext, "BlockDefinitionsExt")) {
		cpe_blockDefsExtVer = extVersion;
		if (extVersion == 1) return;
		Net_PacketSizes[OPCODE_DEFINE_BLOCK_EXT] += 3;
	} else if (String_CaselessEqualsConst(&ext, "ExtEntityPositions")) {
		Net_PacketSizes[OPCODE_ENTITY_TELEPORT] += 6;
		Net_PacketSizes[OPCODE_ADD_ENTITY]      += 6;
		Net_PacketSizes[OPCODE_EXT_ADD_ENTITY2] += 6;
		cpe_extEntityPos = true;
	} else if (String_CaselessEqualsConst(&ext, "TwoWayPing")) {
		cpe_twoWayPing = true;
	} else if (String_CaselessEqualsConst(&ext, "FastMap")) {
		Net_PacketSizes[OPCODE_LEVEL_BEGIN] += 4;
		cpe_fastMap = true;
	}
#ifdef EXTENDED_TEXTURES
	else if (String_CaselessEqualsConst(&ext, "ExtendedTextures")) {
		Net_PacketSizes[OPCODE_DEFINE_BLOCK]     += 3;
		Net_PacketSizes[OPCODE_DEFINE_BLOCK_EXT] += 6;
		cpe_extTextures = true;
	}
#endif
#ifdef EXTENDED_BLOCKS
	else if (String_CaselessEqualsConst(&ext, "ExtendedBlocks")) {
		if (!Game_AllowCustomBlocks) return;
		cpe_extBlocks = true;

		Net_PacketSizes[OPCODE_SET_BLOCK] += 1;
		Net_PacketSizes[OPCODE_HOLD_THIS] += 1;
		Net_PacketSizes[OPCODE_SET_BLOCK_PERMISSION] += 1;
		Net_PacketSizes[OPCODE_DEFINE_BLOCK]     += 1;
		Net_PacketSizes[OPCODE_UNDEFINE_BLOCK]   += 1;
		Net_PacketSizes[OPCODE_DEFINE_BLOCK_EXT] += 1;
		Net_PacketSizes[OPCODE_SET_INVENTORY_ORDER] += 2;
		Net_PacketSizes[OPCODE_BULK_BLOCK_UPDATE]   += 256 / 4;
	}
#endif
}

static void CPE_SetClickDistance(uint8_t* data) {
	LocalPlayer_Instance.ReachDistance = Stream_GetU16_BE(data) / 32.0f;
}

static void CPE_CustomBlockLevel(uint8_t* data) {
	CPE_WriteCustomBlockLevel(1);
	Net_SendPacket();
	Game_UseCPEBlocks = true;
	Event_RaiseVoid(&BlockEvents_PermissionsChanged);
}

static void CPE_HoldThis(uint8_t* data) {
	BlockID block;
	bool canChange;

	Handlers_ReadBlock(data, block);
	canChange = *data == 0;

	Inventory_CanChangeHeldBlock = true;
	Inventory_SetSelectedBlock(block);
	Inventory_CanChangeHeldBlock = canChange;
	Inventory_CanPick = block != BLOCK_AIR;
}

static void CPE_SetTextHotkey(uint8_t* data) {
	char actionBuffer[STRING_SIZE];
	String action = String_FromArray(actionBuffer);
	
	data += STRING_SIZE; /* skip label */
	Handlers_ReadString(&data, &action);

	uint32_t keyCode  = Stream_GetU32_BE(data); data += 4;
	uint8_t keyMods = *data;
	if (keyCode > 255) return;

	Key key = Hotkeys_LWJGL[keyCode];
	if (key == Key_None) return;
	Platform_Log3("CPE hotkey added: %c, %b: %s", Key_Names[key], &keyMods, &action);

	if (!action.length) {
		Hotkeys_Remove(key, keyMods);
	} else if (action.buffer[action.length - 1] == '\n') {
		action.length--;
		Hotkeys_Add(key, keyMods, &action, false);
	} else { /* more input needed by user */
		Hotkeys_Add(key, keyMods, &action, true);
	}
}

static void CPE_ExtAddPlayerName(uint8_t* data) {
	char playerNameBuffer[STRING_SIZE];
	char listNameBuffer[STRING_SIZE];
	char groupNameBuffer[STRING_SIZE];

	String playerName = String_FromArray(playerNameBuffer);
	String listName   = String_FromArray(listNameBuffer);
	String groupName  = String_FromArray(groupNameBuffer);

	EntityID id = data[1]; data += 2;
	Handlers_ReadString(&data, &playerName);
	Handlers_ReadString(&data, &listName);
	Handlers_ReadString(&data, &groupName);
	uint8_t groupRank = *data;

	String_StripCols(&playerName);
	Handlers_RemoveEndPlus(&playerName);
	Handlers_RemoveEndPlus(&listName);

	/* Workarond for server software that declares support for ExtPlayerList, but sends AddEntity then AddPlayerName */
	int mask = id >> 3, bit = 1 << (id & 0x7);
	classic_tabList[mask] &= (uint8_t)~bit;
	Handlers_AddTablistEntry(id, &playerName, &listName, &groupName, groupRank);
}

static void CPE_ExtAddEntity(uint8_t* data) {
	char displayNameBuffer[STRING_SIZE];
	char skinNameBuffer[STRING_SIZE];
	String displayName = String_FromArray(displayNameBuffer);
	String skinName    = String_FromArray(skinNameBuffer);

	EntityID id = *data++;
	Handlers_ReadString(&data, &displayName);
	Handlers_ReadString(&data, &skinName);

	Handlers_CheckName(id, &displayName, &skinName);
	Handlers_AddEntity(data, id, &displayName, &skinName, false);
}

static void CPE_ExtRemovePlayerName(uint8_t* data) {
	EntityID id = data[1];
	Handlers_RemoveTablistEntry(id);
}

static void CPE_MakeSelection(uint8_t* data) {
	uint8_t selectionId;
	Vector3I p1, p2;
	PackedCol c;

	selectionId = *data++;
	data += STRING_SIZE; /* label */

	p1.X = (int16_t)Stream_GetU16_BE(&data[0]);
	p1.Y = (int16_t)Stream_GetU16_BE(&data[2]);
	p1.Z = (int16_t)Stream_GetU16_BE(&data[4]);
	data += 6;

	p2.X = (int16_t)Stream_GetU16_BE(&data[0]);
	p2.Y = (int16_t)Stream_GetU16_BE(&data[2]);
	p2.Z = (int16_t)Stream_GetU16_BE(&data[4]);
	data += 6;

	/* R,G,B,A are actually 16 bit unsigned integers */
	c.R = data[1]; c.G = data[3]; c.B = data[5]; c.A = data[7];
	Selections_Add(selectionId, p1, p2, c);
}

static void CPE_RemoveSelection(uint8_t* data) {
	Selections_Remove(*data);
}

static void CPE_SetEnvCol(uint8_t* data) {
	uint8_t variable = *data++;
	int r = Stream_GetU16_BE(&data[0]);
	int g = Stream_GetU16_BE(&data[2]);
	int b = Stream_GetU16_BE(&data[4]);
	bool invalid = r > 255 || g > 255 || b > 255;
	PackedCol col = PACKEDCOL_CONST((uint8_t)r, (uint8_t)g, (uint8_t)b, 255);

	if (variable == 0) {
		Env_SetSkyCol(invalid ? Env_DefaultSkyCol : col);
	} else if (variable == 1) {
		Env_SetCloudsCol(invalid ? Env_DefaultCloudsCol : col);
	} else if (variable == 2) {
		Env_SetFogCol(invalid ? Env_DefaultFogCol : col);
	} else if (variable == 3) {
		Env_SetShadowCol(invalid ? Env_DefaultShadowCol : col);
	} else if (variable == 4) {
		Env_SetSunCol(invalid ? Env_DefaultSunCol : col);
	}
}

static void CPE_SetBlockPermission(uint8_t* data) {
	BlockID block; 
	Handlers_ReadBlock(data, block);

	Block_CanPlace[block]  = *data++ != 0;
	Block_CanDelete[block] = *data++ != 0;
	Event_RaiseVoid(&BlockEvents_PermissionsChanged);
}

static void CPE_ChangeModel(uint8_t* data) {
	char modelBuffer[STRING_SIZE];
	String model = String_FromArray(modelBuffer);
	EntityID id  = *data++;
	Handlers_ReadString(&data, &model);

	struct Entity* entity = Entities_List[id];
	if (entity) { Entity_SetModel(entity, &model); }
}

static void CPE_EnvSetMapAppearance(uint8_t* data) {
	CPE_SetMapEnvUrl(data);
	Env_SetSidesBlock(data[64]);
	Env_SetEdgeBlock(data[65]);
	Env_SetEdgeHeight((int16_t)Stream_GetU16_BE(&data[66]));
	if (cpe_envMapVer == 1) return;

	/* Version 2 */
	Env_SetCloudsHeight((int16_t)Stream_GetU16_BE(&data[68]));
	int maxViewDist   = (int16_t)Stream_GetU16_BE(&data[70]);
	Game_MaxViewDistance = maxViewDist <= 0 ? 32768 : maxViewDist;
	Game_SetViewDistance(Game_UserViewDistance);
}

static void CPE_EnvWeatherType(uint8_t* data) {
	Env_SetWeather(*data);
}

static void CPE_HackControl(uint8_t* data) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	p->Hacks.CanFly                  = *data++ != 0;
	p->Hacks.CanNoclip               = *data++ != 0;
	p->Hacks.CanSpeed                = *data++ != 0;
	p->Hacks.CanRespawn              = *data++ != 0;
	p->Hacks.CanUseThirdPersonCamera = *data++ != 0;
	LocalPlayer_CheckHacksConsistency();

	int jumpHeight = Stream_GetU16_BE(data);
	struct PhysicsComp* physics = &p->Physics;
	if (jumpHeight == UInt16_MaxValue) { /* special value of -1 to reset default */
		physics->JumpVel = HacksComp_CanJumpHigher(&p->Hacks) ? physics->UserJumpVel : 0.42f;
	} else {
		PhysicsComp_CalculateJumpVelocity(physics, jumpHeight / 32.0f);
	}

	physics->ServerJumpVel = physics->JumpVel;
	Event_RaiseVoid(&UserEvents_HackPermissionsChanged);
}

static void CPE_ExtAddEntity2(uint8_t* data) {
	char displayNameBuffer[STRING_SIZE];
	char skinNameBuffer[STRING_SIZE];
	String displayName = String_FromArray(displayNameBuffer);
	String skinName    = String_FromArray(skinNameBuffer);

	EntityID id = *data++;
	Handlers_ReadString(&data, &displayName);
	Handlers_ReadString(&data, &skinName);

	Handlers_CheckName(id, &displayName, &skinName);
	Handlers_AddEntity(data, id, &displayName, &skinName, true);
}

#define BULK_MAX_BLOCKS 256
static void CPE_BulkBlockUpdate(uint8_t* data) {
	int32_t indices[BULK_MAX_BLOCKS];
	BlockID blocks[BULK_MAX_BLOCKS];
	int i, count = 1 + *data++;
	
	for (i = 0; i < count; i++) {
		indices[i] = Stream_GetU32_BE(data); data += 4;
	}
	data += (BULK_MAX_BLOCKS - count) * 4;
	
	for (i = 0; i < count; i++) {
		blocks[i] = data[i];
	}
	data += BULK_MAX_BLOCKS;

	if (cpe_extBlocks) {
		for (i = 0; i < count; i += 4) {
			uint8_t flags = data[i >> 2];
			blocks[i + 0] |= (BlockID)((flags & 0x03) << 8);
			blocks[i + 1] |= (BlockID)((flags & 0x0C) << 6);
			blocks[i + 2] |= (BlockID)((flags & 0x30) << 4);
			blocks[i + 3] |= (BlockID)((flags & 0xC0) << 2);
		}
		data += BULK_MAX_BLOCKS / 4;
	}

	int x, y, z;
	for (i = 0; i < count; i++) {
		int index = indices[i];
		if (index < 0 || index >= World_BlocksSize) continue;
		World_Unpack(index, x, y, z);

		if (World_IsValidPos(x, y, z)) {
			Game_UpdateBlock(x, y, z, blocks[i]);
		}
	}
}

static void CPE_SetTextColor(uint8_t* data) {
	PackedCol c;
	c.R = *data++; c.G = *data++; c.B = *data++; c.A = *data++;

	uint8_t code = *data;
	/* disallow space, null, and colour code specifiers */
	if (code == '\0' || code == ' ' || code == 0xFF) return;
	if (code == '%' || code == '&') return;

	Drawer2D_Cols[code] = c;
	Event_RaiseInt(&ChatEvents_ColCodeChanged, code);
}

static void CPE_SetMapEnvUrl(uint8_t* data) {
	char urlBuffer[STRING_SIZE];
	String url = String_FromArray(urlBuffer);
	Handlers_ReadString(&data, &url);
	if (!Game_AllowServerTextures) return;

	if (!url.length) {
		/* don't extract default texture pack if we can */
		if (World_TextureUrl.length) TexturePack_ExtractDefault();
	} else if (Utils_IsUrlPrefix(&url, 0)) {
		ServerConnection_RetrieveTexturePack(&url);
	}
	Platform_Log1("Image url: %s", &url);
}

static void CPE_SetMapEnvProperty(uint8_t* data) {
	uint8_t type = *data++;
	int value    = (int)Stream_GetU32_BE(data);
	Math_Clamp(value, -0xFFFFFF, 0xFFFFFF);

	switch (type) {
	case 0:
		Math_Clamp(value, 0, BLOCK_MAX_DEFINED);
		Env_SetSidesBlock((BlockID)value); break;
	case 1:
		Math_Clamp(value, 0, BLOCK_MAX_DEFINED);
		Env_SetEdgeBlock((BlockID)value); break;
	case 2:
		Env_SetEdgeHeight(value); break;
	case 3:
		Env_SetCloudsHeight(value); break;
	case 4:
		Math_Clamp(value, -0x7FFF, 0x7FFF);
		Game_MaxViewDistance = value <= 0 ? 32768 : value;
		Game_SetViewDistance(Game_UserViewDistance); break;
	case 5:
		Env_SetCloudsSpeed(value / 256.0f); break;
	case 6:
		Env_SetWeatherSpeed(value / 256.0f); break;
	case 7:
		Math_Clamp(value, 0, UInt8_MaxValue);
		Env_SetWeatherFade(value / 128.0f); break;
	case 8:
		Env_SetExpFog(value != 0); break;
	case 9:
		Env_SetSidesOffset(value); break;
	case 10:
		Env_SetSkyboxHorSpeed(value / 1024.0f); break;
	case 11:
		Env_SetSkyboxVerSpeed(value / 1024.0f); break;
	}
}

static void CPE_SetEntityProperty(uint8_t* data) {
	EntityID id  = *data++;
	uint8_t type = *data++;
	int value    = (int)Stream_GetU32_BE(data);

	struct Entity* entity = Entities_List[id];
	if (!entity) return;
	struct LocationUpdate update = { 0 };

	float scale;
	switch (type) {
	case 0:
		update.Flags |= LOCATIONUPDATE_FLAG_ROTX;
		update.RotX = LocationUpdate_Clamp((float)value); break;
	case 1:
		update.Flags |= LOCATIONUPDATE_FLAG_HEADY;
		update.HeadY = LocationUpdate_Clamp((float)value); break;
	case 2:
		update.Flags |= LOCATIONUPDATE_FLAG_ROTZ;
		update.RotZ = LocationUpdate_Clamp((float)value); break;

	case 3:
	case 4:
	case 5:
		scale = value / 1000.0f;
		Math_Clamp(scale, 0.01f, entity->Model->MaxScale);
		if (type == 3) entity->ModelScale.X = scale;
		if (type == 4) entity->ModelScale.Y = scale;
		if (type == 5) entity->ModelScale.Z = scale;

		Entity_UpdateModelBounds(entity);
		return;
	default:
		return;
	}
	entity->VTABLE->SetLocation(entity, &update, true);
}

static void CPE_TwoWayPing(uint8_t* data) {
	uint8_t serverToClient = *data++;
	int payload = Stream_GetU16_BE(data);

	if (serverToClient) {
		CPE_WriteTwoWayPing(true, payload); /* server to client reply */
		Net_SendPacket();
	} else { PingList_Update(payload); }
}

static void CPE_SetInventoryOrder(uint8_t* data) {
	BlockID block, order;
	Handlers_ReadBlock(data, block);
	Handlers_ReadBlock(data, order);

	Inventory_Remove(block);
	if (order) { Inventory_Map[order - 1] = block; }
}

static void CPE_Reset(void) {
	cpe_serverExtensionsCount = 0; cpe_pingTicks = 0;
	cpe_sendHeldBlock = false; cpe_useMessageTypes = false;
	cpe_envMapVer = 2; cpe_blockDefsExtVer = 2;
	cpe_needD3Fix = false; cpe_extEntityPos = false; cpe_twoWayPing = false; 
	cpe_extTextures = false; cpe_fastMap = false; cpe_extBlocks = false;
	Game_UseCPEBlocks = false;
	if (!Game_UseCPE) return;

	Net_Set(OPCODE_EXT_INFO, CPE_ExtInfo, 67);
	Net_Set(OPCODE_EXT_ENTRY, CPE_ExtEntry, 69);
	Net_Set(OPCODE_SET_REACH, CPE_SetClickDistance, 3);
	Net_Set(OPCODE_CUSTOM_BLOCK_LEVEL, CPE_CustomBlockLevel, 2);
	Net_Set(OPCODE_HOLD_THIS, CPE_HoldThis, 3);
	Net_Set(OPCODE_SET_TEXT_HOTKEY, CPE_SetTextHotkey, 134);

	Net_Set(OPCODE_EXT_ADD_PLAYER_NAME, CPE_ExtAddPlayerName, 196);
	Net_Set(OPCODE_EXT_ADD_ENTITY, CPE_ExtAddEntity, 130);
	Net_Set(OPCODE_EXT_REMOVE_PLAYER_NAME, CPE_ExtRemovePlayerName, 3);

	Net_Set(OPCODE_ENV_SET_COLOR, CPE_SetEnvCol, 8);
	Net_Set(OPCODE_MAKE_SELECTION, CPE_MakeSelection, 86);
	Net_Set(OPCODE_REMOVE_SELECTION, CPE_RemoveSelection, 2);
	Net_Set(OPCODE_SET_BLOCK_PERMISSION, CPE_SetBlockPermission, 4);
	Net_Set(OPCODE_SET_MODEL, CPE_ChangeModel, 66);
	Net_Set(OPCODE_ENV_SET_MAP_APPEARANCE, CPE_EnvSetMapAppearance, 69);
	Net_Set(OPCODE_ENV_SET_WEATHER, CPE_EnvWeatherType, 2);
	Net_Set(OPCODE_HACK_CONTROL, CPE_HackControl, 8);
	Net_Set(OPCODE_EXT_ADD_ENTITY2, CPE_ExtAddEntity2, 138);

	Net_Set(OPCODE_BULK_BLOCK_UPDATE, CPE_BulkBlockUpdate, 1282);
	Net_Set(OPCODE_SET_TEXT_COLOR, CPE_SetTextColor, 6);
	Net_Set(OPCODE_ENV_SET_MAP_URL, CPE_SetMapEnvUrl, 65);
	Net_Set(OPCODE_ENV_SET_MAP_PROPERTY, CPE_SetMapEnvProperty, 6);
	Net_Set(OPCODE_SET_ENTITY_PROPERTY, CPE_SetEntityProperty, 7);
	Net_Set(OPCODE_TWO_WAY_PING, CPE_TwoWayPing, 4);
	Net_Set(OPCODE_SET_INVENTORY_ORDER, CPE_SetInventoryOrder, 3);
}

static void CPE_Tick(void) {
	cpe_pingTicks++;
	if (cpe_pingTicks >= 20 && cpe_twoWayPing) {
		CPE_WriteTwoWayPing(false, PingList_NextPingData());
		cpe_pingTicks = 0;
	}
}


/*########################################################################################################################*
*------------------------------------------------------Custom blocks------------------------------------------------------*
*#########################################################################################################################*/
static void BlockDefs_OnBlockUpdated(BlockID block, bool didBlockLight) {
	if (!World_Blocks) return;
	/* Need to refresh lighting when a block's light blocking state changes */
	if (Block_BlocksLight[block] != didBlockLight) { Lighting_Refresh(); }
}

static TextureLoc BlockDefs_Tex(uint8_t** ptr) {
	TextureLoc loc; uint8_t* data = *ptr;

	if (!cpe_extTextures) {
		loc = *data++;
	} else {
		loc = Stream_GetU16_BE(data) % ATLAS1D_MAX_ATLASES; data += 2;
	}

	*ptr = data; return loc;
}

static BlockID BlockDefs_DefineBlockCommonStart(uint8_t** ptr, bool uniqueSideTexs) {
	BlockID block;
	bool didBlockLight;
	uint8_t sound;
	uint8_t* data = *ptr;

	char nameBuffer[STRING_SIZE];
	String name = String_FromArray(nameBuffer);

	Handlers_ReadBlock(data, block);
	didBlockLight = Block_BlocksLight[block];
	Block_ResetProps(block);
	
	Handlers_ReadString(&data, &name);
	Block_SetName(block, &name);
	Block_SetCollide(block, *data++);

	float multiplierExponent = (*data++ - 128) / 64.0f;
	#define LOG_2 0.693147180559945
	Block_SpeedMultiplier[block] = (float)Math_Exp(LOG_2 * multiplierExponent); /* pow(2, x) */

	Block_SetTex(BlockDefs_Tex(&data), FACE_YMAX, block);
	if (uniqueSideTexs) {
		Block_SetTex(BlockDefs_Tex(&data), FACE_XMIN, block);
		Block_SetTex(BlockDefs_Tex(&data), FACE_XMAX, block);
		Block_SetTex(BlockDefs_Tex(&data), FACE_ZMIN, block);
		Block_SetTex(BlockDefs_Tex(&data), FACE_ZMAX, block);
	} else {
		Block_SetSide(BlockDefs_Tex(&data), block);
	}
	Block_SetTex(BlockDefs_Tex(&data), FACE_YMIN, block);

	Block_BlocksLight[block] = *data++ == 0;
	BlockDefs_OnBlockUpdated(block, didBlockLight);

	sound = *data++;
	Block_StepSounds[block] = sound;
	Block_DigSounds[block]  = sound;
	if (sound == SOUND_GLASS) Block_StepSounds[block] = SOUND_STONE;

	Block_FullBright[block] = *data++ != 0;
	*ptr = data;
	return block;
}

static void BlockDefs_DefineBlockCommonEnd(uint8_t* data, uint8_t shape, BlockID block) {
	uint8_t blockDraw;
	uint8_t density;
	PackedCol c;

	blockDraw = *data++;
	if (shape == 0) {
		Block_SpriteOffset[block] = blockDraw;
		blockDraw = DRAW_SPRITE;
	}
	Block_Draw[block] = blockDraw;

	density = *data++;
	Block_FogDensity[block] = density == 0 ? 0.0f : (density + 1) / 128.0f;

	c.R = *data++; c.G = *data++; c.B = *data++; c.A = 255;
	Block_FogCol[block] = c;
	Block_DefineCustom(block);
}

static void BlockDefs_DefineBlock(uint8_t* data) {
	BlockID block = BlockDefs_DefineBlockCommonStart(&data, false);

	uint8_t shape = *data++;
	if (shape > 0 && shape <= 16) {
		Block_MaxBB[block].Y = shape / 16.0f;
	}

	BlockDefs_DefineBlockCommonEnd(data, shape, block);
	/* Update sprite BoundingBox if necessary */
	if (Block_Draw[block] == DRAW_SPRITE) {
		Block_RecalculateBB(block);
	}
}

static void BlockDefs_UndefineBlock(uint8_t* data) {
	BlockID block;
	bool didBlockLight;

	Handlers_ReadBlock(data, block);
	didBlockLight = Block_BlocksLight[block];

	Block_ResetProps(block);
	BlockDefs_OnBlockUpdated(block, didBlockLight);
	Block_UpdateCulling(block);

	Inventory_Remove(block);
	if (block < BLOCK_CPE_COUNT) { Inventory_AddDefault(block); }

	Block_SetCustomDefined(block, false);
	Event_RaiseVoid(&BlockEvents_BlockDefChanged);
}

static void BlockDefs_DefineBlockExt(uint8_t* data) {
	Vector3 minBB, maxBB;
	BlockID block = BlockDefs_DefineBlockCommonStart(&data, cpe_blockDefsExtVer >= 2);

	minBB.X = (int8_t)(*data++) / 16.0f;
	minBB.Y = (int8_t)(*data++) / 16.0f;
	minBB.Z = (int8_t)(*data++) / 16.0f;

	maxBB.X = (int8_t)(*data++) / 16.0f;
	maxBB.Y = (int8_t)(*data++) / 16.0f;
	maxBB.Z = (int8_t)(*data++) / 16.0f;

	Block_MinBB[block] = minBB;
	Block_MaxBB[block] = maxBB;
	BlockDefs_DefineBlockCommonEnd(data, 1, block);
}

#if 0
void HandleDefineModel(uint8_t* data) {
	int start = reader.index - 1;
	uint8_t id   = *data++;
	uint8_t type = *data++;
	CustomModel model = null;

	switch (type) {
	case 0:
		model = new CustomModel(game);
		model.ReadSetupPacket(reader);
		game.ModelCache.CustomModels[id] = model;
		break;
	case 1:
		model = game.ModelCache.CustomModels[id];
		if (model != null) model.ReadMetadataPacket(reader);
		break;
	case 2:
		model = game.ModelCache.CustomModels[id];
		if (model != null) model.ReadDefinePartPacket(reader);
		break;
	case 3:
		model = game.ModelCache.CustomModels[id];
		if (model != null) model.ReadRotationPacket(reader);
		break;
	}
	int total = packetSizes[(byte)Opcode.CpeDefineModel];
	reader.Skip(total - (reader.index - start));
}
#endif
static void BlockDefs_Reset(void) {
	if (!Game_UseCPE || !Game_AllowCustomBlocks) return;
	Net_Set(OPCODE_DEFINE_BLOCK,     BlockDefs_DefineBlock,    80);
	Net_Set(OPCODE_UNDEFINE_BLOCK,   BlockDefs_UndefineBlock,  2);
	Net_Set(OPCODE_DEFINE_BLOCK_EXT, BlockDefs_DefineBlockExt, 85);
}


/*########################################################################################################################*
*-----------------------------------------------------Public handlers-----------------------------------------------------*
*#########################################################################################################################*/
void Handlers_Reset(void) {
	Classic_Reset();
	CPE_Reset();
	BlockDefs_Reset();
	WoM_Reset();
}

void Handlers_Tick(void) {
	Classic_Tick();
	CPE_Tick();
	WoM_Tick();
}
