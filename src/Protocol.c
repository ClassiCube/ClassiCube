#include "Protocol.h"
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

/* Classic state */
static uint8_t classic_tabList[ENTITIES_MAX_COUNT >> 3];
static struct Screen* classic_prevScreen;
static bool classic_receivedFirstPos;

/* Map state */
static bool map_begunLoading;
static TimeMS map_receiveStart;
static struct Stream map_part;
static struct GZipHeader map_gzHeader;
static int map_sizeIndex, map_volume;
static uint8_t map_size[4];

struct MapState {
	struct InflateState inflateState;
	struct Stream stream;
	BlockRaw* blocks;
	int index;
	bool allocFailed;
};
static struct MapState map;
#ifdef EXTENDED_BLOCKS
static struct MapState map2;
#endif

/* CPE state */
bool cpe_needD3Fix;
static int cpe_serverExtensionsCount, cpe_pingTicks;
static int cpe_envMapVer = 2, cpe_blockDefsExtVer = 2, cpe_heldBlockVer = 2;
static bool cpe_sendHeldBlock, cpe_useMessageTypes, cpe_extEntityPos, cpe_blockPerms, cpe_fastMap;
static bool cpe_twoWayPing, cpe_extTextures, cpe_extBlocks;

/*########################################################################################################################*
*-----------------------------------------------------Common handlers-----------------------------------------------------*
*#########################################################################################################################*/
#define Classic_TabList_Get(id)   (classic_tabList[id >> 3] & (1 << (id & 0x7)))
#define Classic_TabList_Set(id)   (classic_tabList[id >> 3] |=  (uint8_t)(1 << (id & 0x7)))
#define Classic_TabList_Reset(id) (classic_tabList[id >> 3] &= (uint8_t)~(1 << (id & 0x7)))

#ifndef EXTENDED_BLOCKS
#define Protocol_ReadBlock(data, value) value = *data++;
#else
#define Protocol_ReadBlock(data, value)\
if (cpe_extBlocks) {\
	value = Stream_GetU16_BE(data); data += 2;\
} else { value = *data++; }
#endif

#ifndef EXTENDED_BLOCKS
#define Protocol_WriteBlock(data, value) *data++ = value;
#else
#define Protocol_WriteBlock(data, value)\
if (cpe_extBlocks) {\
	Stream_SetU16_BE(data, value); data += 2;\
} else { *data++ = (BlockRaw)value; }
#endif

static String Protocol_UNSAFE_GetString(uint8_t* data) {
	int i, length = 0;
	for (i = STRING_SIZE - 1; i >= 0; i--) {
		char code = data[i];
		if (code == '\0' || code == ' ') continue;
		length = i + 1; break;
	}
	return String_Init((char*)data, length, STRING_SIZE);
}

static void Protocol_ReadString(uint8_t** ptr, String* str) {
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

static void Protocol_WriteString(uint8_t* data, const String* value) {
	int i, count = min(value->length, STRING_SIZE);
	for (i = 0; i < count; i++) {
		char c = value->buffer[i];
		if (c == '&') c = '%'; /* escape colour codes */
		data[i] = c;
	}

	for (; i < STRING_SIZE; i++) { data[i] = ' '; }
}

static void Protocol_RemoveEndPlus(String* value) {
	/* Workaround for MCDzienny (and others) use a '+' at the end to distinguish classicube.net accounts */
	/* from minecraft.net accounts. Unfortunately they also send this ending + to the client. */
	if (!value->length || value->buffer[value->length - 1] != '+') return;
	value->length--;
}

static void Protocol_AddTablistEntry(EntityID id, const String* playerName, const String* listName, const String* groupName, uint8_t groupRank) {
	String rawName; char rawBuffer[STRING_SIZE];
	String_InitArray(rawName, rawBuffer);

	String_AppendColorless(&rawName, playerName);
	TabList_Set(id, &rawName, listName, groupName, groupRank);
}

static void Protocol_CheckName(EntityID id, String* name, String* skin) {
	String colorlessName; char colorlessBuffer[STRING_SIZE];

	Protocol_RemoveEndPlus(name);
	/* Server is only allowed to change our own name colours. */
	if (id == ENTITIES_SELF_ID) {
		String_InitArray(colorlessName, colorlessBuffer);
		String_AppendColorless(&colorlessName, name);
		if (!String_Equals(&colorlessName, &Game_Username)) String_Copy(name, &Game_Username);
	}

	if (!skin->length) String_Copy(skin, name);
	Protocol_RemoveEndPlus(skin);
	String_StripCols(skin);
}

static void Classic_ReadAbsoluteLocation(uint8_t* data, EntityID id, bool interpolate);
static void Protocol_AddEntity(uint8_t* data, EntityID id, const String* displayName, const String* skinName, bool readPosition) {
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
	Entity_SetSkin(e, skinName);
	Entity_SetName(e, displayName);

	if (!readPosition) return;
	Classic_ReadAbsoluteLocation(data, id, false);
	if (id != ENTITIES_SELF_ID) return;

	p->Spawn      = p->Base.Position;
	p->SpawnRotY  = p->Base.HeadY;
	p->SpawnHeadX = p->Base.HeadX;
}

void Protocol_RemoveEntity(EntityID id) {
	struct Entity* entity = Entities.List[id];
	if (!entity) return;
	if (id != ENTITIES_SELF_ID) Entities_Remove(id);

	/* See comment about some servers in Classic_AddEntity */
	if (!Classic_TabList_Get(id)) return;
	TabList_Remove(id);
	Classic_TabList_Reset(id);
}

static void Protocol_UpdateLocation(EntityID playerId, struct LocationUpdate* update, bool interpolate) {
	struct Entity* entity = Entities.List[playerId];
	if (entity) {
		entity->VTABLE->SetLocation(entity, update, interpolate);
	}
}


/*########################################################################################################################*
*------------------------------------------------------WoM protocol-------------------------------------------------------*
*#########################################################################################################################*/
/* Partially based on information from http://files.worldofminecraft.com/texturing/ */
/* NOTE: http://files.worldofminecraft.com/ has been down for quite a while, so support was removed on Oct 10, 2015 */

static char wom_identifierBuffer[32];
static String wom_identifier = String_FromArray(wom_identifierBuffer);
static int wom_counter;
static bool wom_sendId, wom_sentId;

static void WoM_UpdateIdentifier(void) {
	wom_identifier.length = 0;
	String_Format1(&wom_identifier, "womenv_%i", &wom_counter);
}

static void WoM_CheckMotd(void) {
	static const String cfg = String_FromConst("cfg=");
	String url; char urlBuffer[STRING_SIZE];
	String motd, host;
	int index;	

	motd = Server.MOTD;
	if (!motd.length) return;
	index = String_IndexOfString(&motd, &cfg);
	if (Game_PureClassic || index == -1) return;
	
	host = String_UNSAFE_SubstringAt(&motd, index + cfg.length);
	String_InitArray(url, urlBuffer);
	String_Format1(&url, "http://%s", &host);
	/* TODO: Replace $U with username */
	/*url = url.Replace("$U", game.Username); */

	/* Ensure that if the user quickly changes to a different world, env settings from old world aren't
	applied in the new world if the async 'get env request' didn't complete before the old world was unloaded */
	wom_counter++;
	WoM_UpdateIdentifier();
	Http_AsyncGetData(&url, true, &wom_identifier);
	wom_sendId = true;
}

static void WoM_CheckSendWomID(void) {
	static const String msg = String_FromConst("/womid WoMClient-2.0.7");

	if (wom_sendId && !wom_sentId) {
		Chat_Send(&msg, false);
		wom_sentId = true;
	}
}

static PackedCol WoM_ParseCol(const String* value, PackedCol defaultCol) {
	PackedCol col;
	int argb;
	if (!Convert_ParseInt(value, &argb)) return defaultCol;

	col.A = 255;
	col.R = (uint8_t)(argb >> 16);
	col.G = (uint8_t)(argb >> 8);
	col.B = (uint8_t)argb;
	return col;
}

static void WoM_ParseConfig(struct HttpRequest* item) {
	String line; char lineBuffer[STRING_SIZE * 2];
	struct Stream mem;
	String key, value;
	int waterLevel;
	PackedCol col;

	String_InitArray(line, lineBuffer);
	Stream_ReadonlyMemory(&mem, item->Data, item->Size);

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
			if (Convert_ParseInt(&value, &waterLevel)) {
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
	struct HttpRequest item;
	if (!Http_GetResult(&wom_identifier, &item)) return;
	if (!item.Success) return;

	WoM_ParseConfig(&item);
	HttpRequest_Free(&item);
}


/*########################################################################################################################*
*----------------------------------------------------Classic protocol-----------------------------------------------------*
*#########################################################################################################################*/

void Classic_SendChat(const String* text, bool partial) {
	uint8_t data[66];
	data[0] = OPCODE_MESSAGE;
	{
		data[1] = Server.SupportsPartialMessages ? partial : ENTITIES_SELF_ID;
		Protocol_WriteString(&data[2], text);
	}
	Server.SendData(data, 66);
}

void Classic_WritePosition(Vec3 pos, float rotY, float headX) {
	BlockID payload;
	int x, y, z;

	uint8_t* data = Server.WriteBuffer;
	*data++ = OPCODE_ENTITY_TELEPORT;
	{
		payload = cpe_sendHeldBlock ? Inventory_SelectedBlock : ENTITIES_SELF_ID;
		Protocol_WriteBlock(data, payload);
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
	Server.WriteBuffer = data;
}

void Classic_WriteSetBlock(int x, int y, int z, bool place, BlockID block) {
	uint8_t* data = Server.WriteBuffer;
	*data++ = OPCODE_SET_BLOCK_CLIENT;
	{
		Stream_SetU16_BE(data, x); data += 2;
		Stream_SetU16_BE(data, y); data += 2;
		Stream_SetU16_BE(data, z); data += 2;
		*data++ = place;
		Protocol_WriteBlock(data, block);
	}
	Server.WriteBuffer = data;
}

void Classic_SendLogin(const String* username, const String* verKey) {
	uint8_t data[131];
	data[0] = OPCODE_HANDSHAKE;
	{
		data[1] = 7; /* protocol version */
		Protocol_WriteString(&data[2],  username);
		Protocol_WriteString(&data[66], verKey);
		data[130] = Game_UseCPE ? 0x42 : 0x00;
	}
	Server.SendData(data, 131);
}

static void Classic_Handshake(uint8_t* data) {
	struct HacksComp* hacks;

	Server.Name.length = 0;
	Server.MOTD.length = 0;
	data++; /* protocol version */

	Protocol_ReadString(&data, &Server.Name);
	Protocol_ReadString(&data, &Server.MOTD);
	Chat_SetLogName(&Server.Name);

	hacks = &LocalPlayer_Instance.Hacks;
	HacksComp_SetUserType(hacks, *data, !cpe_blockPerms);
	
	String_Copy(&hacks->HacksFlags,         &Server.Name);
	String_AppendString(&hacks->HacksFlags, &Server.MOTD);
	HacksComp_RecheckFlags(hacks);
}

static void Classic_Ping(uint8_t* data) { }

static void MapState_Init(struct MapState* m) {
	Inflate_MakeStream(&m->stream, &m->inflateState, &map_part);
	m->index  = 0;
	m->blocks = NULL;
	m->allocFailed = false;
}

static void MapState_Read(struct MapState* m) {
	uint32_t left, read;
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
	m->stream.Read(&m->stream, &m->blocks[m->index], left, &read);
	m->index += read;
}

static void Classic_StartLoading(void) {
	World_Reset();
	Event_RaiseVoid(&WorldEvents.NewMap);
	Stream_ReadonlyMemory(&map_part, NULL, 0);

	classic_prevScreen = Gui_Active;
	if (classic_prevScreen == LoadingScreen_UNSAFE_RawPointer) {
		/* otherwise replacing LoadingScreen with LoadingScreen will cause issues */
		Gui_FreeActive();
		classic_prevScreen = NULL;
	}

	Gui_SetActive(LoadingScreen_MakeInstance(&Server.Name, &Server.MOTD));
	WoM_CheckMotd();
	classic_receivedFirstPos = false;

	GZipHeader_Init(&map_gzHeader);
	map_begunLoading = true;
	map_sizeIndex    = 0;
	map_receiveStart = DateTime_CurrentUTC_MS();
	map_volume       = 0;

	MapState_Init(&map);
#ifdef EXTENDED_BLOCKS
	MapState_Init(&map2);
#endif
}

static void Classic_LevelInit(uint8_t* data) {
	if (!map_begunLoading) Classic_StartLoading();
	if (!cpe_fastMap) return;

	/* Fast map puts volume in header, and uses raw DEFLATE without GZIP header/footer */
	map_volume    = Stream_GetU32_BE(data);
	map_gzHeader.Done = true;
	map_sizeIndex = 4;
}

static void Classic_LevelDataChunk(uint8_t* data) {
	int usedLength;
	float progress;
	uint32_t left, read;
	uint8_t value;
	ReturnCode res;

	/* Workaround for some servers that send LevelDataChunk before LevelInit due to their async sending behaviour */
	if (!map_begunLoading) Classic_StartLoading();
	usedLength = Stream_GetU16_BE(data); data += 2;

	map_part.Meta.Mem.Cur    = data;
	map_part.Meta.Mem.Base   = data;
	map_part.Meta.Mem.Left   = usedLength;
	map_part.Meta.Mem.Length = usedLength;

	data += 1024;
	value = *data; /* progress in original classic, but we ignore it */

	if (!map_gzHeader.Done) {
		res = GZipHeader_Read(&map_part, &map_gzHeader);
		if (res && res != ERR_END_OF_STREAM) Logger_Abort2(res, "reading map data");
	}

	if (map_gzHeader.Done) {
		if (map_sizeIndex < 4) {
			left = 4 - map_sizeIndex;
			map.stream.Read(&map.stream, &map_size[map_sizeIndex], left, &read); 
			map_sizeIndex += read;
		}

		if (map_sizeIndex == 4) {
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

static void Classic_LevelFinalise(uint8_t* data) {
	int width, height, length;
	int loadingMs;

	Gui_CloseActive();
	Gui_Active = classic_prevScreen;
	classic_prevScreen = NULL;
	Camera_CheckFocus();

	loadingMs = (int)(DateTime_CurrentUTC_MS() - map_receiveStart);
	Platform_Log1("map loading took: %i", &loadingMs);
	map_begunLoading = false;
	WoM_CheckSendWomID();

	if (map.allocFailed) return;
#ifdef EXTENDED_BLOCKS
	if (map2.allocFailed) { Mem_Free(map.blocks); map.blocks = NULL; return; }
#endif

	width  = Stream_GetU16_BE(&data[0]);
	height = Stream_GetU16_BE(&data[2]);
	length = Stream_GetU16_BE(&data[4]);

	if (map_volume != (width * height * length)) {
		Logger_Abort("Blocks array size does not match volume of map");
	}

	World_SetNewMap(map.blocks, width, height, length);
#ifdef EXTENDED_BLOCKS
	/* defer allocation of second map array if possible */
	if (cpe_extBlocks && map2.blocks) {
		World_SetMapUpper(map2.blocks);
	}
#endif
	Event_RaiseVoid(&WorldEvents.MapLoaded);
}

static void Classic_SetBlock(uint8_t* data) {
	int x, y, z;
	BlockID block;

	x = Stream_GetU16_BE(&data[0]);
	y = Stream_GetU16_BE(&data[2]);
	z = Stream_GetU16_BE(&data[4]);
	data += 6;

	Protocol_ReadBlock(data, block);
	if (World_Contains(x, y, z)) {
		Game_UpdateBlock(x, y, z, block);
	}
}

static void Classic_AddEntity(uint8_t* data) {
	static const String group = String_FromConst("Players");
	String name; char nameBuffer[STRING_SIZE];
	String skin; char skinBuffer[STRING_SIZE];
	EntityID id;
	String_InitArray(name, nameBuffer);
	String_InitArray(skin, skinBuffer);

	id = *data++;
	Protocol_ReadString(&data, &name);
	Protocol_CheckName(id, &name, &skin);
	Protocol_AddEntity(data, id, &name, &skin, true);

	/* Workaround for some servers that declare support for ExtPlayerList but don't send ExtAddPlayerName */
	Protocol_AddTablistEntry(id, &name, &name, &group, 0);
	Classic_TabList_Set(id);
}

static void Classic_EntityTeleport(uint8_t* data) {
	EntityID id = *data++;
	Classic_ReadAbsoluteLocation(data, id, true);
}

static void Classic_RelPosAndOrientationUpdate(uint8_t* data) {
	struct LocationUpdate update;
	EntityID id = *data++; 
	Vec3 pos;
	float rotY, headX;

	pos.X = (int8_t)(*data++) / 32.0f;
	pos.Y = (int8_t)(*data++) / 32.0f;
	pos.Z = (int8_t)(*data++) / 32.0f;
	rotY  = Math_Packed2Deg(*data++);
	headX = Math_Packed2Deg(*data++);

	LocationUpdate_MakePosAndOri(&update, pos, rotY, headX, true);
	Protocol_UpdateLocation(id, &update, true);
}

static void Classic_RelPositionUpdate(uint8_t* data) {
	struct LocationUpdate update;
	EntityID id = *data++; 
	Vec3 pos;

	pos.X = (int8_t)(*data++) / 32.0f;
	pos.Y = (int8_t)(*data++) / 32.0f;
	pos.Z = (int8_t)(*data++) / 32.0f;

	LocationUpdate_MakePos(&update, pos, true);
	Protocol_UpdateLocation(id, &update, true);
}

static void Classic_OrientationUpdate(uint8_t* data) {
	struct LocationUpdate update;
	EntityID id = *data++;
	float rotY, headX;

	rotY  = Math_Packed2Deg(*data++);
	headX = Math_Packed2Deg(*data++);

	LocationUpdate_MakeOri(&update, rotY, headX);
	Protocol_UpdateLocation(id, &update, true);
}

static void Classic_RemoveEntity(uint8_t* data) {
	EntityID id = *data;
	Protocol_RemoveEntity(id);
}

static void Classic_Message(uint8_t* data) {
	static const String detailMsg  = String_FromConst("^detail.user=");
	static const String detailUser = String_FromConst("^detail.user");
	String text; char textBuffer[STRING_SIZE + 2];

	uint8_t type = *data++;
	String_InitArray(text, textBuffer);

	/* Original vanilla server uses player ids for type, 255 for server messages (&e prefix) */
	if (!cpe_useMessageTypes) {
		if (type == 0xFF) String_AppendConst(&text, "&e");
		type = MSG_TYPE_NORMAL;
	}
	Protocol_ReadString(&data, &text);

	/* WoM detail messages (used e.g. for fCraft server compass) */
	if (String_CaselessStarts(&text, &detailMsg)) {
		text = String_UNSAFE_SubstringAt(&text, detailMsg.length);
		type = MSG_TYPE_STATUS_3;
	}
	/* Ignore ^detail.user.joined etc */
	if (!String_CaselessStarts(&text, &detailUser)) Chat_AddOf(&text, type);
}

static void Classic_Kick(uint8_t* data) {
	static const String title = String_FromConst("&eLost connection to the server");
	String reason = Protocol_UNSAFE_GetString(data);
	Game_Disconnect(&title, &reason);
}

static void Classic_SetPermission(uint8_t* data) {
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	HacksComp_SetUserType(hacks, *data, !cpe_blockPerms);
	HacksComp_RecheckFlags(hacks);
}

static void Classic_ReadAbsoluteLocation(uint8_t* data, EntityID id, bool interpolate) {
	struct LocationUpdate update;
	int x, y, z;
	Vec3 pos;
	float rotY, headX;

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

	pos.X = x/32.0f; pos.Y = y/32.0f; pos.Z = z/32.0f;
	rotY  = Math_Packed2Deg(*data++);
	headX = Math_Packed2Deg(*data++);

	if (id == ENTITIES_SELF_ID) classic_receivedFirstPos = true;
	LocationUpdate_MakePosAndOri(&update, pos, rotY, headX, false);
	Protocol_UpdateLocation(id, &update, interpolate);
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

#define Ext_Deg2Packed(x) ((int)((x) * 65536.0f / 360.0f))
void CPE_SendPlayerClick(MouseButton button, bool pressed, uint8_t targetId, struct PickedPos* pos) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	uint8_t data[15];

	data[0] = OPCODE_PLAYER_CLICK;
	{
		data[1] = button;
		data[2] = !pressed;
		Stream_SetU16_BE(&data[3], Ext_Deg2Packed(p->HeadY));
		Stream_SetU16_BE(&data[5], Ext_Deg2Packed(p->HeadX));

		data[7] = targetId;
		Stream_SetU16_BE(&data[8],  pos->BlockPos.X);
		Stream_SetU16_BE(&data[10], pos->BlockPos.Y);
		Stream_SetU16_BE(&data[12], pos->BlockPos.Z);

		data[14] = 255;
		/* Our own face values differ from CPE block face */
		switch (pos->Closest) {
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

static void CPE_SendExtInfo(const String* appName, int extsCount) {
	uint8_t data[67];
	data[0] = OPCODE_EXT_INFO;
	{
		Protocol_WriteString(&data[1], appName);
		Stream_SetU16_BE(&data[65],    extsCount);
	}
	Server.SendData(data, 67);
}

static void CPE_SendExtEntry(const String* extName, int extVersion) {
	uint8_t data[69];
	data[0] = OPCODE_EXT_ENTRY;
	{
		Protocol_WriteString(&data[1], extName);
		Stream_SetU32_BE(&data[65],    extVersion);
	}
	Server.SendData(data, 69);
}

static void CPE_WriteTwoWayPing(bool serverToClient, int id) {
	uint8_t* data = Server.WriteBuffer; 
	*data++ = OPCODE_TWO_WAY_PING;
	{
		*data++ = serverToClient;
		Stream_SetU16_BE(data, id); data += 2;
	}
	Server.WriteBuffer = data;
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
	CPE_SendExtInfo(&Server.AppName, count);

	for (i = 0; i < Array_Elems(cpe_clientExtensions); i++) {
		name = String_FromReadonly(cpe_clientExtensions[i]);
		ver = 1;

		if (String_CaselessEqualsConst(&name, "ExtPlayerList"))       ver = 2;
		if (String_CaselessEqualsConst(&name, "HeldBlock"))           ver = cpe_heldBlockVer;
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
		CPE_SendExtEntry(&name, ver);
	}
}

static void CPE_ExtInfo(uint8_t* data) {
	static const String d3Server = String_FromConst("D3 server");
	String appName = Protocol_UNSAFE_GetString(&data[0]);
	cpe_needD3Fix  = String_CaselessStarts(&appName, &d3Server);
	Chat_Add1("Server software: %s", &appName);

	/* Workaround for old MCGalaxy that send ExtEntry sync but ExtInfo async. */
	/* Means ExtEntry may sometimes arrive before ExtInfo, so use += instead of = */
	cpe_serverExtensionsCount += Stream_GetU16_BE(&data[64]);
	CPE_SendCpeExtInfoReply();
}

static void CPE_ExtEntry(uint8_t* data) {
	String ext  = Protocol_UNSAFE_GetString(&data[0]);
	int version = data[67];
	Platform_Log2("cpe ext: %s, %i", &ext, &version);

	cpe_serverExtensionsCount--;
	CPE_SendCpeExtInfoReply();	

	/* update support state */
	if (String_CaselessEqualsConst(&ext, "HeldBlock")) {
		cpe_sendHeldBlock = true;
		cpe_heldBlockVer = version;
		if (version == 1) return;
		Net_PacketSizes[OPCODE_HOLD_THIS] += 1;
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
		Net_PacketSizes[OPCODE_ENV_SET_MAP_APPEARANCE] += 4;
	} else if (String_CaselessEqualsConst(&ext, "LongerMessages")) {
		Server.SupportsPartialMessages = true;
	} else if (String_CaselessEqualsConst(&ext, "FullCP437")) {
		Server.SupportsFullCP437 = true;
	} else if (String_CaselessEqualsConst(&ext, "BlockDefinitionsExt")) {
		cpe_blockDefsExtVer = version;
		if (version == 1) return;
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
	/* reply with version 1 level support */
	uint8_t reply[2] = { OPCODE_CUSTOM_BLOCK_LEVEL, 1 };
	Server.SendData(reply, 2);

	Game_UseCPEBlocks = true;
	Event_RaiseVoid(&BlockEvents.PermissionsChanged);
}

static void CPE_HoldThis(uint8_t* data) {
	BlockID block;
	bool canChange;
	uint8_t index;

	Protocol_ReadBlock(data, block);
	canChange = *data++ == 0;
	Inventory.CanChangeSelected = true;

	if (cpe_heldBlockVer == 1) {
		Inventory_SetSelectedBlock(block);
	} else {
		index = *data;
		if (index == 255) {
			Inventory_SetSelectedBlock(block);
		} else {
			Inventory_SetBlockAtIndex(block, index);
		}
	}
	Inventory.CanChangeSelected = canChange;
}

static void CPE_SetTextHotkey(uint8_t* data) {
	/* First 64 bytes are label string */
	String action    = Protocol_UNSAFE_GetString(&data[64]);
	uint32_t keyCode = Stream_GetU32_BE(&data[128]);
	uint8_t keyMods  = data[132];
	Key key;
	
	if (keyCode > 255) return;
	key = Hotkeys_LWJGL[keyCode];
	if (!key) return;
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
	EntityID id = data[1]; /* 16 bit id */
	String playerName = Protocol_UNSAFE_GetString(&data[2]);
	String listName   = Protocol_UNSAFE_GetString(&data[66]);
	String groupName  = Protocol_UNSAFE_GetString(&data[130]);
	uint8_t groupRank = data[194];

	Protocol_RemoveEndPlus(&playerName);
	Protocol_RemoveEndPlus(&listName);

	/* Workarond for server software that declares support for ExtPlayerList, but sends AddEntity then AddPlayerName */
	Classic_TabList_Reset(id);
	Protocol_AddTablistEntry(id, &playerName, &listName, &groupName, groupRank);
}

static void CPE_ExtAddEntity(uint8_t* data) {
	String name; char nameBuffer[STRING_SIZE];
	String skin; char skinBuffer[STRING_SIZE];
	EntityID id;
	String_InitArray(name, nameBuffer);
	String_InitArray(skin, skinBuffer);

	id = *data++;
	Protocol_ReadString(&data, &name);
	Protocol_ReadString(&data, &skin);

	Protocol_CheckName(id, &name, &skin);
	Protocol_AddEntity(data, id, &name, &skin, false);
}

static void CPE_ExtRemovePlayerName(uint8_t* data) {
	EntityID id = data[1];
	TabList_Remove(id);
}

static void CPE_MakeSelection(uint8_t* data) {
	uint8_t selectionId;
	IVec3 p1, p2;
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
	PackedCol c;
	uint8_t variable;
	bool invalid;
	
	variable = *data++;
	invalid  = data[0] || data[2] || data[4];
	/* R,G,B are actually 16 bit unsigned integers */
	/* Above > 255 is 'invalid' (this is used by servers) */
	c.R = data[1]; c.G = data[3]; c.B = data[5]; c.A = 255;

	if (variable == 0) {
		Env_SetSkyCol(invalid ? Env_DefaultSkyCol : c);
	} else if (variable == 1) {
		Env_SetCloudsCol(invalid ? Env_DefaultCloudsCol : c);
	} else if (variable == 2) {
		Env_SetFogCol(invalid ? Env_DefaultFogCol : c);
	} else if (variable == 3) {
		Env_SetShadowCol(invalid ? Env_DefaultShadowCol : c);
	} else if (variable == 4) {
		Env_SetSunCol(invalid ? Env_DefaultSunCol : c);
	} else if (variable == 5) {
		Env_SetSkyboxCol(invalid ? Env_DefaultSkyboxCol : c);
	}
}

static void CPE_SetBlockPermission(uint8_t* data) {
	BlockID block; 
	Protocol_ReadBlock(data, block);

	Blocks.CanPlace[block]  = *data++ != 0;
	Blocks.CanDelete[block] = *data++ != 0;
	Event_RaiseVoid(&BlockEvents.PermissionsChanged);
}

static void CPE_ChangeModel(uint8_t* data) {
	struct Entity* entity;
	EntityID id  = data[0];
	String model = Protocol_UNSAFE_GetString(&data[1]);

	entity = Entities.List[id];
	if (entity) Entity_SetModel(entity, &model);
}

static void CPE_EnvSetMapAppearance(uint8_t* data) {
	int maxViewDist;

	CPE_SetMapEnvUrl(data);
	Env_SetSidesBlock(data[64]);
	Env_SetEdgeBlock(data[65]);
	Env_SetEdgeHeight((int16_t)Stream_GetU16_BE(&data[66]));
	if (cpe_envMapVer == 1) return;

	/* Version 2 */
	Env_SetCloudsHeight((int16_t)Stream_GetU16_BE(&data[68]));
	maxViewDist       = (int16_t)Stream_GetU16_BE(&data[70]);
	Game_MaxViewDistance = maxViewDist <= 0 ? 32768 : maxViewDist;
	Game_SetViewDistance(Game_UserViewDistance);
}

static void CPE_EnvWeatherType(uint8_t* data) {
	Env_SetWeather(*data);
}

static void CPE_HackControl(uint8_t* data) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct PhysicsComp* physics;
	int jumpHeight;

	p->Hacks.CanFly                  = *data++ != 0;
	p->Hacks.CanNoclip               = *data++ != 0;
	p->Hacks.CanSpeed                = *data++ != 0;
	p->Hacks.CanRespawn              = *data++ != 0;
	p->Hacks.CanUseThirdPersonCamera = *data++ != 0;
	HacksComp_Update(&p->Hacks);

	jumpHeight = Stream_GetU16_BE(data);
	physics    = &p->Physics;

	if (jumpHeight == UInt16_MaxValue) { /* special value of -1 to reset default */
		physics->JumpVel = HacksComp_CanJumpHigher(&p->Hacks) ? physics->UserJumpVel : 0.42f;
	} else {
		physics->JumpVel = PhysicsComp_CalcJumpVelocity(jumpHeight / 32.0f);
	}
	physics->ServerJumpVel = physics->JumpVel;
}

static void CPE_ExtAddEntity2(uint8_t* data) {
	String name; char nameBuffer[STRING_SIZE];
	String skin; char skinBuffer[STRING_SIZE];
	EntityID id;
	String_InitArray(name, nameBuffer);
	String_InitArray(skin, skinBuffer);

	id = *data++;
	Protocol_ReadString(&data, &name);
	Protocol_ReadString(&data, &skin);

	Protocol_CheckName(id, &name, &skin);
	Protocol_AddEntity(data, id, &name, &skin, true);
}

#define BULK_MAX_BLOCKS 256
static void CPE_BulkBlockUpdate(uint8_t* data) {
	int32_t indices[BULK_MAX_BLOCKS];
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
			uint8_t flags = data[i >> 2];
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

		if (World_Contains(x, y, z)) {
			Game_UpdateBlock(x, y, z, blocks[i]);
		}
	}
}

static void CPE_SetTextColor(uint8_t* data) {
	BitmapCol c;
	uint8_t code;

	c.R = *data++; c.G = *data++; c.B = *data++; c.A = *data++;
	code = *data;

	/* disallow space, null, and colour code specifiers */
	if (code == '\0' || code == ' ' || code == 0xFF) return;
	if (code == '%' || code == '&') return;

	Drawer2D_Cols[code] = c;
	Event_RaiseInt(&ChatEvents.ColCodeChanged, code);
}

static void CPE_SetMapEnvUrl(uint8_t* data) {
	String url = Protocol_UNSAFE_GetString(data);

	if (!url.length || Utils_IsUrlPrefix(&url)) {
		Server_RetrieveTexturePack(&url);
	}
	Platform_Log1("Tex url: %s", &url);
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

static void CPE_SetEntityProperty(uint8_t* data) {
	struct LocationUpdate update = { 0 };
	struct Entity* entity;
	float scale;

	EntityID id  = *data++;
	uint8_t type = *data++;
	int value    = (int)Stream_GetU32_BE(data);

	entity = Entities.List[id];
	if (!entity) return;

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
	int id = Stream_GetU16_BE(data);

	if (serverToClient) {
		CPE_WriteTwoWayPing(true, id); /* server to client reply */
		Net_SendPacket();
	} else { Ping_Update(id); }
}

static void CPE_SetInventoryOrder(uint8_t* data) {
	BlockID block, order;
	Protocol_ReadBlock(data, block);
	Protocol_ReadBlock(data, order);

	Inventory_Remove(block);
	if (order) { Inventory.Map[order - 1] = block; }
}

static void CPE_Reset(void) {
	cpe_serverExtensionsCount = 0; cpe_pingTicks = 0;
	cpe_sendHeldBlock = false; cpe_useMessageTypes = false;
	cpe_envMapVer = 2; cpe_blockDefsExtVer = 2; cpe_heldBlockVer = 2;
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
		CPE_WriteTwoWayPing(false, Ping_NextPingId());
		cpe_pingTicks = 0;
	}
}


/*########################################################################################################################*
*------------------------------------------------------Custom blocks------------------------------------------------------*
*#########################################################################################################################*/
static void BlockDefs_OnBlockUpdated(BlockID block, bool didBlockLight) {
	if (!World.Blocks) return;
	/* Need to refresh lighting when a block's light blocking state changes */
	if (Blocks.BlocksLight[block] != didBlockLight) Lighting_Refresh();
}

static TextureLoc BlockDefs_Tex(uint8_t** ptr) {
	TextureLoc loc; 
	uint8_t* data = *ptr;

	if (!cpe_extTextures) {
		loc = *data++;
	} else {
		loc = Stream_GetU16_BE(data) % ATLAS1D_MAX_ATLASES; data += 2;
	}

	*ptr = data; return loc;
}

static BlockID BlockDefs_DefineBlockCommonStart(uint8_t** ptr, bool uniqueSideTexs) {
	String name;
	BlockID block;
	bool didBlockLight;
	float speedLog2;
	uint8_t sound;
	uint8_t* data = *ptr;

	Protocol_ReadBlock(data, block);
	didBlockLight = Blocks.BlocksLight[block];
	Block_ResetProps(block);
	
	name = Protocol_UNSAFE_GetString(data); data += STRING_SIZE;
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

static void BlockDefs_DefineBlockCommonEnd(uint8_t* data, uint8_t shape, BlockID block) {
	uint8_t blockDraw;
	uint8_t density;
	PackedCol c;

	blockDraw = *data++;
	if (shape == 0) {
		Blocks.SpriteOffset[block] = blockDraw;
		blockDraw = DRAW_SPRITE;
	}
	Blocks.Draw[block] = blockDraw;

	density = *data++;
	Blocks.FogDensity[block] = density == 0 ? 0.0f : (density + 1) / 128.0f;

	c.R = *data++; c.G = *data++; c.B = *data++; c.A = 255;
	Blocks.FogCol[block] = c;
	Block_DefineCustom(block);
}

static void BlockDefs_DefineBlock(uint8_t* data) {
	BlockID block = BlockDefs_DefineBlockCommonStart(&data, false);

	uint8_t shape = *data++;
	if (shape > 0 && shape <= 16) {
		Blocks.MaxBB[block].Y = shape / 16.0f;
	}

	BlockDefs_DefineBlockCommonEnd(data, shape, block);
	/* Update sprite BoundingBox if necessary */
	if (Blocks.Draw[block] == DRAW_SPRITE) {
		Block_RecalculateBB(block);
	}
}

static void BlockDefs_UndefineBlock(uint8_t* data) {
	BlockID block;
	bool didBlockLight;

	Protocol_ReadBlock(data, block);
	didBlockLight = Blocks.BlocksLight[block];

	Block_ResetProps(block);
	BlockDefs_OnBlockUpdated(block, didBlockLight);
	Block_UpdateCulling(block);

	Inventory_Remove(block);
	if (block < BLOCK_CPE_COUNT) { Inventory_AddDefault(block); }

	Block_SetCustomDefined(block, false);
	Event_RaiseVoid(&BlockEvents.BlockDefChanged);
}

static void BlockDefs_DefineBlockExt(uint8_t* data) {
	Vec3 minBB, maxBB;
	BlockID block = BlockDefs_DefineBlockCommonStart(&data, cpe_blockDefsExtVer >= 2);

	minBB.X = (int8_t)(*data++) / 16.0f;
	minBB.Y = (int8_t)(*data++) / 16.0f;
	minBB.Z = (int8_t)(*data++) / 16.0f;

	maxBB.X = (int8_t)(*data++) / 16.0f;
	maxBB.Y = (int8_t)(*data++) / 16.0f;
	maxBB.Z = (int8_t)(*data++) / 16.0f;

	Blocks.MinBB[block] = minBB;
	Blocks.MaxBB[block] = maxBB;
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
void Protocol_Reset(void) {
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
