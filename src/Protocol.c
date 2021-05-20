#include "Protocol.h"
#include "String.h"
#include "Deflate.h"
#include "Server.h"
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
#include "Http.h"
#include "Drawer2D.h"
#include "Logger.h"
#include "TexturePack.h"
#include "Gui.h"
#include "Errors.h"
#include "Camera.h"
#include "Window.h"
#include "Particle.h"
#include "Picking.h"
#include "Input.h"
#include "Utils.h"

#define QUOTE(x) #x
#define STRINGIFY(val) QUOTE(val)
struct _ProtocolData Protocol;

/* Classic state */
static cc_uint8 classic_tabList[ENTITIES_MAX_COUNT >> 3];
static cc_bool classic_receivedFirstPos;

/* Map state */
static cc_bool map_begunLoading;
static cc_uint64 map_receiveBeg;
static struct Stream map_part;
static struct GZipHeader map_gzHeader;
static int map_sizeIndex, map_volume;
static cc_uint8 map_size[4];

struct MapState {
	struct InflateState inflateState;
	struct Stream stream;
	BlockRaw* blocks;
	int index;
	cc_bool allocFailed;
};
static struct MapState map;
#ifdef EXTENDED_BLOCKS
static struct MapState map2;
#endif

/* CPE state */
cc_bool cpe_needD3Fix;
static int cpe_serverExtensionsCount, cpe_pingTicks;
static int cpe_envMapVer = 2, cpe_blockDefsExtVer = 2, cpe_customModelsVer = 2;
static cc_bool cpe_sendHeldBlock, cpe_useMessageTypes, cpe_extEntityPos, cpe_blockPerms, cpe_fastMap;
static cc_bool cpe_twoWayPing, cpe_extTextures, cpe_extBlocks;

/*########################################################################################################################*
*-----------------------------------------------------Common handlers-----------------------------------------------------*
*#########################################################################################################################*/
#define Classic_TabList_Get(id)   (classic_tabList[id >> 3] & (1 << (id & 0x7)))
#define Classic_TabList_Set(id)   (classic_tabList[id >> 3] |=  (cc_uint8)(1 << (id & 0x7)))
#define Classic_TabList_Reset(id) (classic_tabList[id >> 3] &= (cc_uint8)~(1 << (id & 0x7)))

#ifndef EXTENDED_BLOCKS
#define ReadBlock(data, value) value = *data++;
#else
#define ReadBlock(data, value)\
if (cpe_extBlocks) {\
	value = Stream_GetU16_BE(data) % BLOCK_COUNT; data += 2;\
} else { value = *data++; }
#endif

#ifndef EXTENDED_BLOCKS
#define WriteBlock(data, value) *data++ = value;
#else
#define WriteBlock(data, value)\
if (cpe_extBlocks) {\
	Stream_SetU16_BE(data, value); data += 2;\
} else { *data++ = (BlockRaw)value; }
#endif

static cc_string UNSAFE_GetString(cc_uint8* data) {
	int i, length = 0;
	for (i = STRING_SIZE - 1; i >= 0; i--) {
		char code = data[i];
		if (code == '\0' || code == ' ') continue;
		length = i + 1; break;
	}
	return String_Init((char*)data, length, STRING_SIZE);
}

static float GetFloat(cc_uint8* data) {
	union IntAndFloat raw;
	raw.u = Stream_GetU32_BE(data);
	return raw.f;
}

static void ReadString(cc_uint8** ptr, cc_string* str) {
	int i, length = 0;
	cc_uint8* data = *ptr;
	for (i = STRING_SIZE - 1; i >= 0; i--) {
		char code = data[i];
		if (code == '\0' || code == ' ') continue;
		length = i + 1; break;
	}

	String_AppendAll(str, data, length);
	*ptr = data + STRING_SIZE;
}

static void WriteString(cc_uint8* data, const cc_string* value) {
	int i, count = min(value->length, STRING_SIZE);
	for (i = 0; i < count; i++) {
		char c = value->buffer[i];
		if (c == '&') c = '%'; /* escape colour codes */
		data[i] = c;
	}

	for (; i < STRING_SIZE; i++) { data[i] = ' '; }
}

static void RemoveEndPlus(cc_string* value) {
	/* Workaround for MCDzienny (and others) use a '+' at the end to distinguish classicube.net accounts */
	/* from minecraft.net accounts. Unfortunately they also send this ending + to the client. */
	if (!value->length || value->buffer[value->length - 1] != '+') return;
	value->length--;
}

static void AddTablistEntry(EntityID id, const cc_string* playerName, const cc_string* listName, const cc_string* groupName, cc_uint8 groupRank) {
	cc_string rawName; char rawBuffer[STRING_SIZE];
	String_InitArray(rawName, rawBuffer);

	String_AppendColorless(&rawName, playerName);
	TabList_Set(id, &rawName, listName, groupName, groupRank);
}

static void CheckName(EntityID id, cc_string* name, cc_string* skin) {
	cc_string colorlessName; char colorlessBuffer[STRING_SIZE];

	RemoveEndPlus(name);
	/* Server is only allowed to change our own name colours. */
	if (id == ENTITIES_SELF_ID) {
		String_InitArray(colorlessName, colorlessBuffer);
		String_AppendColorless(&colorlessName, name);
		if (!String_Equals(&colorlessName, &Game_Username)) String_Copy(name, &Game_Username);
	}

	if (!skin->length) String_Copy(skin, name);
	RemoveEndPlus(skin);
}

static void Classic_ReadAbsoluteLocation(cc_uint8* data, EntityID id, cc_bool interpolate);
static void AddEntity(cc_uint8* data, EntityID id, const cc_string* name, const cc_string* skin, cc_bool readPosition) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct Entity* e;

	if (id != ENTITIES_SELF_ID) {
		if (Entities.List[id]) Entities_Remove(id);
		e = &NetPlayers_List[id].Base;

		NetPlayer_Init((struct NetPlayer*)e);
		Entities.List[id] = e;
		Event_RaiseInt(&EntityEvents.Added, id);
	} else {
		e = &LocalPlayer_Instance.Base;
	}
	Entity_SetSkin(e, skin);
	Entity_SetName(e, name);

	if (!readPosition) return;
	Classic_ReadAbsoluteLocation(data, id, false);
	if (id != ENTITIES_SELF_ID) return;

	p->Spawn      = p->Base.Position;
	p->SpawnYaw   = p->Base.Yaw;
	p->SpawnPitch = p->Base.Pitch;
}

void Protocol_RemoveEntity(EntityID id) {
	struct Entity* e = Entities.List[id];
	if (!e) return;
	if (id != ENTITIES_SELF_ID) Entities_Remove(id);

	/* See comment about some servers in Classic_AddEntity */
	if (!Classic_TabList_Get(id)) return;
	TabList_Remove(id);
	Classic_TabList_Reset(id);
}

static void UpdateLocation(EntityID id, struct LocationUpdate* update, cc_bool interpolate) {
	struct Entity* e = Entities.List[id];
	if (e) { e->VTABLE->SetLocation(e, update, interpolate); }
}

static void UpdateUserType(struct HacksComp* hacks, cc_uint8 value) {
	cc_bool isOp = value >= 100 && value <= 127;
	hacks->IsOp  = isOp;
	if (cpe_blockPerms) return;

	Blocks.CanPlace[BLOCK_BEDROCK]     = isOp;
	Blocks.CanDelete[BLOCK_BEDROCK]    = isOp;
	Blocks.CanPlace[BLOCK_WATER]       = isOp;
	Blocks.CanPlace[BLOCK_STILL_WATER] = isOp;
	Blocks.CanPlace[BLOCK_LAVA]        = isOp;
	Blocks.CanPlace[BLOCK_STILL_LAVA]  = isOp;
}


/*########################################################################################################################*
*------------------------------------------------------WoM protocol-------------------------------------------------------*
*#########################################################################################################################*/
/* Partially based on information from http://files.worldofminecraft.com/texturing/ */
/* NOTE: http://files.worldofminecraft.com/ has been down for quite a while, so support was removed on Oct 10, 2015 */
static int wom_identifier;
static cc_bool wom_sendId, wom_sentId;

static void WoM_CheckMotd(void) {
	cc_string url; char urlBuffer[STRING_SIZE];
	cc_string motd, host;
	int index;	

	motd = Server.MOTD;
	if (!motd.length) return;
	index = String_IndexOfConst(&motd, "cfg=");
	if (Game_PureClassic || index == -1) return;
	
	host = String_UNSAFE_SubstringAt(&motd, index + 4);
	String_InitArray(url, urlBuffer);
	String_Format1(&url, "http://%s", &host);
	/* TODO: Replace $U with username */
	/*url = url.Replace("$U", game.Username); */

	/* Ensure that if the user quickly changes to a different world, env settings from old world aren't
	applied in the new world if the async 'get env request' didn't complete before the old world was unloaded */
	wom_identifier = Http_AsyncGetData(&url, true);
	wom_sendId = true;
}

static void WoM_CheckSendWomID(void) {
	static const cc_string msg = String_FromConst("/womid WoMClient-2.0.7");

	if (wom_sendId && !wom_sentId) {
		Chat_Send(&msg, false);
		wom_sentId = true;
	}
}

static PackedCol WoM_ParseCol(const cc_string* value, PackedCol defaultCol) {
	int argb;
	if (!Convert_ParseInt(value, &argb)) return defaultCol;
	return PackedCol_Make(argb >> 16, argb >> 8, argb, 255);
}

static void WoM_ParseConfig(struct HttpRequest* item) {
	cc_string line; char lineBuffer[STRING_SIZE * 2];
	struct Stream mem;
	cc_string key, value;
	int waterLevel;
	PackedCol col;

	String_InitArray(line, lineBuffer);
	Stream_ReadonlyMemory(&mem, item->data, item->size);

	while (!Stream_ReadLine(&mem, &line)) {
		Platform_Log(line.buffer, line.length);
		if (!String_UNSAFE_Separate(&line, '=', &key, &value)) continue;

		if (String_CaselessEqualsConst(&key, "environment.cloud")) {
			col = WoM_ParseCol(&value, ENV_DEFAULT_CLOUDS_COL);
			Env_SetCloudsCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.sky")) {
			col = WoM_ParseCol(&value, ENV_DEFAULT_SKY_COL);
			Env_SetSkyCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.fog")) {
			col = WoM_ParseCol(&value, ENV_DEFAULT_FOG_COL);
			Env_SetFogCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.level")) {
			if (Convert_ParseInt(&value, &waterLevel)) {
				Env_SetEdgeHeight(waterLevel);
			}
		} else if (String_CaselessEqualsConst(&key, "user.detail") && !cpe_useMessageTypes) {
			Chat_AddOf(&value, MSG_TYPE_STATUS_2);
		}
	}
}

static void WoM_Reset(void) {
	wom_identifier = 0;
	wom_sendId = false; wom_sentId = false;
}

static void WoM_Tick(void) {
	struct HttpRequest item;
	if (!Http_GetResult(wom_identifier, &item)) return;
	if (!item.success) return;

	WoM_ParseConfig(&item);
	Mem_Free(item.data);
}


/*########################################################################################################################*
*----------------------------------------------------Classic protocol-----------------------------------------------------*
*#########################################################################################################################*/

void Classic_SendChat(const cc_string* text, cc_bool partial) {
	cc_uint8 data[66];
	data[0] = OPCODE_MESSAGE;
	{
		data[1] = Server.SupportsPartialMessages ? partial : ENTITIES_SELF_ID;
		WriteString(&data[2], text);
	}
	Server.SendData(data, 66);
}

void Classic_WritePosition(Vec3 pos, float yaw, float pitch) {
	BlockID payload;
	int x, y, z;

	cc_uint8* data = Server.WriteBuffer;
	*data++ = OPCODE_ENTITY_TELEPORT;
	{
		payload = cpe_sendHeldBlock ? Inventory_SelectedBlock : ENTITIES_SELF_ID;
		WriteBlock(data, payload);
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

		*data++ = Math_Deg2Packed(yaw);
		*data++ = Math_Deg2Packed(pitch);
	}
	Server.WriteBuffer = data;
}

void Classic_WriteSetBlock(int x, int y, int z, cc_bool place, BlockID block) {
	cc_uint8* data = Server.WriteBuffer;
	*data++ = OPCODE_SET_BLOCK_CLIENT;
	{
		Stream_SetU16_BE(data, x); data += 2;
		Stream_SetU16_BE(data, y); data += 2;
		Stream_SetU16_BE(data, z); data += 2;
		*data++ = place;
		WriteBlock(data, block);
	}
	Server.WriteBuffer = data;
}

void Classic_SendLogin(void) {
	cc_uint8 data[131];
	data[0] = OPCODE_HANDSHAKE;
	{
		data[1] = 7; /* protocol version */
		WriteString(&data[2],  &Game_Username);
		WriteString(&data[66], &Game_Mppass);
		data[130] = Game_UseCPE ? 0x42 : 0x00;
	}
	Server.SendData(data, 131);
}

static void Classic_Handshake(cc_uint8* data) {
	struct HacksComp* hacks;

	Server.Name.length = 0;
	Server.MOTD.length = 0;
	data++; /* protocol version */

	ReadString(&data, &Server.Name);
	ReadString(&data, &Server.MOTD);
	Chat_SetLogName(&Server.Name);

	hacks = &LocalPlayer_Instance.Hacks;
	UpdateUserType(hacks, *data);
	
	String_Copy(&hacks->HacksFlags,         &Server.Name);
	String_AppendString(&hacks->HacksFlags, &Server.MOTD);
	HacksComp_RecheckFlags(hacks);
}

static void Classic_Ping(cc_uint8* data) { }

#define MAP_SIZE_LEN 4
static void DisconnectInvalidMap(cc_result res) {
	static const cc_string title  = String_FromConst("Disconnected");
	cc_string tmp; char tmpBuffer[STRING_SIZE];
	String_InitArray(tmp, tmpBuffer);

	String_Format1(&tmp, "Server sent corrupted map data (error %h)", &res);
	Game_Disconnect(&title, &tmp); return;
}

static void MapState_Init(struct MapState* m) {
	Inflate_MakeStream2(&m->stream, &m->inflateState, &map_part);
	m->index  = 0;
	m->blocks = NULL;
	m->allocFailed = false;
}

static void FreeMapStates(void) {
	Mem_Free(map.blocks);
	map.blocks  = NULL;
#ifdef EXTENDED_BLOCKS
	Mem_Free(map2.blocks);
	map2.blocks = NULL;
#endif
}

static void MapState_Read(struct MapState* m) {
	cc_uint32 left, read;
	cc_result res;
	if (m->allocFailed) return;

	if (!m->blocks) {
		m->blocks = (BlockRaw*)Mem_TryAlloc(map_volume, 1);
		/* unlikely but possible */
		if (!m->blocks) {
			Window_ShowDialog("Out of memory", "Not enough free memory to join that map.\nTry joining a different map.");
			m->allocFailed = true;
			return;
		}
	}

	left = map_volume - m->index;
	res  = m->stream.Read(&m->stream, &m->blocks[m->index], left, &read);

	if (res) DisconnectInvalidMap(res);
	m->index += read;
}

static void Classic_StartLoading(void) {
	World_NewMap();
	Stream_ReadonlyMemory(&map_part, NULL, 0);

	LoadingScreen_Show(&Server.Name, &Server.MOTD);
	WoM_CheckMotd();
	classic_receivedFirstPos = false;

	GZipHeader_Init(&map_gzHeader);
	map_begunLoading = true;
	map_sizeIndex    = 0;
	map_receiveBeg   = Stopwatch_Measure();
	map_volume       = 0;

	MapState_Init(&map);
#ifdef EXTENDED_BLOCKS
	MapState_Init(&map2);
#endif
}

static void Classic_LevelInit(cc_uint8* data) {
	if (!map_begunLoading) Classic_StartLoading();
	if (!cpe_fastMap) return;

	/* Fast map puts volume in header, and uses raw DEFLATE without GZIP header/footer */
	map_volume    = Stream_GetU32_BE(data);
	map_gzHeader.done = true;
	map_sizeIndex = MAP_SIZE_LEN;
}

static void Classic_LevelDataChunk(cc_uint8* data) {
	int usedLength;
	float progress;
	cc_uint32 left, read;
	cc_uint8 value;
	cc_result res;

	/* Workaround for some servers that send LevelDataChunk before LevelInit due to their async sending behaviour */
	if (!map_begunLoading) Classic_StartLoading();
	usedLength = Stream_GetU16_BE(data); data += 2;

	map_part.Meta.Mem.Cur    = data;
	map_part.Meta.Mem.Base   = data;
	map_part.Meta.Mem.Left   = usedLength;
	map_part.Meta.Mem.Length = usedLength;

	data += 1024;
	value = *data; /* progress in original classic, but we ignore it */

	if (!map_gzHeader.done) {
		res = GZipHeader_Read(&map_part, &map_gzHeader);
		if (res && res != ERR_END_OF_STREAM) { DisconnectInvalidMap(res); return; }
	}

	if (map_gzHeader.done) {
		if (map_sizeIndex < MAP_SIZE_LEN) {
			left = MAP_SIZE_LEN - map_sizeIndex;
			res  = map.stream.Read(&map.stream, &map_size[map_sizeIndex], left, &read); 

			if (res) { DisconnectInvalidMap(res); return; }
			map_sizeIndex += read;
		}

		if (map_sizeIndex == MAP_SIZE_LEN) {
			if (!map_volume) map_volume = Stream_GetU32_BE(map_size);

#ifndef EXTENDED_BLOCKS
			MapState_Read(&map);
#else
			if (cpe_extBlocks && value) {
				MapState_Read(&map2);
			} else {
				MapState_Read(&map);
			}
#endif
		}
	}

	progress = !map.blocks ? 0.0f : (float)map.index / map_volume;
	Event_RaiseFloat(&WorldEvents.Loading, progress);
}

static void Classic_LevelFinalise(cc_uint8* data) {
	int width, height, length;
	cc_uint64 end;
	int delta;

	end   = Stopwatch_Measure();
	delta = Stopwatch_ElapsedMS(map_receiveBeg, end);
	Platform_Log1("map loading took: %i", &delta);
	map_begunLoading = false;
	WoM_CheckSendWomID();

#ifdef EXTENDED_BLOCKS
	if (map2.allocFailed) FreeMapStates();
#endif

	width  = Stream_GetU16_BE(data + 0);
	height = Stream_GetU16_BE(data + 2);
	length = Stream_GetU16_BE(data + 4);

	if (map_volume != (width * height * length)) {
		Chat_AddRaw("&cFailed to load map, try joining a different map");
		Chat_AddRaw("   &cBlocks array size does not match volume of map");
		FreeMapStates();
	}
	
#ifdef EXTENDED_BLOCKS
	/* defer allocation of second map array if possible */
	if (cpe_extBlocks && map2.blocks) World_SetMapUpper(map2.blocks);
	map2.blocks = NULL;
#endif
	World_SetNewMap(map.blocks, width, height, length);
	map.blocks  = NULL;
}

static void Classic_SetBlock(cc_uint8* data) {
	int x, y, z;
	BlockID block;

	x = Stream_GetU16_BE(data + 0);
	y = Stream_GetU16_BE(data + 2);
	z = Stream_GetU16_BE(data + 4);
	data += 6;

	ReadBlock(data, block);
	if (World_Contains(x, y, z)) {
		Game_UpdateBlock(x, y, z, block);
	}
}

static void Classic_AddEntity(cc_uint8* data) {
	static const cc_string group = String_FromConst("Players");
	cc_string name; char nameBuffer[STRING_SIZE];
	cc_string skin; char skinBuffer[STRING_SIZE];
	EntityID id;
	String_InitArray(name, nameBuffer);
	String_InitArray(skin, skinBuffer);

	id = *data++;
	ReadString(&data, &name);
	CheckName(id, &name, &skin);
	AddEntity(data, id, &name, &skin, true);

	/* Workaround for some servers that declare support for ExtPlayerList but don't send ExtAddPlayerName */
	AddTablistEntry(id, &name, &name, &group, 0);
	Classic_TabList_Set(id);
}

static void Classic_EntityTeleport(cc_uint8* data) {
	EntityID id = *data++;
	Classic_ReadAbsoluteLocation(data, id, true);
}

static void Classic_RelPosAndOrientationUpdate(cc_uint8* data) {
	struct LocationUpdate update;
	EntityID id = data[0];
	Vec3 pos;
	float yaw, pitch;

	pos.X = (cc_int8)data[1] / 32.0f;
	pos.Y = (cc_int8)data[2] / 32.0f;
	pos.Z = (cc_int8)data[3] / 32.0f;
	yaw   = Math_Packed2Deg(data[4]);
	pitch = Math_Packed2Deg(data[5]);

	LocationUpdate_MakePosAndOri(&update, pos, yaw, pitch, true);
	UpdateLocation(id, &update, true);
}

static void Classic_RelPositionUpdate(cc_uint8* data) {
	struct LocationUpdate update;
	EntityID id = data[0];
	Vec3 pos;

	pos.X = (cc_int8)data[1] / 32.0f;
	pos.Y = (cc_int8)data[2] / 32.0f;
	pos.Z = (cc_int8)data[3] / 32.0f;

	LocationUpdate_MakePos(&update, pos, true);
	UpdateLocation(id, &update, true);
}

static void Classic_OrientationUpdate(cc_uint8* data) {
	struct LocationUpdate update;
	EntityID id = data[0];
	float yaw, pitch;

	yaw   = Math_Packed2Deg(data[1]);
	pitch = Math_Packed2Deg(data[2]);

	LocationUpdate_MakeOri(&update, yaw, pitch);
	UpdateLocation(id, &update, true);
}

static void Classic_RemoveEntity(cc_uint8* data) {
	EntityID id = data[0];
	Protocol_RemoveEntity(id);
}

static void Classic_Message(cc_uint8* data) {
	static const cc_string detailMsg  = String_FromConst("^detail.user=");
	static const cc_string detailUser = String_FromConst("^detail.user");
	cc_string text; char textBuffer[STRING_SIZE + 2];

	cc_uint8 type = *data++;
	String_InitArray(text, textBuffer);

	/* Original vanilla server uses player ids for type, 255 for server messages (&e prefix) */
	if (!cpe_useMessageTypes) {
		if (type == 0xFF) String_AppendConst(&text, "&e");
		type = MSG_TYPE_NORMAL;
	}
	ReadString(&data, &text);

	/* WoM detail messages (used e.g. for fCraft server compass) */
	if (String_CaselessStarts(&text, &detailMsg)) {
		text = String_UNSAFE_SubstringAt(&text, detailMsg.length);
		type = MSG_TYPE_STATUS_3;
	}
	/* Ignore ^detail.user.joined etc */
	if (!String_CaselessStarts(&text, &detailUser)) Chat_AddOf(&text, type);
}

static void Classic_Kick(cc_uint8* data) {
	static const cc_string title = String_FromConst("&eLost connection to the server");
	cc_string reason = UNSAFE_GetString(data);
	Game_Disconnect(&title, &reason);
}

static void Classic_SetPermission(cc_uint8* data) {
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	UpdateUserType(hacks, data[0]);
	HacksComp_RecheckFlags(hacks);
}

static void Classic_ReadAbsoluteLocation(cc_uint8* data, EntityID id, cc_bool interpolate) {
	struct LocationUpdate update;
	int x, y, z;
	Vec3 pos;
	float yaw, pitch;

	if (cpe_extEntityPos) {
		x = (int)Stream_GetU32_BE(&data[0]);
		y = (int)Stream_GetU32_BE(&data[4]);
		z = (int)Stream_GetU32_BE(&data[8]);
		data += 12;
	} else {
		x = (cc_int16)Stream_GetU16_BE(&data[0]);
		y = (cc_int16)Stream_GetU16_BE(&data[2]);
		z = (cc_int16)Stream_GetU16_BE(&data[4]);
		data += 6;
	}

	y -= 51; /* Convert to feet position */
	if (id == ENTITIES_SELF_ID) y += 22;

	pos.X = x/32.0f; pos.Y = y/32.0f; pos.Z = z/32.0f;
	yaw   = Math_Packed2Deg(*data++);
	pitch = Math_Packed2Deg(*data++);

	if (id == ENTITIES_SELF_ID) classic_receivedFirstPos = true;
	LocationUpdate_MakePosAndOri(&update, pos, yaw, pitch, false);
	UpdateLocation(id, &update, interpolate);
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
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct Entity* e      = &LocalPlayer_Instance.Base;
	if (!classic_receivedFirstPos) return;
	/* Report end position of each physics tick, rather than current position */
	/*  (otherwise can miss landing on a block then jumping off of it again) */
	Classic_WritePosition(p->Interp.Next.Pos, e->Yaw, e->Pitch);
}


/*########################################################################################################################*
*------------------------------------------------------CPE protocol-------------------------------------------------------*
*#########################################################################################################################*/
static const char* cpe_clientExtensions[35] = {
	"ClickDistance", "CustomBlocks", "HeldBlock", "EmoteFix", "TextHotKey", "ExtPlayerList",
	"EnvColors", "SelectionCuboid", "BlockPermissions", "ChangeModel", "EnvMapAppearance",
	"EnvWeatherType", "MessageTypes", "HackControl", "PlayerClick", "FullCP437", "LongerMessages",
	"BlockDefinitions", "BlockDefinitionsExt", "BulkBlockUpdate", "TextColors", "EnvMapAspect",
	"EntityProperty", "ExtEntityPositions", "TwoWayPing", "InventoryOrder", "InstantMOTD", "FastMap", "SetHotbar",
	"SetSpawnpoint", "VelocityControl", "CustomParticles", "CustomModels",
	/* NOTE: These must be placed last for when EXTENDED_TEXTURES or EXTENDED_BLOCKS are not defined */
	"ExtendedTextures", "ExtendedBlocks"
};
static void CPE_SetMapEnvUrl(cc_uint8* data);

#define Ext_Deg2Packed(x) ((int)((x) * 65536.0f / 360.0f))
void CPE_SendPlayerClick(int button, cc_bool pressed, cc_uint8 targetId, struct RayTracer* t) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	cc_uint8 data[15];

	data[0] = OPCODE_PLAYER_CLICK;
	{
		data[1] = button;
		data[2] = !pressed;
		Stream_SetU16_BE(&data[3], Ext_Deg2Packed(p->Yaw));
		Stream_SetU16_BE(&data[5], Ext_Deg2Packed(p->Pitch));

		data[7] = targetId;
		Stream_SetU16_BE(&data[8],  t->pos.X);
		Stream_SetU16_BE(&data[10], t->pos.Y);
		Stream_SetU16_BE(&data[12], t->pos.Z);

		data[14] = 255;
		/* Our own face values differ from CPE block face */
		switch (t->Closest) {
		case FACE_XMAX: data[14] = 0; break;
		case FACE_XMIN: data[14] = 1; break;
		case FACE_YMAX: data[14] = 2; break;
		case FACE_YMIN: data[14] = 3; break;
		case FACE_ZMAX: data[14] = 4; break;
		case FACE_ZMIN: data[14] = 5; break;
		}
	}
	Server.SendData(data, 15);
}

static void CPE_SendExtInfo(int extsCount) {
	cc_uint8 data[67];
	data[0] = OPCODE_EXT_INFO;
	{
		WriteString(data + 1,       &Server.AppName);
		Stream_SetU16_BE(data + 65, extsCount);
	}
	Server.SendData(data, 67);
}

static void CPE_SendExtEntry(const cc_string* extName, int extVersion) {
	cc_uint8 data[69];
	data[0] = OPCODE_EXT_ENTRY;
	{
		WriteString(data + 1,       extName);
		Stream_SetU32_BE(data + 65, extVersion);
	}
	Server.SendData(data, 69);
}

static void CPE_WriteTwoWayPing(cc_bool serverToClient, int id) {
	cc_uint8* data = Server.WriteBuffer; 
	*data++ = OPCODE_TWO_WAY_PING;
	{
		*data++ = serverToClient;
		Stream_SetU16_BE(data, id); data += 2;
	}
	Server.WriteBuffer = data;
}

static void CPE_SendCpeExtInfoReply(void) {
	int count = Array_Elems(cpe_clientExtensions);
	cc_string name;
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
	CPE_SendExtInfo(count);

	for (i = 0; i < Array_Elems(cpe_clientExtensions); i++) {
		name = String_FromReadonly(cpe_clientExtensions[i]);
		ver = 1;

		if (String_CaselessEqualsConst(&name, "ExtPlayerList"))       ver = 2;
		if (String_CaselessEqualsConst(&name, "EnvMapAppearance"))    ver = cpe_envMapVer;
		if (String_CaselessEqualsConst(&name, "BlockDefinitionsExt")) ver = cpe_blockDefsExtVer;
		if (String_CaselessEqualsConst(&name, "CustomModels"))        ver = cpe_customModelsVer;

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
		CPE_SendExtEntry(&name, ver);
	}
}

static void CPE_ExtInfo(cc_uint8* data) {
	static const cc_string d3Server = String_FromConst("D3 server");
	cc_string appName = UNSAFE_GetString(data);
	cpe_needD3Fix     = String_CaselessStarts(&appName, &d3Server);
	Chat_Add1("Server software: %s", &appName);

	/* Workaround for old MCGalaxy that send ExtEntry sync but ExtInfo async. */
	/* Means ExtEntry may sometimes arrive before ExtInfo, so use += instead of = */
	cpe_serverExtensionsCount += Stream_GetU16_BE(&data[64]);
	CPE_SendCpeExtInfoReply();
}

static void CPE_ExtEntry(cc_uint8* data) {
	cc_string ext = UNSAFE_GetString(data);
	int version   = data[67];
	Platform_Log2("cpe ext: %s, %i", &ext, &version);

	cpe_serverExtensionsCount--;
	CPE_SendCpeExtInfoReply();	

	/* update support state */
	if (String_CaselessEqualsConst(&ext, "HeldBlock")) {
		cpe_sendHeldBlock = true;
	} else if (String_CaselessEqualsConst(&ext, "MessageTypes")) {
		cpe_useMessageTypes = true;
	} else if (String_CaselessEqualsConst(&ext, "ExtPlayerList")) {
		Server.SupportsExtPlayerList = true;
	} else if (String_CaselessEqualsConst(&ext, "BlockPermissions")) {
		cpe_blockPerms = true;
	} else if (String_CaselessEqualsConst(&ext, "PlayerClick")) {
		Server.SupportsPlayerClick = true;
	} else if (String_CaselessEqualsConst(&ext, "EnvMapAppearance")) {
		cpe_envMapVer = version;
		if (version == 1) return;
		Protocol.Sizes[OPCODE_ENV_SET_MAP_APPEARANCE] += 4;
	} else if (String_CaselessEqualsConst(&ext, "LongerMessages")) {
		Server.SupportsPartialMessages = true;
	} else if (String_CaselessEqualsConst(&ext, "FullCP437")) {
		Server.SupportsFullCP437 = true;
	} else if (String_CaselessEqualsConst(&ext, "BlockDefinitionsExt")) {
		cpe_blockDefsExtVer = version;
		if (version == 1) return;
		Protocol.Sizes[OPCODE_DEFINE_BLOCK_EXT] += 3;
	} else if (String_CaselessEqualsConst(&ext, "ExtEntityPositions")) {
		Protocol.Sizes[OPCODE_ENTITY_TELEPORT] += 6;
		Protocol.Sizes[OPCODE_ADD_ENTITY]      += 6;
		Protocol.Sizes[OPCODE_EXT_ADD_ENTITY2] += 6;
		Protocol.Sizes[OPCODE_SET_SPAWNPOINT]  += 6;
		cpe_extEntityPos = true;
	} else if (String_CaselessEqualsConst(&ext, "TwoWayPing")) {
		cpe_twoWayPing = true;
	} else if (String_CaselessEqualsConst(&ext, "FastMap")) {
		Protocol.Sizes[OPCODE_LEVEL_BEGIN] += 4;
		cpe_fastMap = true;
	} else if (String_CaselessEqualsConst(&ext, "CustomModels")) {
		cpe_customModelsVer = min(2, version);
		if (version == 2) {
			Protocol.Sizes[OPCODE_DEFINE_MODEL_PART] = 167;
		}
	}
#ifdef EXTENDED_TEXTURES
	else if (String_CaselessEqualsConst(&ext, "ExtendedTextures")) {
		Protocol.Sizes[OPCODE_DEFINE_BLOCK]     += 3;
		Protocol.Sizes[OPCODE_DEFINE_BLOCK_EXT] += 6;
		cpe_extTextures = true;
	}
#endif
#ifdef EXTENDED_BLOCKS
	else if (String_CaselessEqualsConst(&ext, "ExtendedBlocks")) {
		if (!Game_AllowCustomBlocks) return;
		cpe_extBlocks = true;

		Protocol.Sizes[OPCODE_SET_BLOCK] += 1;
		Protocol.Sizes[OPCODE_HOLD_THIS] += 1;
		Protocol.Sizes[OPCODE_SET_BLOCK_PERMISSION] += 1;
		Protocol.Sizes[OPCODE_DEFINE_BLOCK]     += 1;
		Protocol.Sizes[OPCODE_UNDEFINE_BLOCK]   += 1;
		Protocol.Sizes[OPCODE_DEFINE_BLOCK_EXT] += 1;
		Protocol.Sizes[OPCODE_SET_INVENTORY_ORDER] += 2;
		Protocol.Sizes[OPCODE_BULK_BLOCK_UPDATE]   += 256 / 4;
		Protocol.Sizes[OPCODE_SET_HOTBAR]       += 1;
	}
#endif
}

static void CPE_SetClickDistance(cc_uint8* data) {
	LocalPlayer_Instance.ReachDistance = Stream_GetU16_BE(data) / 32.0f;
}

static void CPE_CustomBlockLevel(cc_uint8* data) {
	/* reply with version 1 level support */
	cc_uint8 reply[2] = { OPCODE_CUSTOM_BLOCK_LEVEL, 1 };
	Server.SendData(reply, 2);

	Game_UseCPEBlocks = true;
	Event_RaiseVoid(&BlockEvents.PermissionsChanged);
}

static void CPE_HoldThis(cc_uint8* data) {
	BlockID block;
	cc_bool canChange;

	ReadBlock(data, block);
	canChange = *data == 0;

	Inventory.CanChangeSelected = true;
	Inventory_SetSelectedBlock(block);
	Inventory.CanChangeSelected = canChange;
}

static void CPE_SetTextHotkey(cc_uint8* data) {
	/* First 64 bytes are label string */
	cc_string action  = UNSAFE_GetString(&data[64]);
	cc_uint32 keyCode = Stream_GetU32_BE(&data[128]);
	cc_uint8 keyMods  = data[132];
	int key;
	
	if (keyCode > 255) return;
	key = Hotkeys_LWJGL[keyCode];
	if (!key) return;
	Platform_Log3("CPE hotkey added: %c, %b: %s", Input_Names[key], &keyMods, &action);

	if (!action.length) {
		Hotkeys_Remove(key, keyMods);
		StoredHotkeys_Load(key, keyMods);
	} else if (action.buffer[action.length - 1] == '\n') {
		action.length--;
		Hotkeys_Add(key, keyMods, &action, false);
	} else { /* more input needed by user */
		Hotkeys_Add(key, keyMods, &action, true);
	}
}

static void CPE_ExtAddPlayerName(cc_uint8* data) {
	EntityID id = data[1]; /* 16 bit id */
	cc_string playerName = UNSAFE_GetString(&data[2]);
	cc_string listName   = UNSAFE_GetString(&data[66]);
	cc_string groupName  = UNSAFE_GetString(&data[130]);
	cc_uint8 groupRank   = data[194];

	RemoveEndPlus(&playerName);
	RemoveEndPlus(&listName);

	/* Workarond for server software that declares support for ExtPlayerList, but sends AddEntity then AddPlayerName */
	Classic_TabList_Reset(id);
	AddTablistEntry(id, &playerName, &listName, &groupName, groupRank);
}

static void CPE_ExtAddEntity(cc_uint8* data) {
	cc_string name, skin;
	EntityID id;

	id   = data[0];
	name = UNSAFE_GetString(data + 1);
	skin = UNSAFE_GetString(data + 65);

	CheckName(id, &name, &skin);
	AddEntity(data + 129, id, &name, &skin, false);
}

static void CPE_ExtRemovePlayerName(cc_uint8* data) {
	EntityID id = data[1];
	TabList_Remove(id);
}

static void CPE_MakeSelection(cc_uint8* data) {
	IVec3 p1, p2;
	PackedCol c;
	/* data[0] is id, data[1..64] is label */

	p1.X = (cc_int16)Stream_GetU16_BE(data + 65);
	p1.Y = (cc_int16)Stream_GetU16_BE(data + 67);
	p1.Z = (cc_int16)Stream_GetU16_BE(data + 69);
	p2.X = (cc_int16)Stream_GetU16_BE(data + 71);
	p2.Y = (cc_int16)Stream_GetU16_BE(data + 73);
	p2.Z = (cc_int16)Stream_GetU16_BE(data + 75);

	/* R,G,B,A are actually 16 bit unsigned integers */
	c = PackedCol_Make(data[78], data[80], data[82], data[84]);
	Selections_Add(data[0], &p1, &p2, c);
}

static void CPE_RemoveSelection(cc_uint8* data) {
	Selections_Remove(data[0]);
}

static void CPE_SetEnvCol(cc_uint8* data) {
	PackedCol c;
	cc_uint8 variable;
	cc_bool invalid;
	
	variable = data[0];
	invalid  = data[1] || data[3] || data[5];
	/* R,G,B are actually 16 bit unsigned integers */
	/* Above > 255 is 'invalid' (this is used by servers) */
	c = PackedCol_Make(data[2], data[4], data[6], 255);

	if (variable == 0) {
		Env_SetSkyCol(invalid    ? ENV_DEFAULT_SKY_COL    : c);
	} else if (variable == 1) {
		Env_SetCloudsCol(invalid ? ENV_DEFAULT_CLOUDS_COL : c);
	} else if (variable == 2) {
		Env_SetFogCol(invalid    ? ENV_DEFAULT_FOG_COL    : c);
	} else if (variable == 3) {
		Env_SetShadowCol(invalid ? ENV_DEFAULT_SHADOW_COL : c);
	} else if (variable == 4) {
		Env_SetSunCol(invalid    ? ENV_DEFAULT_SUN_COL    : c);
	} else if (variable == 5) {
		Env_SetSkyboxCol(invalid ? ENV_DEFAULT_SKYBOX_COL : c);
	}
}

static void CPE_SetBlockPermission(cc_uint8* data) {
	BlockID block; 
	ReadBlock(data, block);

	Blocks.CanPlace[block]  = *data++ != 0;
	Blocks.CanDelete[block] = *data++ != 0;
	Event_RaiseVoid(&BlockEvents.PermissionsChanged);
}

static void CPE_ChangeModel(cc_uint8* data) {
	struct Entity* e;
	EntityID id     = data[0];
	cc_string model = UNSAFE_GetString(data + 1);

	e = Entities.List[id];
	if (e) Entity_SetModel(e, &model);
}

static void CPE_EnvSetMapAppearance(cc_uint8* data) {
	int maxViewDist;

	CPE_SetMapEnvUrl(data);
	Env_SetSidesBlock(data[64]);
	Env_SetEdgeBlock(data[65]);
	Env_SetEdgeHeight((cc_int16)Stream_GetU16_BE(data + 66));
	if (cpe_envMapVer == 1) return;

	/* Version 2 */
	Env_SetCloudsHeight((cc_int16)Stream_GetU16_BE(data + 68));
	maxViewDist       = (cc_int16)Stream_GetU16_BE(data + 70);
	Game_MaxViewDistance = maxViewDist <= 0 ? DEFAULT_MAX_VIEWDIST : maxViewDist;
	Game_SetViewDistance(Game_UserViewDistance);
}

static void CPE_EnvWeatherType(cc_uint8* data) {
	Env_SetWeather(data[0]);
}

static void CPE_HackControl(cc_uint8* data) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	int jumpHeight;

	p->Hacks.CanFly            = data[0] != 0;
	p->Hacks.CanNoclip         = data[1] != 0;
	p->Hacks.CanSpeed          = data[2] != 0;
	p->Hacks.CanRespawn        = data[3] != 0;
	p->Hacks.CanUseThirdPerson = data[4] != 0;
	HacksComp_Update(&p->Hacks);
	jumpHeight = Stream_GetU16_BE(data + 5);

	if (jumpHeight == UInt16_MaxValue) { /* special value of -1 to reset default */
		LocalPlayer_ResetJumpVelocity();
	} else {
		p->Physics.JumpVel       = PhysicsComp_CalcJumpVelocity(jumpHeight / 32.0f);
		p->Physics.ServerJumpVel = p->Physics.JumpVel;
	}
}

static void CPE_ExtAddEntity2(cc_uint8* data) {
	cc_string name, skin;
	EntityID id;

	id   = data[0];
	name = UNSAFE_GetString(data + 1);
	skin = UNSAFE_GetString(data + 65);

	CheckName(id, &name, &skin);
	AddEntity(data + 129, id, &name, &skin, true);
}

#define BULK_MAX_BLOCKS 256
static void CPE_BulkBlockUpdate(cc_uint8* data) {
	cc_int32 indices[BULK_MAX_BLOCKS];
	BlockID blocks[BULK_MAX_BLOCKS];
	int index, i;
	int x, y, z;
	int count = 1 + *data++;

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
			cc_uint8 flags = data[i >> 2];
			blocks[i + 0] |= (BlockID)((flags & 0x03) << 8);
			blocks[i + 1] |= (BlockID)((flags & 0x0C) << 6);
			blocks[i + 2] |= (BlockID)((flags & 0x30) << 4);
			blocks[i + 3] |= (BlockID)((flags & 0xC0) << 2);
		}
		data += BULK_MAX_BLOCKS / 4;
	}

	for (i = 0; i < count; i++) {
		index = indices[i];
		if (index < 0 || index >= World.Volume) continue;
		World_Unpack(index, x, y, z);

#ifdef EXTENDED_BLOCKS
		Game_UpdateBlock(x, y, z, blocks[i] % BLOCK_COUNT);
#else
		Game_UpdateBlock(x, y, z, blocks[i]);
#endif
	}
}

static void CPE_SetTextColor(cc_uint8* data) {
	BitmapCol c   = BitmapCol_Make(data[0], data[1], data[2], data[3]);
	cc_uint8 code = data[4];

	/* disallow space, null, and colour code specifiers */
	if (code == '\0' || code == ' ' || code == 0xFF) return;
	if (code == '%'  || code == '&') return;

	Drawer2D.Colors[code] = c;
	Event_RaiseInt(&ChatEvents.ColCodeChanged, code);
}

static void CPE_SetMapEnvUrl(cc_uint8* data) {
	cc_string url = UNSAFE_GetString(data);

	if (!url.length || Utils_IsUrlPrefix(&url)) {
		Server_RetrieveTexturePack(&url);
	}
	Platform_Log1("Tex url: %s", &url);
}

static void CPE_SetMapEnvProperty(cc_uint8* data) {
	int value = (int)Stream_GetU32_BE(data + 1);
	Math_Clamp(value, -0xFFFFFF, 0xFFFFFF);

	switch (data[0]) {
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
		Game_MaxViewDistance = value <= 0 ? DEFAULT_MAX_VIEWDIST : value;
		Game_SetViewDistance(Game_UserViewDistance); break;
	case 5:
		Env_SetCloudsSpeed(value  / 256.0f); break;
	case 6:
		Env_SetWeatherSpeed(value / 256.0f); break;
	case 7:
		Env_SetWeatherFade(value  / 128.0f); break;
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

static void CPE_SetEntityProperty(cc_uint8* data) {
	struct LocationUpdate update = { 0 };
	struct Entity* e;
	float scale;

	EntityID id   = data[0];
	cc_uint8 type = data[1];
	int value     = (int)Stream_GetU32_BE(data + 2);

	e = Entities.List[id];
	if (!e) return;

	switch (type) {
	case 0:
		update.Flags = LOCATIONUPDATE_ROTX;
		update.RotX  = LocationUpdate_Clamp((float)value); break;
	case 1:
		update.Flags = LOCATIONUPDATE_YAW;
		update.Yaw   = LocationUpdate_Clamp((float)value); break;
	case 2:
		update.Flags = LOCATIONUPDATE_ROTZ;
		update.RotZ  = LocationUpdate_Clamp((float)value); break;

	case 3:
	case 4:
	case 5:
		scale = value / 1000.0f;
		if (e->ModelRestrictedScale) {
			Math_Clamp(scale, 0.01f, e->Model->maxScale);
		}

		if (type == 3) e->ModelScale.X = scale;
		if (type == 4) e->ModelScale.Y = scale;
		if (type == 5) e->ModelScale.Z = scale;

		Entity_UpdateModelBounds(e);
		return;
	default:
		return;
	}
	e->VTABLE->SetLocation(e, &update, true);
}

static void CPE_TwoWayPing(cc_uint8* data) {
	cc_uint8 serverToClient = data[0];
	int id = Stream_GetU16_BE(data + 1);

	if (serverToClient) {
		CPE_WriteTwoWayPing(true, id); /* server to client reply */
		Net_SendPacket();
	} else { Ping_Update(id); }
}

static void CPE_SetInventoryOrder(cc_uint8* data) {
	BlockID block, order;
	ReadBlock(data, block);
	ReadBlock(data, order);

	Inventory_Remove(block);
	if (order) { Inventory.Map[order - 1] = block; }
}

static void CPE_SetHotbar(cc_uint8* data) {
	BlockID block;
	cc_uint8 index;
	ReadBlock(data, block);
	index = *data;

	if (index >= INVENTORY_BLOCKS_PER_HOTBAR) return;
	Inventory_Set(index, block);
}

static void CPE_SetSpawnPoint(cc_uint8* data) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	int x, y, z;

	if (cpe_extEntityPos) {
		x = (int)Stream_GetU32_BE(&data[0]);
		y = (int)Stream_GetU32_BE(&data[4]);
		z = (int)Stream_GetU32_BE(&data[8]);
		data += 12;
	} else {
		x = (cc_int16)Stream_GetU16_BE(&data[0]);
		y = (cc_int16)Stream_GetU16_BE(&data[2]);
		z = (cc_int16)Stream_GetU16_BE(&data[4]);
		data += 6;
	}
	p->SpawnYaw   = Math_Packed2Deg(*data++);
	p->SpawnPitch = Math_Packed2Deg(*data++);

	y -= 51; /* Convert to feet position */
	Vec3_Set(p->Spawn, (float)(x / 32.0f), (float)(y / 32.0f), (float)(z / 32.0f));
}

static void CalcVelocity(float* vel, cc_uint8* src, cc_uint8 mode) {
	int raw     = (int)Stream_GetU32_BE(src);
	float value = Math_AbsF(raw / 10000.0f);
	value       = Math_Sign(raw) * PhysicsComp_CalcJumpVelocity(value);

	if (mode == 0) *vel += value;
	if (mode == 1) *vel = value;
}

static void CPE_VelocityControl(cc_uint8* data) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	CalcVelocity(&p->Base.Velocity.X, data + 0, data[12]);
	CalcVelocity(&p->Base.Velocity.Y, data + 4, data[13]);
	CalcVelocity(&p->Base.Velocity.Z, data + 8, data[14]);
}

static void CPE_DefineEffect(cc_uint8* data) {
	struct CustomParticleEffect* e = &Particles_CustomEffects[data[0]];

	/* e.g. bounds of 0,0, 15,15 gives an 8x8 icon in the default 128x128 particles.png */
	e->rec.U1 = data[1]       / 256.0f;
	e->rec.V1 = data[2]       / 256.0f;
	e->rec.U2 = (data[3] + 1) / 256.0f;
	e->rec.V2 = (data[4] + 1) / 256.0f;

	e->tintCol       = PackedCol_Make(data[5], data[6], data[7], 255);
	e->frameCount    = data[8];
	e->particleCount = data[9];
	e->size          = data[10] / 32.0f; /* 32 units per block */

	e->sizeVariation      = (int)Stream_GetU32_BE(data + 11) / 10000.0f;
	e->spread             =      Stream_GetU16_BE(data + 15) / 32.0f;
	e->speed              = (int)Stream_GetU32_BE(data + 17) / 10000.0f;
	e->gravity            = (int)Stream_GetU32_BE(data + 21) / 10000.0f;
	e->baseLifetime       = (int)Stream_GetU32_BE(data + 25) / 10000.0f;
	e->lifetimeVariation  = (int)Stream_GetU32_BE(data + 29) / 10000.0f;

	e->collideFlags = data[33];
	e->fullBright   = data[34];
}

static void CPE_SpawnEffect(cc_uint8* data) {
	float x, y, z, originX, originY, originZ;

	x       = (int)Stream_GetU32_BE(data +  1) / 32.0f;
	y       = (int)Stream_GetU32_BE(data +  5) / 32.0f;
	z       = (int)Stream_GetU32_BE(data +  9) / 32.0f;
	originX = (int)Stream_GetU32_BE(data + 13) / 32.0f;
	originY = (int)Stream_GetU32_BE(data + 17) / 32.0f;
	originZ = (int)Stream_GetU32_BE(data + 21) / 32.0f;

	Particles_CustomEffect(data[0], x, y, z, originX, originY, originZ);
}

static void CPE_DefineModel(cc_uint8* data) {
	cc_uint8 id = data[0];
	struct CustomModel* cm = &custom_models[id];
	cc_string name;
	cc_uint8 flags;
	cc_uint8 numParts;

	if (id >= MAX_CUSTOM_MODELS) return;
	CustomModel_Undefine(cm);
	Model_Init(&cm->model);

	name = UNSAFE_GetString(data + 1);
	String_CopyToRawArray(cm->name, &name);

	flags = data[65];
	cm->model.bobbing        = flags & 0x01;
	cm->model.pushes         = flags & 0x02;
	cm->model.usesHumanSkin  = flags & 0x04;
	cm->model.calcHumanAnims = flags & 0x08;

	cm->nameY = GetFloat(data + 66);
	cm->eyeY  = GetFloat(data + 70);

	cm->collisionBounds.X = GetFloat(data + 74);
	cm->collisionBounds.Y = GetFloat(data + 78);
	cm->collisionBounds.Z = GetFloat(data + 82);

	cm->pickingBoundsAABB.Min.X = GetFloat(data + 86);
	cm->pickingBoundsAABB.Min.Y = GetFloat(data + 90);
	cm->pickingBoundsAABB.Min.Z = GetFloat(data + 94);

	cm->pickingBoundsAABB.Max.X = GetFloat(data + 98);
	cm->pickingBoundsAABB.Max.Y = GetFloat(data + 102);
	cm->pickingBoundsAABB.Max.Z = GetFloat(data + 106);

	cm->uScale = Stream_GetU16_BE(data + 110);
	cm->vScale = Stream_GetU16_BE(data + 112);
	numParts   = data[114];

	if (numParts > MAX_CUSTOM_MODEL_PARTS) {
		cc_string msg; char msgBuffer[256];
		String_InitArray(msg, msgBuffer);

		String_Format1(
			&msg,
			"&cCustom Model '%s' exceeds parts limit of " STRINGIFY(MAX_CUSTOM_MODEL_PARTS),
			&name
		);
		Logger_WarnFunc(&msg);
		return;
	}

	cm->numParts = numParts;
	cm->model.vertices = (struct ModelVertex*)Mem_AllocCleared(numParts * MODEL_BOX_VERTICES,
												sizeof(struct ModelVertex), "CustomModel vertices");
	cm->defined = true;
}

static void CPE_DefineModelPart(cc_uint8* data) {
	cc_uint8 id = data[0];
	struct CustomModel* m = &custom_models[id];
	struct CustomModelPart* part;
	struct CustomModelPartDef p;
	int i;

	if (id >= MAX_CUSTOM_MODELS || !m->defined || m->curPartIndex >= m->numParts) return;
	part = &m->parts[m->curPartIndex];

	p.min.X = GetFloat(data +  1);
	p.min.Y = GetFloat(data +  5);
	p.min.Z = GetFloat(data +  9);
	p.max.X = GetFloat(data + 13);
	p.max.Y = GetFloat(data + 17);
	p.max.Z = GetFloat(data + 21);

	/* read u, v coords for our 6 faces */
	for (i = 0; i < 6; i++) {
		p.u1[i] = Stream_GetU16_BE(data + 25 + (i*8 + 0));
		p.v1[i] = Stream_GetU16_BE(data + 25 + (i*8 + 2));
		p.u2[i] = Stream_GetU16_BE(data + 25 + (i*8 + 4));
		p.v2[i] = Stream_GetU16_BE(data + 25 + (i*8 + 6));
	}

	p.rotationOrigin.X = GetFloat(data + 73);
	p.rotationOrigin.Y = GetFloat(data + 77);
	p.rotationOrigin.Z = GetFloat(data + 81);

	part->rotation.X = GetFloat(data + 85);
	part->rotation.Y = GetFloat(data + 89);
	part->rotation.Z = GetFloat(data + 93);

	if (cpe_customModelsVer == 1) {
		/* ignore animations */
		p.flags = data[102];
	} else if (cpe_customModelsVer == 2) {
		p.flags = data[165];

		data += 97;
		for (i = 0; i < MAX_CUSTOM_MODEL_ANIMS; i++) {
			cc_uint8 tmp = *data++;
			part->anims[i].type = tmp & 0x3F;
			part->anims[i].axis = tmp >> 6;

			part->anims[i].a = GetFloat(data);
			data += 4;
			part->anims[i].b = GetFloat(data);
			data += 4;
			part->anims[i].c = GetFloat(data);
			data += 4;
			part->anims[i].d = GetFloat(data);
			data += 4;
		}
	}

	CustomModel_BuildPart(m, &p);
	m->curPartIndex++;
	if (m->curPartIndex == m->numParts) CustomModel_Register(m);
}

/* unregisters and frees the custom model */
static void CPE_UndefineModel(cc_uint8* data) {
	cc_uint8 id = data[0];
	if (id < MAX_CUSTOM_MODELS) CustomModel_Undefine(&custom_models[id]);
}

static void CPE_Reset(void) {
	cpe_serverExtensionsCount = 0; cpe_pingTicks = 0;
	cpe_sendHeldBlock = false; cpe_useMessageTypes = false;
	cpe_envMapVer = 2; cpe_blockDefsExtVer = 2; cpe_customModelsVer = 2;
	cpe_needD3Fix = false; cpe_extEntityPos = false; cpe_twoWayPing = false; 
	cpe_extTextures = false; cpe_fastMap = false; cpe_extBlocks = false;
	Game_UseCPEBlocks = false; cpe_blockPerms = false;
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
	Net_Set(OPCODE_SET_HOTBAR, CPE_SetHotbar, 3);
	Net_Set(OPCODE_SET_SPAWNPOINT, CPE_SetSpawnPoint, 9);
	Net_Set(OPCODE_VELOCITY_CONTROL, CPE_VelocityControl, 16);
	Net_Set(OPCODE_DEFINE_EFFECT, CPE_DefineEffect, 36);
	Net_Set(OPCODE_SPAWN_EFFECT, CPE_SpawnEffect, 26);
	Net_Set(OPCODE_DEFINE_MODEL, CPE_DefineModel, 116);
	Net_Set(OPCODE_DEFINE_MODEL_PART, CPE_DefineModelPart, 104);
	Net_Set(OPCODE_UNDEFINE_MODEL, CPE_UndefineModel, 2);
}

static void CPE_Tick(void) {
	cpe_pingTicks++;
	if (cpe_pingTicks >= 20 && cpe_twoWayPing) {
		CPE_WriteTwoWayPing(false, Ping_NextPingId());
		cpe_pingTicks = 0;
	}
}


/*########################################################################################################################*
*------------------------------------------------------Custom blocks------------------------------------------------------*
*#########################################################################################################################*/
static void BlockDefs_OnBlockUpdated(BlockID block, cc_bool didBlockLight) {
	if (!World.Loaded) return;
	/* Need to refresh lighting when a block's light blocking state changes */
	if (Blocks.BlocksLight[block] != didBlockLight) Lighting_Refresh();
}

static TextureLoc BlockDefs_Tex(cc_uint8** ptr) {
	TextureLoc loc; 
	cc_uint8* data = *ptr;

	if (!cpe_extTextures) {
		loc = *data++;
	} else {
		loc = Stream_GetU16_BE(data) % ATLAS1D_MAX_ATLASES; data += 2;
	}

	*ptr = data; return loc;
}

static BlockID BlockDefs_DefineBlockCommonStart(cc_uint8** ptr, cc_bool uniqueSideTexs) {
	cc_string name;
	BlockID block;
	cc_bool didBlockLight;
	float speedLog2;
	cc_uint8 sound;
	cc_uint8* data = *ptr;

	ReadBlock(data, block);
	didBlockLight = Blocks.BlocksLight[block];
	Block_ResetProps(block);
	
	name = UNSAFE_GetString(data); data += STRING_SIZE;
	Block_SetName(block, &name);
	Block_SetCollide(block, *data++);

	speedLog2 = (*data++ - 128) / 64.0f;
	#define LOG_2 0.693147180559945
	Blocks.SpeedMultiplier[block] = (float)Math_Exp(LOG_2 * speedLog2); /* pow(2, x) */

	Block_Tex(block, FACE_YMAX) = BlockDefs_Tex(&data);
	if (uniqueSideTexs) {
		Block_Tex(block, FACE_XMIN) = BlockDefs_Tex(&data);
		Block_Tex(block, FACE_XMAX) = BlockDefs_Tex(&data);
		Block_Tex(block, FACE_ZMIN) = BlockDefs_Tex(&data);
		Block_Tex(block, FACE_ZMAX) = BlockDefs_Tex(&data);
	} else {
		Block_SetSide(BlockDefs_Tex(&data), block);
	}
	Block_Tex(block, FACE_YMIN) = BlockDefs_Tex(&data);

	Blocks.BlocksLight[block] = *data++ == 0;
	BlockDefs_OnBlockUpdated(block, didBlockLight);

	sound = *data++;
	Blocks.StepSounds[block] = sound;
	Blocks.DigSounds[block]  = sound;
	if (sound == SOUND_GLASS) Blocks.StepSounds[block] = SOUND_STONE;

	Blocks.FullBright[block] = *data++ != 0;
	*ptr = data;
	return block;
}

static void BlockDefs_DefineBlockCommonEnd(cc_uint8* data, cc_uint8 shape, BlockID block) {
	cc_uint8 draw = data[0];
	if (shape == 0) {
		Blocks.SpriteOffset[block] = draw;
		draw = DRAW_SPRITE;
	}
	Blocks.Draw[block] = draw;

	Blocks.FogDensity[block] = data[1] == 0 ? 0.0f : (data[1] + 1) / 128.0f;
	Blocks.FogCol[block]     = PackedCol_Make(data[2], data[3], data[4], 255);
	Block_DefineCustom(block);
}

static void BlockDefs_DefineBlock(cc_uint8* data) {
	BlockID block = BlockDefs_DefineBlockCommonStart(&data, false);

	cc_uint8 shape = *data++;
	if (shape > 0 && shape <= 16) {
		Blocks.MaxBB[block].Y = shape / 16.0f;
	}

	BlockDefs_DefineBlockCommonEnd(data, shape, block);
	/* Update sprite BoundingBox if necessary */
	if (Blocks.Draw[block] == DRAW_SPRITE) Block_RecalculateBB(block);
}

static void BlockDefs_UndefineBlock(cc_uint8* data) {
	BlockID block;
	cc_bool didBlockLight;

	ReadBlock(data, block);
	didBlockLight = Blocks.BlocksLight[block];

	Block_ResetProps(block);
	BlockDefs_OnBlockUpdated(block, didBlockLight);
	Block_UpdateCulling(block);

	Inventory_Remove(block);
	if (block < BLOCK_CPE_COUNT) { Inventory_AddDefault(block); }

	Block_SetCustomDefined(block, false);
	Event_RaiseVoid(&BlockEvents.BlockDefChanged);
	/* Update sprite BoundingBox if necessary */
	if (Blocks.Draw[block] == DRAW_SPRITE) Block_RecalculateBB(block);
}

static void BlockDefs_DefineBlockExt(cc_uint8* data) {
	Vec3 minBB, maxBB;
	BlockID block = BlockDefs_DefineBlockCommonStart(&data, cpe_blockDefsExtVer >= 2);

	minBB.X = (cc_int8)(*data++) / 16.0f;
	minBB.Y = (cc_int8)(*data++) / 16.0f;
	minBB.Z = (cc_int8)(*data++) / 16.0f;

	maxBB.X = (cc_int8)(*data++) / 16.0f;
	maxBB.Y = (cc_int8)(*data++) / 16.0f;
	maxBB.Z = (cc_int8)(*data++) / 16.0f;

	Blocks.MinBB[block] = minBB;
	Blocks.MaxBB[block] = maxBB;
	BlockDefs_DefineBlockCommonEnd(data, 1, block);
}

static void BlockDefs_Reset(void) {
	if (!Game_UseCPE || !Game_AllowCustomBlocks) return;
	Net_Set(OPCODE_DEFINE_BLOCK,     BlockDefs_DefineBlock,    80);
	Net_Set(OPCODE_UNDEFINE_BLOCK,   BlockDefs_UndefineBlock,  2);
	Net_Set(OPCODE_DEFINE_BLOCK_EXT, BlockDefs_DefineBlockExt, 85);
}


/*########################################################################################################################*
*-----------------------------------------------------Public handlers-----------------------------------------------------*
*#########################################################################################################################*/
static void Protocol_Reset(void) {
	Classic_Reset();
	CPE_Reset();
	BlockDefs_Reset();
	WoM_Reset();
}

void Protocol_Tick(void) {
	Classic_Tick();
	CPE_Tick();
	WoM_Tick();
}

static void OnInit(void) {
	if (Server.IsSinglePlayer) return;
	Protocol_Reset();
}

static void OnReset(void) {
	if (Server.IsSinglePlayer) return;
	Mem_Set(&Protocol, 0, sizeof(Protocol));
	Protocol_Reset();
	FreeMapStates();
}

struct IGameComponent Protocol_Component = {
	OnInit,  /* Init  */
	NULL,    /* Free  */
	OnReset, /* Reset */
};
