#include "Protocol.h"
#include "Game.h"
#ifdef CC_BUILD_NETWORKING
#include "String.h"
#include "Deflate.h"
#include "Server.h"
#include "Stream.h"
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
#include "InputHandler.h"
#include "HeldBlockRenderer.h"
#include "Options.h"

struct _ProtocolData Protocol;

/* Classic state */
static cc_bool classic_receivedFirstPos;

/* Map state */
static cc_bool map_begunLoading;
static cc_uint64 map_receiveBeg;
static struct Stream map_part;
static int map_volume;

/*########################################################################################################################*
*-----------------------------------------------------CPE extensions------------------------------------------------------*
*#########################################################################################################################*/
struct CpeExt {
	const char* name;
	cc_uint8 clientVersion, serverVersion;
};
cc_bool cpe_needD3Fix;
static int cpe_serverExtensionsCount, cpe_pingTicks;

static struct CpeExt 
	clickDist_Ext       = { "ClickDistance", 1 },
	customBlocks_Ext    = { "CustomBlocks", 1 },
	heldBlock_Ext       = { "HeldBlock", 1 },  
	emoteFix_Ext        = { "EmoteFix", 1 },
	textHotKey_Ext      = { "TextHotKey", 1 },  
	extPlayerList_Ext   = { "ExtPlayerList", 2 },
	envColors_Ext       = { "EnvColors", 1 },
	selectionCuboid_Ext = { "SelectionCuboid", 1 },
	blockPerms_Ext      = { "BlockPermissions", 1 },
	changeModel_Ext     = { "ChangeModel", 1 },
	mapAppearance_Ext   = { "EnvMapAppearance", 2 },
	weatherType_Ext     = { "EnvWeatherType", 1 },
	messageTypes_Ext    = { "MessageTypes", 1 },
	hackControl_Ext     = { "HackControl", 1 },
	playerClick_Ext     = { "PlayerClick", 1 },
	fullCP437_Ext       = { "FullCP437", 1 },
	longerMessages_Ext  = { "LongerMessages", 1 },
	blockDefs_Ext       = { "BlockDefinitions", 1 },
	blockDefsExt_Ext    = { "BlockDefinitionsExt", 2 },
	bulkBlockUpdate_Ext = { "BulkBlockUpdate", 1 },
	textColors_Ext      = { "TextColors", 1 },
	envMapAspect_Ext    = { "EnvMapAspect", 2 },
	entityProperty_Ext  = { "EntityProperty", 1 },
	extEntityPos_Ext    = { "ExtEntityPositions", 1 },
	twoWayPing_Ext      = { "TwoWayPing", 1 },
	invOrder_Ext        = { "InventoryOrder", 1 },
	instantMOTD_Ext     = { "InstantMOTD", 1 },
	fastMap_Ext         = { "FastMap", 1 },
	setHotbar_Ext       = { "SetHotbar", 1 },
	setSpawnpoint_Ext   = { "SetSpawnpoint", 1 },
	velControl_Ext      = { "VelocityControl", 1 },
	customParticles_Ext = { "CustomParticles", 1 },
	customModels_Ext    = { "CustomModels", 2 },
	pluginMessages_Ext  = { "PluginMessages", 1 },
	extTeleport_Ext     = { "ExtEntityTeleport", 1 },
	lightingMode_Ext    = { "LightingMode", 1 },
	cinematicGui_Ext    = { "CinematicGui", 1 },
	notifyAction_Ext    = { "NotifyAction", 1 },
	extTextures_Ext     = { "ExtendedTextures", 1 },
	extBlocks_Ext       = { "ExtendedBlocks", 1 };

static struct CpeExt* cpe_clientExtensions[] = {
	&clickDist_Ext, &customBlocks_Ext, &heldBlock_Ext, &emoteFix_Ext, &textHotKey_Ext, &extPlayerList_Ext,
	&envColors_Ext, &selectionCuboid_Ext, &blockPerms_Ext, &changeModel_Ext, &mapAppearance_Ext, &weatherType_Ext,
	&messageTypes_Ext, &hackControl_Ext, &playerClick_Ext, &fullCP437_Ext, &longerMessages_Ext, &blockDefs_Ext,
	&blockDefsExt_Ext, &bulkBlockUpdate_Ext, &textColors_Ext, &envMapAspect_Ext, &entityProperty_Ext, &extEntityPos_Ext,
	&twoWayPing_Ext, &invOrder_Ext, &instantMOTD_Ext, &fastMap_Ext, &setHotbar_Ext, &setSpawnpoint_Ext, &velControl_Ext,
	&customParticles_Ext, &pluginMessages_Ext, &extTeleport_Ext, &lightingMode_Ext, &cinematicGui_Ext, &notifyAction_Ext,
#ifdef CUSTOM_MODELS
	&customModels_Ext,
#endif
#ifdef EXTENDED_TEXTURES
	&extTextures_Ext,
#endif
#ifdef EXTENDED_BLOCKS
	&extBlocks_Ext,
#endif
}; 
#define IsSupported(ext) (ext.serverVersion > 0)


/*########################################################################################################################*
*-----------------------------------------------------Common handlers-----------------------------------------------------*
*#########################################################################################################################*/

#ifndef EXTENDED_BLOCKS
#define ReadBlock(data, value) value = *data++;
#else
#define ReadBlock(data, value)\
if (IsSupported(extBlocks_Ext)) {\
	value = Stream_GetU16_BE(data) % BLOCK_COUNT; data += 2;\
} else { value = *data++; }
#endif

#ifndef EXTENDED_BLOCKS
#define WriteBlock(data, value) *data++ = value;
#else
#define WriteBlock(data, value)\
if (IsSupported(extBlocks_Ext)) {\
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
		if (c == '&') c = '%'; /* escape color codes */
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

static void Classic_ReadAbsoluteLocation(cc_uint8* data, EntityID id, cc_uint8 flags);
static void AddEntity(cc_uint8* data, EntityID id, const cc_string* name, const cc_string* skin, cc_bool readPosition) {
	struct LocalPlayer* p = Entities.CurPlayer;
	struct Entity* e;

	if (id != ENTITIES_SELF_ID) {
		Entities_Remove(id);
		e = &NetPlayers_List[id].Base;

		NetPlayer_Init((struct NetPlayer*)e);
		Entities.List[id] = e;
		Event_RaiseInt(&EntityEvents.Added, id);
	} else {
		e = &Entities.CurPlayer->Base;
	}
	Entity_SetSkin(e, skin);
	Entity_SetName(e, name);

	if (!readPosition) return;
	Classic_ReadAbsoluteLocation(data, id,
		LU_HAS_POS | LU_HAS_YAW | LU_HAS_PITCH | LU_POS_ABSOLUTE_INSTANT);
	if (id != ENTITIES_SELF_ID) return;

	p->Spawn      = p->Base.Position;
	p->SpawnYaw   = p->Base.Yaw;
	p->SpawnPitch = p->Base.Pitch;
}

static void UpdateLocation(EntityID id, struct LocationUpdate* update) {
	struct Entity* e = Entities.List[id];
	if (e) { e->VTABLE->SetLocation(e, update); }
}

static void UpdateUserType(struct HacksComp* hacks, cc_uint8 value) {
	cc_bool isOp = value >= 100 && value <= 127;
	hacks->IsOp  = isOp;
	if (IsSupported(blockPerms_Ext)) return;

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
	wom_identifier = Http_AsyncGetData(&url, HTTP_FLAG_PRIORITY);
	wom_sendId = true;
}

static void WoM_CheckSendWomID(void) {
	static const cc_string msg = String_FromConst("/womid WoMClient-2.0.7");

	if (wom_sendId && !wom_sentId) {
		Chat_Send(&msg, false);
		wom_sentId = true;
	}
}

static PackedCol WoM_ParseCol(const cc_string* value, PackedCol defaultColor) {
	int argb;
	if (!Convert_ParseInt(value, &argb)) return defaultColor;
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
			col = WoM_ParseCol(&value, ENV_DEFAULT_CLOUDS_COLOR);
			Env_SetCloudsCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.sky")) {
			col = WoM_ParseCol(&value, ENV_DEFAULT_SKY_COLOR);
			Env_SetSkyCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.fog")) {
			col = WoM_ParseCol(&value, ENV_DEFAULT_FOG_COLOR);
			Env_SetFogCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.level")) {
			if (Convert_ParseInt(&value, &waterLevel)) {
				Env_SetEdgeHeight(waterLevel);
			}
		} else if (String_CaselessEqualsConst(&key, "user.detail") && !IsSupported(messageTypes_Ext)) {
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

	if (item.success) WoM_ParseConfig(&item);
	HttpRequest_Free(&item);
}


/*########################################################################################################################*
*----------------------------------------------------Map decompressor-----------------------------------------------------*
*#########################################################################################################################*/
#define MAP_SIZE_LEN 4

struct MapState {
	struct InflateState inflateState;
	struct Stream stream;
	BlockRaw* blocks;
	struct GZipHeader gzHeader;
	cc_uint8 size[MAP_SIZE_LEN];
	int index, sizeIndex;
	cc_bool allocFailed;
};
static struct MapState map1;
#ifdef EXTENDED_BLOCKS
static struct MapState map2;
#endif

static void DisconnectInvalidMap(cc_result res) {
	static const cc_string title  = String_FromConst("Disconnected");
	cc_string tmp; char tmpBuffer[STRING_SIZE];
	String_InitArray(tmp, tmpBuffer);

	String_Format1(&tmp, "Server sent corrupted map data (error %e)", &res);
	Game_Disconnect(&title, &tmp); return;
}

static void MapState_Init(struct MapState* m) {
	Inflate_MakeStream2(&m->stream, &m->inflateState, &map_part);
	GZipHeader_Init(&m->gzHeader);

	m->index       = 0;
	m->blocks      = NULL;
	m->sizeIndex   = 0;
	m->allocFailed = false;
}

static CC_INLINE void MapState_SkipHeader(struct MapState* m) {
	m->gzHeader.done = true;
	m->sizeIndex     = MAP_SIZE_LEN;
}

static void FreeMapStates(void) {
	Mem_Free(map1.blocks);
	map1.blocks = NULL;
#ifdef EXTENDED_BLOCKS
	Mem_Free(map2.blocks);
	map2.blocks = NULL;
#endif
}

static cc_result MapState_Read(struct MapState* m) {
	cc_uint32 left, read;
	cc_result res;
	if (m->allocFailed) return 0;

	if (m->sizeIndex < MAP_SIZE_LEN) {
		left = MAP_SIZE_LEN - m->sizeIndex;
		res  = m->stream.Read(&m->stream, &m->size[m->sizeIndex], left, &read); 

		m->sizeIndex += read;
		if (res) return res;

		/* 0.01% chance to happen, but still possible */
		if (m->sizeIndex < MAP_SIZE_LEN) return 0;
	}

	if (!map_volume) map_volume = Stream_GetU32_BE(m->size);

	if (!m->blocks) {
		m->blocks = (BlockRaw*)Mem_TryAlloc(map_volume, 1);
		/* unlikely but possible */
		if (!m->blocks) {
			Window_ShowDialog("Out of memory", "Not enough free memory to join that map.\nTry joining a different map.");
			m->allocFailed = true;
			return 0;
		}
	}

	left = map_volume - m->index;
	res  = m->stream.Read(&m->stream, &m->blocks[m->index], left, &read);

	m->index += read;
	return res;
}


/*########################################################################################################################*
*----------------------------------------------------Classic protocol-----------------------------------------------------*
*#########################################################################################################################*/
void Classic_SendLogin(void) {
	cc_uint8 data[131];
	data[0] = OPCODE_HANDSHAKE;
	{
		data[1]   = Game_Version.Protocol;
		WriteString(&data[2],  &Game_Username);
		WriteString(&data[66], &Game_Mppass);

		/* The 'user type' field's behaviour depends on protocol version: */
		/*   version 7 - 0x42 specifies CPE client, any other value is ignored? */
		/*   version 6 - any value ignored? */
		/*   version 5 - field doesn't exist */
		/* Theroetically, this means packet size is 131 bytes for 6/7, 130 bytes for 5 and below. */
		/* In practice, some version 7 server software always expects to read 131 bytes for handshake packet, */
		/*  and will get stuck waiting for data if client connects using version 5 and only sends 130 bytes */
		/* To workaround this, include a 'ping packet' after 'version 5 handshake packet' - version 5 server software */
		/*  will do nothing with the ping packet, and the aforementioned server software will be happy with 131 bytes */
		data[130] = Game_Version.HasCPE ? 0x42 : (Game_Version.Protocol <= PROTOCOL_0019 ? OPCODE_PING : 0x00);
	}
	Server.SendData(data, 131);
}

void Classic_SendChat(const cc_string* text, cc_bool partial) {
	cc_uint8 data[66];
	data[0] = OPCODE_MESSAGE;
	{
		data[1] = Server.SupportsPartialMessages ? partial : ENTITIES_SELF_ID;
		WriteString(&data[2], text);
	}
	Server.SendData(data, 66);
}

static cc_uint8* Classic_WritePosition(cc_uint8* data, Vec3 pos, float yaw, float pitch) {
	BlockID payload;
	int x, y, z;

	*data++ = OPCODE_ENTITY_TELEPORT;
	{
		payload = IsSupported(heldBlock_Ext) ? Inventory_SelectedBlock : ENTITIES_SELF_ID;
		WriteBlock(data, payload);
		x = (int)(pos.x * 32);
		y = (int)(pos.y * 32) + 51;
		z = (int)(pos.z * 32);

		if (IsSupported(extEntityPos_Ext)) {
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
	return data;
}

void Classic_SendSetBlock(int x, int y, int z, cc_bool place, BlockID block) {
	cc_uint8 tmp[32];
	cc_uint8* data = tmp;

	*data++ = OPCODE_SET_BLOCK_CLIENT;
	{
		Stream_SetU16_BE(data, x); data += 2;
		Stream_SetU16_BE(data, y); data += 2;
		Stream_SetU16_BE(data, z); data += 2;
		*data++ = place;
		WriteBlock(data, block);
	}
	Server.SendData(tmp, (cc_uint32)(data - tmp));
}

static void Classic_Handshake(cc_uint8* data) {
	struct HacksComp* hacks;

	Server.Name.length = 0;
	Server.MOTD.length = 0;
	data++; /* protocol version */

	ReadString(&data, &Server.Name);
	ReadString(&data, &Server.MOTD);
	Chat_SetLogName(&Server.Name);

	hacks = &Entities.CurPlayer->Hacks;
	UpdateUserType(hacks, *data);
	
	String_Copy(&hacks->HacksFlags,         &Server.Name);
	String_AppendString(&hacks->HacksFlags, &Server.MOTD);
	HacksComp_RecheckFlags(hacks);
}

static void Classic_Ping(cc_uint8* data) { }

static void Classic_StartLoading(void) {
	World_NewMap();
	LoadingScreen_Show(&Server.Name, &Server.MOTD);
	WoM_CheckMotd();
	classic_receivedFirstPos = false;

	map_begunLoading = true;
	map_receiveBeg   = Stopwatch_Measure();
	map_volume       = 0;

	MapState_Init(&map1);
#ifdef EXTENDED_BLOCKS
	MapState_Init(&map2);
#endif
}

static void Classic_LevelInit(cc_uint8* data) {
	/* in case server is buggy and sends LevelInit multiple times */
	if (map_begunLoading) return;

	Classic_StartLoading();
	if (!IsSupported(fastMap_Ext)) return;

	/* Fast map puts volume in header, and uses raw DEFLATE without GZIP header/footer */
	map_volume = Stream_GetU32_BE(data);
	MapState_SkipHeader(&map1);
#ifdef EXTENDED_BLOCKS
	MapState_SkipHeader(&map2);
#endif
}

static void Classic_LevelDataChunk(cc_uint8* data) {
	struct MapState* m;
	int usedLength;
	float progress;
	cc_result res;

	/* Workaround for some servers that send LevelDataChunk before LevelInit due to their async sending behaviour */
	if (!map_begunLoading) Classic_StartLoading();
	usedLength = Stream_GetU16_BE(data);

	map_part.meta.mem.cur    = data + 2;
	map_part.meta.mem.base   = data + 2;
	map_part.meta.mem.left   = usedLength;
	map_part.meta.mem.length = usedLength;

#ifndef EXTENDED_BLOCKS
	m = &map1;
#else
	/* progress byte in original classic, but we ignore it */
	if (IsSupported(extBlocks_Ext) && data[1026]) {
		m = &map2;
	} else {
		m = &map1;
	}
#endif

	if (!m->gzHeader.done) {
		res = GZipHeader_Read(&map_part, &m->gzHeader);
		if (res && res != ERR_END_OF_STREAM) { DisconnectInvalidMap(res); return; }
	}

	if (m->gzHeader.done) {
		res = MapState_Read(m);
		if (res) { DisconnectInvalidMap(res); return; }
	}

	progress = !map_volume ? 0.0f : (float)map1.index / map_volume;
	Event_RaiseFloat(&WorldEvents.Loading, progress);
}

static void Classic_LevelFinalise(cc_uint8* data) {
	int width, height, length, volume;
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
	volume = width * height * length;

	if (map1.allocFailed) {
		Chat_AddRaw("&cFailed to load map, try joining a different map");
		Chat_AddRaw("   &cNot enough free memory to load the map");
	} else if (!map1.blocks) {
		Chat_AddRaw("&cFailed to load map, try joining a different map");
		Chat_AddRaw("   &cAttempted to load map without a Blocks array");
	} else if (map_volume != volume) {
		Chat_AddRaw("&cFailed to load map, try joining a different map");
		Chat_Add2(  "   &cBlocks array size (%i) does not match volume of map (%i)", &map_volume, &volume);
		FreeMapStates();
	} else if (!World_CheckVolume(width, height, length)) {
		Chat_AddRaw("&cFailed to load map, try joining a different map");
		Chat_AddRaw("   &cAttempted to load map over 2 GB in size");
		FreeMapStates();
	}
	
#ifdef EXTENDED_BLOCKS
	/* defer allocation of second map array if possible */
	if (IsSupported(extBlocks_Ext) && map2.blocks) {
		World_SetMapUpper(map2.blocks);
	}
	map2.blocks = NULL;
#endif
	World_SetNewMap(map1.blocks, width, height, length);
	map1.blocks  = NULL;
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
	TabList_Set(id, &name, &name, &group, 0);
	TabList_EntityLinked_Set(id);
}

static void Classic_EntityTeleport(cc_uint8* data) {
	EntityID id = *data++;
	Classic_ReadAbsoluteLocation(data, id, 
		LU_HAS_POS | LU_HAS_YAW | LU_HAS_PITCH | LU_POS_ABSOLUTE_SMOOTH | LU_ORI_INTERPOLATE);
}

static void Classic_RelPosAndOrientationUpdate(cc_uint8* data) {
	struct LocationUpdate update;
	EntityID id = data[0];

	update.flags = LU_HAS_POS | LU_HAS_YAW | LU_HAS_PITCH | LU_POS_RELATIVE_SMOOTH | LU_ORI_INTERPOLATE;
	update.pos.x = (cc_int8)data[1] / 32.0f;
	update.pos.y = (cc_int8)data[2] / 32.0f;
	update.pos.z = (cc_int8)data[3] / 32.0f;
	update.yaw   = Math_Packed2Deg(data[4]);
	update.pitch = Math_Packed2Deg(data[5]);
	UpdateLocation(id, &update);
}

static void Classic_RelPositionUpdate(cc_uint8* data) {
	struct LocationUpdate update;
	EntityID id = data[0];

	update.flags = LU_HAS_POS | LU_POS_RELATIVE_SMOOTH | LU_ORI_INTERPOLATE;
	update.pos.x = (cc_int8)data[1] / 32.0f;
	update.pos.y = (cc_int8)data[2] / 32.0f;
	update.pos.z = (cc_int8)data[3] / 32.0f;
	UpdateLocation(id, &update);
}

static void Classic_OrientationUpdate(cc_uint8* data) {
	struct LocationUpdate update;
	EntityID id = data[0];

	update.flags = LU_HAS_YAW | LU_HAS_PITCH | LU_ORI_INTERPOLATE;
	update.yaw   = Math_Packed2Deg(data[1]);
	update.pitch = Math_Packed2Deg(data[2]);
	UpdateLocation(id, &update);
}

static void Classic_RemoveEntity(cc_uint8* data) {
	EntityID id = data[0];
	if (id != ENTITIES_SELF_ID) Entities_Remove(id);
}

static void Classic_Message(cc_uint8* data) {
	static const cc_string detailMsg  = String_FromConst("^detail.user=");
	static const cc_string detailUser = String_FromConst("^detail.user");
	cc_string text; char textBuffer[STRING_SIZE + 2];

	cc_uint8 type = *data++;
	String_InitArray(text, textBuffer);

	/* Original vanilla server uses player ids for type, 255 for server messages (&e prefix) */
	if (!IsSupported(messageTypes_Ext)) {
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
	struct HacksComp* hacks = &Entities.CurPlayer->Hacks;
	UpdateUserType(hacks, data[0]);
	HacksComp_RecheckFlags(hacks);
}

static void Classic_ReadAbsoluteLocation(cc_uint8* data, EntityID id, cc_uint8 flags) {
	struct LocationUpdate update;
	int x, y, z;
	cc_uint8 mode;

	if (IsSupported(extEntityPos_Ext)) {
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

	mode = flags & LU_POS_MODEMASK;
	if (mode == LU_POS_ABSOLUTE_SMOOTH || mode == LU_POS_ABSOLUTE_INSTANT) { /* Only perform height shifts on absolute updates */
		y -= 51; /* Convert to feet position */
		/* The original classic client behaves strangely in that */
		/*   Y+0  is sent back to the server for next client->server position update */
		/*   Y+22 is sent back to the server for all subsequent position updates */
		/* so to simplify things, just always add 22 to Y*/
		if (id == ENTITIES_SELF_ID) y += 22;
	}

	update.flags = flags;
	update.pos.x = x/32.0f; 
	update.pos.y = y/32.0f; 
	update.pos.z = z/32.0f;
	update.yaw   = Math_Packed2Deg(*data++);
	update.pitch = Math_Packed2Deg(*data++);

	if (id == ENTITIES_SELF_ID) classic_receivedFirstPos = true;
	UpdateLocation(id, &update);
}

#define Classic_HandshakeSize() (Game_Version.Protocol > PROTOCOL_0019 ? 131 : 130)
static void Classic_Reset(void) {
	Stream_ReadonlyMemory(&map_part, NULL, 0);
	map_begunLoading = false;
	classic_receivedFirstPos = false;

	Net_Set(OPCODE_HANDSHAKE, Classic_Handshake, Classic_HandshakeSize());
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

static cc_uint8* Classic_Tick(cc_uint8* data) {
	struct Entity* e = &Entities.CurPlayer->Base;
	if (!classic_receivedFirstPos) return data;

	/* Report end position of each physics tick, rather than current position */
	/*  (otherwise can miss landing on a block then jumping off of it again) */
	return Classic_WritePosition(data, e->next.pos, e->Yaw, e->Pitch);
}


/*########################################################################################################################*
*------------------------------------------------------CPE protocol-------------------------------------------------------*
*#########################################################################################################################*/
static void CPEExtensions_Reset(void) {
	struct CpeExt* ext;
	int i;

	for (i = 0; i < Array_Elems(cpe_clientExtensions); i++)
	{
		ext = cpe_clientExtensions[i];
		ext->serverVersion = 0;
	}
}

static struct CpeExt* CPEExtensions_Find(const cc_string* name) {
	struct CpeExt* ext;
	int i;

	for (i = 0; i < Array_Elems(cpe_clientExtensions); i++)
	{
		ext = cpe_clientExtensions[i];
		if (String_CaselessEqualsConst(name, ext->name)) return ext;
	}
	return NULL;
}

#define Ext_Deg2Packed(x) ((int)((x) * 65536.0f / 360.0f))
void CPE_SendPlayerClick(int button, cc_bool pressed, cc_uint8 targetId, struct RayTracer* t) {
	struct Entity* p = &Entities.CurPlayer->Base;
	cc_uint8 data[15];

	data[0] = OPCODE_PLAYER_CLICK;
	{
		data[1] = button;
		data[2] = !pressed;
		Stream_SetU16_BE(&data[3], Ext_Deg2Packed(p->Yaw));
		Stream_SetU16_BE(&data[5], Ext_Deg2Packed(p->Pitch));

		data[7] = targetId;
		Stream_SetU16_BE(&data[8],  t->pos.x);
		Stream_SetU16_BE(&data[10], t->pos.y);
		Stream_SetU16_BE(&data[12], t->pos.z);

		data[14] = 255;
		/* FACE enum values differ from CPE block face values */
		switch (t->closest) {
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

void CPE_SendPluginMessage(cc_uint8 channel, cc_uint8* data) {
	cc_uint8 buffer[66];
	if (!IsSupported(pluginMessages_Ext)) return;

	buffer[0] = OPCODE_PLUGIN_MESSAGE;
	{
		buffer[1] = channel;
		Mem_Copy(buffer + 2, data, 64);
	}
	Server.SendData(buffer, 66);
}

void CPE_SendNotifyAction(int action, cc_uint16 value) {
	cc_uint8 data[5];
	if (!Server.SupportsNotifyAction) return;

	data[0] = OPCODE_NOTIFY_ACTION;
	{
		Stream_SetU16_BE(data + 1, action);
		Stream_SetU16_BE(data + 3, value);
	}
	Server.SendData(data, 5);
}

void CPE_SendNotifyPositionAction(int action, int x, int y, int z) {
	cc_uint8 data[9];
	if (!Server.SupportsNotifyAction) return;

	data[0] = OPCODE_NOTIFY_POSITION_ACTION;
	{
		Stream_SetU16_BE(data + 1, action);
		Stream_SetU16_BE(data + 3, x);
		Stream_SetU16_BE(data + 5, y);
		Stream_SetU16_BE(data + 7, z);
	}
	Server.SendData(data, 9);
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

static cc_uint8* CPE_WriteTwoWayPing(cc_uint8* data, cc_bool serverToClient, int id) {
	*data++ = OPCODE_TWO_WAY_PING;
	{
		*data++ = serverToClient;
		Stream_SetU16_BE(data, id); data += 2;
	}
	return data;
}

static void CPE_SendCpeExtInfoReply(void) {
	struct CpeExt* ext;
	int count = Array_Elems(cpe_clientExtensions);
	cc_string name;
	int i, ver;

	if (cpe_serverExtensionsCount) return;

#ifdef EXTENDED_BLOCKS
	if (!Game_AllowCustomBlocks) count -= 3;
#else
	if (!Game_AllowCustomBlocks) count -= 2;
#endif
	CPE_SendExtInfo(count);

	for (i = 0; i < Array_Elems(cpe_clientExtensions); i++) 
	{
		ext  = cpe_clientExtensions[i];
		name = String_FromReadonly(ext->name);
		ver  = ext->serverVersion ? ext->serverVersion : ext->clientVersion;
		/* Don't reply with version higher than what server supports to workaround some buggy server software */

		if (!Game_AllowCustomBlocks) {
			if (String_CaselessEqualsConst(&name, "BlockDefinitionsExt")) continue;
			if (String_CaselessEqualsConst(&name, "BlockDefinitions"))    continue;
#ifdef EXTENDED_BLOCKS
			if (String_CaselessEqualsConst(&name, "ExtendedBlocks"))      continue;
#endif
		}
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
	struct CpeExt* ext;
	cc_string name = UNSAFE_GetString(data);
	int version    = data[67];
	Platform_Log2("cpe ext: %s, %i", &name, &version);

	cpe_serverExtensionsCount--;
	CPE_SendCpeExtInfoReply();

	ext = CPEExtensions_Find(&name);
	if (!ext) return;
	ext->serverVersion = min(ext->clientVersion, version);

	/* update support state */
	if (ext == &extPlayerList_Ext) {
		Server.SupportsExtPlayerList = true;
	} else if (ext == &playerClick_Ext) {
		Server.SupportsPlayerClick = true;
	} else if (ext == &mapAppearance_Ext) {
		if (ext->serverVersion == 1) return;
		Protocol.Sizes[OPCODE_ENV_SET_MAP_APPEARANCE] += 4;
	} else if (ext == &longerMessages_Ext) {
		Server.SupportsPartialMessages = true;
	} else if (ext == &fullCP437_Ext) {
		Server.SupportsFullCP437 = true;
	} else if (ext == &blockDefsExt_Ext) {
		if (ext->serverVersion == 1) return;
		Protocol.Sizes[OPCODE_DEFINE_BLOCK_EXT] += 3;
	} else if (ext == &extEntityPos_Ext) {
		Protocol.Sizes[OPCODE_ENTITY_TELEPORT]     += 6;
		Protocol.Sizes[OPCODE_ADD_ENTITY]          += 6;
		Protocol.Sizes[OPCODE_EXT_ADD_ENTITY2]     += 6;
		Protocol.Sizes[OPCODE_SET_SPAWNPOINT]      += 6;
		Protocol.Sizes[OPCODE_ENTITY_TELEPORT_EXT] += 6;
	} else if (ext == &fastMap_Ext) {
		Protocol.Sizes[OPCODE_LEVEL_BEGIN] += 4;
	} else if (ext == &envMapAspect_Ext) {
		if (ext->serverVersion == 1) return;
		Protocol.Sizes[OPCODE_ENV_SET_MAP_URL] += 64;
	}  else if (ext == &customModels_Ext) {
		if (ext->serverVersion == 2) {
			Protocol.Sizes[OPCODE_DEFINE_MODEL_PART] = 167;
		}
	} else if (ext == &notifyAction_Ext) {
		Server.SupportsNotifyAction = true;
	}
#ifdef EXTENDED_TEXTURES
	else if (ext == &extTextures_Ext) {
		Protocol.Sizes[OPCODE_DEFINE_BLOCK]     += 3;
		Protocol.Sizes[OPCODE_DEFINE_BLOCK_EXT] += 6;
	}
#endif
#ifdef EXTENDED_BLOCKS
	else if (ext == &extBlocks_Ext) {
		if (!Game_AllowCustomBlocks) return;

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

static void CPE_ApplyTexturePack(const cc_string* url) {
	if (!url->length || Utils_IsUrlPrefix(url)) {
		Server_RetrieveTexturePack(url);
	}
	Platform_Log1("Tex url: %s", url);
}


static void CPE_SetClickDistance(cc_uint8* data) {
	Entities.CurPlayer->ReachDistance = Stream_GetU16_BE(data) / 32.0f;
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
	Platform_Log3("CPE hotkey added: %c, %b: %s", Input_DisplayNames[key], &keyMods, &action);

	if (!action.length) {
		Hotkeys_Remove(key, keyMods);
		StoredHotkeys_Load(key, keyMods);
	} else if (action.buffer[action.length - 1] == '\n') {
		action.length--;
		Hotkeys_Add(key, keyMods, &action, 
					HOTKEY_FLAG_AUTO_DEFINED);
	} else { /* more input needed by user */
		Hotkeys_Add(key, keyMods, &action, 
					HOTKEY_FLAG_AUTO_DEFINED | HOTKEY_FLAG_STAYS_OPEN);
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
	TabList_EntityLinked_Reset(id);
	TabList_Set(id, &playerName, &listName, &groupName, groupRank);
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

	p1.x = (cc_int16)Stream_GetU16_BE(data + 65);
	p1.y = (cc_int16)Stream_GetU16_BE(data + 67);
	p1.z = (cc_int16)Stream_GetU16_BE(data + 69);
	p2.x = (cc_int16)Stream_GetU16_BE(data + 71);
	p2.y = (cc_int16)Stream_GetU16_BE(data + 73);
	p2.z = (cc_int16)Stream_GetU16_BE(data + 75);

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
		Env_SetSkyCol(invalid        ? ENV_DEFAULT_SKY_COLOR      : c);
	} else if (variable == 1) {	     
		Env_SetCloudsCol(invalid     ? ENV_DEFAULT_CLOUDS_COLOR   : c);
	} else if (variable == 2) {	     
		Env_SetFogCol(invalid        ? ENV_DEFAULT_FOG_COLOR      : c);
	} else if (variable == 3) {	     
		Env_SetShadowCol(invalid     ? ENV_DEFAULT_SHADOW_COLOR   : c);
	} else if (variable == 4) {	     
		Env_SetSunCol(invalid        ? ENV_DEFAULT_SUN_COLOR      : c);
	} else if (variable == 5) {	     
		Env_SetSkyboxCol(invalid     ? ENV_DEFAULT_SKYBOX_COLOR   : c);
	} else if (variable == 6) {
		Env_SetLavaLightCol(invalid ? ENV_DEFAULT_LAVALIGHT_COLOR : c);
	} else if (variable == 7) {
		Env_SetLampLightCol(invalid ? ENV_DEFAULT_LAMPLIGHT_COLOR : c);
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
	cc_string url = UNSAFE_GetString(data);
	int maxViewDist;
	CPE_ApplyTexturePack(&url);

	Env_SetSidesBlock(data[64]);
	Env_SetEdgeBlock(data[65]);
	Env_SetEdgeHeight((cc_int16)Stream_GetU16_BE(data + 66));
	if (mapAppearance_Ext.serverVersion == 1) return;

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
	struct LocalPlayer* p = Entities.CurPlayer;
	int jumpHeight;

	p->Hacks.CanFly            = data[0] != 0;
	p->Hacks.CanNoclip         = data[1] != 0;
	p->Hacks.CanSpeed          = data[2] != 0;
	p->Hacks.CanRespawn        = data[3] != 0;
	p->Hacks.CanUseThirdPerson = data[4] != 0;
	HacksComp_Update(&p->Hacks);
	jumpHeight = Stream_GetU16_BE(data + 5);

	if (jumpHeight == UInt16_MaxValue) { /* special value of -1 to reset default */
		LocalPlayer_ResetJumpVelocity(p);
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

	if (IsSupported(extBlocks_Ext)) {
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

	/* disallow space, null, and color code specifiers */
	if (code == '\0' || code == ' ' || code == 0xFF) return;
	if (code == '%'  || code == '&') return;

	Drawer2D.Colors[code] = c;
	Event_RaiseInt(&ChatEvents.ColCodeChanged, code);
}

static void CPE_SetMapEnvUrl(cc_uint8* data) {
	char urlBuffer[URL_MAX_SIZE];
	cc_string url;

	cc_string part1 = UNSAFE_GetString(data);
	String_InitArray(url, urlBuffer);
	String_Copy(&url, &part1);

	/* Version 1 only supports URLs up to 64 characters long */
	/* Version 2 supports URLs up to 128 characters long */
	if (envMapAspect_Ext.serverVersion > 1) {
		cc_string part2 = UNSAFE_GetString(data + 64);
		String_AppendString(&url, &part2);
	}
	CPE_ApplyTexturePack(&url);
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
	struct LocationUpdate update;
	struct Entity* e;
	float scale;

	EntityID id   = data[0];
	cc_uint8 type = data[1];
	int value     = (int)Stream_GetU32_BE(data + 2);

	e = Entities.List[id];
	if (!e) return;

	switch (type) {
	case 0:
		update.flags = LU_HAS_ROTX | LU_ORI_INTERPOLATE;
		update.rotX  = (float)value; break;
	case 1:
		update.flags = LU_HAS_YAW  | LU_ORI_INTERPOLATE;
		update.yaw   = (float)value; break;
	case 2:
		update.flags = LU_HAS_ROTZ | LU_ORI_INTERPOLATE;
		update.rotZ  = (float)value; break;

	case 3:
	case 4:
	case 5:
		scale = value / 1000.0f;
		if (e->Flags & ENTITY_FLAG_MODEL_RESTRICTED_SCALE) {
			Math_Clamp(scale, 0.01f, e->Model->maxScale);
		}

		if (type == 3) e->ModelScale.x = scale;
		if (type == 4) e->ModelScale.y = scale;
		if (type == 5) e->ModelScale.z = scale;

		Entity_UpdateModelBounds(e);
		return;
	default:
		return;
	}
	e->VTABLE->SetLocation(e, &update);
}

static void CPE_TwoWayPing(cc_uint8* data) {
	cc_uint8 serverToClient = data[0];
	int id = Stream_GetU16_BE(data + 1);
	cc_uint8 tmp[32];

	/* handle client>server ping response from server */
	if (!serverToClient) { Ping_Update(id); return; }

	/* send server>client ping response to server */
	data = CPE_WriteTwoWayPing(tmp, true, id);
	Server.SendData(tmp, (cc_uint32)(data - tmp));
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
	struct LocalPlayer* p = Entities.CurPlayer;
	int x, y, z;

	if (IsSupported(extEntityPos_Ext)) {
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

	if (*vel < -1024) *vel = -1024;
	if (*vel > +1024) *vel = +1024;
}

static void CPE_VelocityControl(cc_uint8* data) {
	struct LocalPlayer* p = Entities.CurPlayer;
	CalcVelocity(&p->Base.Velocity.x, data + 0, data[12]);
	CalcVelocity(&p->Base.Velocity.y, data + 4, data[13]);
	CalcVelocity(&p->Base.Velocity.z, data + 8, data[14]);
}

static void CPE_DefineEffect(cc_uint8* data) {
	struct CustomParticleEffect* e = &Particles_CustomEffects[data[0]];

	/* e.g. bounds of 0,0, 15,15 gives an 8x8 icon in the default 128x128 particles.png */
	e->rec.u1 = data[1]       / 256.0f;
	e->rec.v1 = data[2]       / 256.0f;
	e->rec.u2 = (data[3] + 1) / 256.0f;
	e->rec.v2 = (data[4] + 1) / 256.0f;

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

static void CPE_PluginMessage(cc_uint8* data) {
	cc_uint8 channel = data[0];
	Event_RaisePluginMessage(&NetEvents.PluginMessageReceived, channel, data + 1);
}

static void CPE_ExtEntityTeleport(cc_uint8* data) {
	EntityID id = *data++;
	cc_uint8 packetFlags = *data++;
	cc_uint8 flags = 0;

	/* bit  0    includes position */
	/* bits 1-2  position mode(absolute_instant / absolute_smooth / relative_smooth / relative_seamless) */
	/* bit  3    unused */
	/* bit  4    includes orientation */
	/* bit  5    interpolate ori */
	/* bit  6-7  unused */

	if (packetFlags & 1) flags |= LU_HAS_POS;
	flags |= (packetFlags & 6) << 4; /* bit-and with 00000110 to isolate only pos mode, then left shift by 4 to match client mode offset */
	if (packetFlags & 16) flags |= LU_HAS_PITCH | LU_HAS_YAW;
	if (packetFlags & 32) flags |= LU_ORI_INTERPOLATE;

	Classic_ReadAbsoluteLocation(data, id, flags);
}

static void CPE_LightingMode(cc_uint8* data) {
	cc_uint8  mode = *data++;
	cc_bool locked = *data++ != 0;

	if (mode == 0) {
		if (!Lighting_ModeSetByServer) return;
		/* locked is ignored with mode 0 and always set to false */
		Lighting_ModeLockedByServer = false;
		Lighting_ModeSetByServer    = false;

		Lighting_SetMode(Lighting_ModeUserCached, true);
		return;
	}
	/* Convert from Network mode (0 = no change, 1 = classic, 2 = fancy) to client mode (0 = classic, 1 = fancy) */
	mode--;
	if (mode >= LIGHTING_MODE_COUNT) return;

	if (!Lighting_ModeSetByServer) Lighting_ModeUserCached = Lighting_Mode;
	Lighting_ModeLockedByServer = locked;
	Lighting_ModeSetByServer    = true;

	Lighting_SetMode(mode, true);
}

static void CPE_CinematicGui(cc_uint8* data) {
	cc_bool hideCrosshair = data[0];
	cc_bool hideHand = data[1];
	cc_bool hideHotbar = data[2];
	cc_uint16 barSize = Stream_GetU16_BE(data + 7);

	HeldBlockRenderer_Show = !hideHand && Options_GetBool(OPT_SHOW_BLOCK_IN_HAND, true);
	Gui.HideCrosshair = hideCrosshair;
	Gui.HideHotbar = hideHotbar;
	Gui.CinematicBarColor = PackedCol_Make(data[3], data[4], data[5], data[6]);
	Gui.BarSize = (float)barSize / UInt16_MaxValue;
}

static void CPE_Reset(void) {
	cpe_serverExtensionsCount = 0; cpe_pingTicks = 0;
	CPEExtensions_Reset();
	cpe_needD3Fix = false;
	Game_UseCPEBlocks = false;
	if (!Game_Version.HasCPE) return;

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
	Net_Set(OPCODE_PLUGIN_MESSAGE, CPE_PluginMessage, 66);
	Net_Set(OPCODE_ENTITY_TELEPORT_EXT, CPE_ExtEntityTeleport, 11);
	Net_Set(OPCODE_LIGHTING_MODE, CPE_LightingMode, 3);
	Net_Set(OPCODE_CINEMATIC_GUI, CPE_CinematicGui, 10);
}

static cc_uint8* CPE_Tick(cc_uint8* data) {
	cpe_pingTicks++;
	if (cpe_pingTicks >= 20 && IsSupported(twoWayPing_Ext)) {
		data = CPE_WriteTwoWayPing(data, false, Ping_NextPingId());
		cpe_pingTicks = 0;
	}
	return data;
}

#ifdef CUSTOM_MODELS
/*########################################################################################################################*
*------------------------------------------------------Custom models------------------------------------------------------*
*#########################################################################################################################*/
static void CPE_DefineModel(cc_uint8* data) {
	struct CustomModel* cm = CustomModel_Get(data[0]);
	cc_string name;
	cc_uint8 flags;
	cc_uint8 numParts;

	if (!cm) return;
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

	cm->collisionBounds.x = GetFloat(data + 74);
	cm->collisionBounds.y = GetFloat(data + 78);
	cm->collisionBounds.z = GetFloat(data + 82);

	cm->pickingBoundsAABB.Min.x = GetFloat(data + 86);
	cm->pickingBoundsAABB.Min.y = GetFloat(data + 90);
	cm->pickingBoundsAABB.Min.z = GetFloat(data + 94);

	cm->pickingBoundsAABB.Max.x = GetFloat(data + 98);
	cm->pickingBoundsAABB.Max.y = GetFloat(data + 102);
	cm->pickingBoundsAABB.Max.z = GetFloat(data + 106);

	cm->uScale = Stream_GetU16_BE(data + 110);
	cm->vScale = Stream_GetU16_BE(data + 112);
	numParts   = data[114];

	if (numParts > MAX_CUSTOM_MODEL_PARTS) {
		int maxParts = MAX_CUSTOM_MODEL_PARTS;
		Chat_Add2("&cCustom Model '%s' exceeds parts limit of %i", &name, &maxParts);
		return;
	}

	cm->numParts = numParts;
	cm->model.vertices = (struct ModelVertex*)Mem_AllocCleared(numParts * MODEL_BOX_VERTICES,
												sizeof(struct ModelVertex), "CustomModel vertices");
	cm->defined = true;
}

static void CPE_DefineModelPart(cc_uint8* data) {
	struct CustomModel* m;
	struct CustomModelPart* part;
	struct CustomModelPartDef p;
	int i;

	m = CustomModel_Get(data[0]);
	if (!m || !m->defined || m->curPartIndex >= m->numParts) return;
	part = &m->parts[m->curPartIndex];

	p.min.x = GetFloat(data +  1);
	p.min.y = GetFloat(data +  5);
	p.min.z = GetFloat(data +  9);
	p.max.x = GetFloat(data + 13);
	p.max.y = GetFloat(data + 17);
	p.max.z = GetFloat(data + 21);

	/* read u, v coords for our 6 faces */
	for (i = 0; i < 6; i++) 
	{
		p.u1[i] = Stream_GetU16_BE(data + 25 + (i*8 + 0));
		p.v1[i] = Stream_GetU16_BE(data + 25 + (i*8 + 2));
		p.u2[i] = Stream_GetU16_BE(data + 25 + (i*8 + 4));
		p.v2[i] = Stream_GetU16_BE(data + 25 + (i*8 + 6));
	}

	p.rotationOrigin.x = GetFloat(data + 73);
	p.rotationOrigin.y = GetFloat(data + 77);
	p.rotationOrigin.z = GetFloat(data + 81);

	part->rotation.x = GetFloat(data + 85);
	part->rotation.y = GetFloat(data + 89);
	part->rotation.z = GetFloat(data + 93);

	if (customModels_Ext.serverVersion == 1) {
		/* ignore animations */
		p.flags = data[102];
	} else {
		p.flags = data[165];

		data += 97;
		for (i = 0; i < MAX_CUSTOM_MODEL_ANIMS; i++) 
		{
			cc_uint8 tmp = *data++;
			part->animType[i] = tmp & 0x3F;
			part->animAxis[i] = tmp >> 6;

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

static void CPE_UndefineModel(cc_uint8* data) {
	struct CustomModel* cm = CustomModel_Get(data[0]);
	if (cm) CustomModel_Undefine(cm);
}

static void CustomModels_Reset(void) {
	if (!Game_Version.HasCPE) return;

	Net_Set(OPCODE_DEFINE_MODEL,      CPE_DefineModel, 116);
	Net_Set(OPCODE_DEFINE_MODEL_PART, CPE_DefineModelPart, 104);
	Net_Set(OPCODE_UNDEFINE_MODEL,    CPE_UndefineModel, 2);
}
#else
static void CustomModels_Reset(void) { }
#endif


/*########################################################################################################################*
*------------------------------------------------------Custom blocks------------------------------------------------------*
*#########################################################################################################################*/
static void BlockDefs_OnBlocksLightPropertyUpdated(BlockID block, cc_bool oldProp) {
	if (!World.Loaded) return;
	/* Need to refresh lighting when a block's light blocking state changes */
	if (Blocks.BlocksLight[block] != oldProp) Lighting.Refresh();
}

static void BlockDefs_OnBrightnessPropertyUpdated(BlockID block, cc_uint8 oldProp) {
	if (!World.Loaded) return;
	if (Lighting_Mode == LIGHTING_MODE_CLASSIC) return;
	/* Need to refresh fancy lighting when a block's brightness changes */
	if (Blocks.Brightness[block] != oldProp) Lighting.Refresh();
}

static TextureLoc BlockDefs_Tex(cc_uint8** ptr) {
	TextureLoc loc; 
	cc_uint8* data = *ptr;

	if (!IsSupported(extTextures_Ext)) {
		loc = *data++;
	} else {
		loc = Stream_GetU16_BE(data) % ATLAS1D_MAX_ATLASES; data += 2;
	}

	*ptr = data; return loc;
}

static BlockID BlockDefs_DefineBlockCommonStart(cc_uint8** ptr, cc_bool uniqueSideTexs) {
	cc_string name;
	BlockID block;
	cc_bool oldBlocksLight;
	cc_uint8 oldBrightness;
	float speedLog2;
	cc_uint8 sound;
	cc_uint8* data = *ptr;

	ReadBlock(data, block);
	oldBlocksLight = Blocks.BlocksLight[block];
	oldBrightness = Blocks.Brightness[block];
	Block_ResetProps(block);
	
	name = UNSAFE_GetString(data); data += STRING_SIZE;
	Block_SetName(block, &name);
	Blocks.Collide[block] = *data++;

	speedLog2 = (*data++ - 128) / 64.0f;
	Blocks.SpeedMultiplier[block] = (float)Math_Exp2(speedLog2);

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
	BlockDefs_OnBlocksLightPropertyUpdated(block, oldBlocksLight);

	sound = *data++;
	Blocks.StepSounds[block] = sound;
	Blocks.DigSounds[block]  = sound;
	if (sound == SOUND_GLASS) Blocks.StepSounds[block] = SOUND_STONE;

	Blocks.Brightness[block] = Block_ReadBrightness(*data++);
	BlockDefs_OnBrightnessPropertyUpdated(block, oldBrightness);
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
}

static void BlockDefs_DefineBlock(cc_uint8* data) {
	BlockID block = BlockDefs_DefineBlockCommonStart(&data, false);

	cc_uint8 shape = *data++;
	if (shape > 0 && shape <= 16) {
		Blocks.MaxBB[block].y = shape / 16.0f;
	}

	BlockDefs_DefineBlockCommonEnd(data, shape, block);
	Block_DefineCustom(block, true);
}

static void BlockDefs_UndefineBlock(cc_uint8* data) {
	BlockID block;
	cc_bool didBlockLight;

	ReadBlock(data, block);
	didBlockLight = Blocks.BlocksLight[block];

	Block_UndefineCustom(block);
	BlockDefs_OnBlocksLightPropertyUpdated(block, didBlockLight);
}

static void BlockDefs_DefineBlockExt(cc_uint8* data) {
	Vec3 minBB, maxBB;
	BlockID block = BlockDefs_DefineBlockCommonStart(&data, 
						blockDefsExt_Ext.serverVersion >= 2);

	minBB.x = (cc_int8)(*data++) / 16.0f;
	minBB.y = (cc_int8)(*data++) / 16.0f;
	minBB.z = (cc_int8)(*data++) / 16.0f;

	maxBB.x = (cc_int8)(*data++) / 16.0f;
	maxBB.y = (cc_int8)(*data++) / 16.0f;
	maxBB.z = (cc_int8)(*data++) / 16.0f;

	Blocks.MinBB[block] = minBB;
	Blocks.MaxBB[block] = maxBB;
	BlockDefs_DefineBlockCommonEnd(data, 1, block);
	Block_DefineCustom(block, false);
}

static void BlockDefs_Reset(void) {
	if (!Game_Version.HasCPE || !Game_AllowCustomBlocks) return;
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
	CustomModels_Reset();
	WoM_Reset();
}

void Protocol_Tick(void) {
	cc_uint8 tmp[256];
	cc_uint8* data = tmp;

	data = Classic_Tick(data);
	data = CPE_Tick(data);
	WoM_Tick();

	/* Have any packets been written? */
	if (data == tmp) return;
	Server.SendData(tmp, (cc_uint32)(data - tmp));
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
#else
void CPE_SendPlayerClick(int button, cc_bool pressed, cc_uint8 targetId, struct RayTracer* t) { }
void CPE_SendNotifyAction(int action, cc_uint16 value) { }
void CPE_SendNotifyPositionAction(int action, int x, int y, int z) { }

static void OnInit(void) { }

static void OnReset(void) { }
#endif

struct IGameComponent Protocol_Component = {
	OnInit,  /* Init  */
	NULL,    /* Free  */
	OnReset, /* Reset */
};
