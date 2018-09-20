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

/*########################################################################################################################*
*-----------------------------------------------------Common handlers-----------------------------------------------------*
*#########################################################################################################################*/
UInt8 classicTabList[256 >> 3];
#define Handlers_ReadBlock(data) *data++;

static void Handlers_ReadString(UInt8** ptr, String* str) {
	Int32 i, length = 0;
	UInt8* data = *ptr;
	for (i = STRING_SIZE - 1; i >= 0; i--) {
		char code = data[i];
		if (code == '\0' || code == ' ') continue;
		length = i + 1; break;
	}

	for (i = 0; i < length; i++) { String_Append(str, data[i]); }
	*ptr = data + STRING_SIZE;
}

static void Handlers_WriteString(UInt8* data, const String* value) {
	Int32 i, count = min(value->length, STRING_SIZE);
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

static void Handlers_AddTablistEntry(EntityID id, const String* playerName, const String* listName, const String* groupName, UInt8 groupRank) {
	/* Only redraw the tab list if something changed. */
	if (TabList_Valid(id)) {
		String oldPlayerName = TabList_UNSAFE_GetPlayer(id);
		String oldListName   = TabList_UNSAFE_GetList(id);
		String oldGroupName  = TabList_UNSAFE_GetList(id);
		UInt8 oldGroupRank   = TabList_GroupRanks[id];

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

static void Classic_ReadAbsoluteLocation(UInt8* data, EntityID id, bool interpolate);
static void Handlers_AddEntity(UInt8* data, EntityID id, const String* displayName, const String* skinName, bool readPosition) {
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
	Int32 mask = id >> 3, bit = 1 << (id & 0x7);
	if (!(classicTabList[mask] & bit)) return;

	Handlers_RemoveTablistEntry(id);
	classicTabList[mask] &= (UInt8)~bit;
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
Int32 wom_counter;
bool wom_sendId, wom_sentId;

static void WoM_UpdateIdentifier(void) {
	wom_identifier.length = 0;
	String_Format1(&wom_identifier, "womenv_%i", &wom_counter);
}

static void WoM_CheckMotd(void) {
	String motd = ServerConnection_ServerMOTD;
	if (!motd.length) return;

	String cfg = String_FromConst("cfg=");
	Int32 index = String_IndexOfString(&motd, &cfg);
	if (Game_PureClassic || index == -1) return;

	char urlBuffer[STRING_SIZE];
	String url  = String_FromArray(urlBuffer);
	String host = String_UNSAFE_SubstringAt(&motd, index + cfg.length);
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
	if (wom_sendId && !wom_sentId) {
		String msg = String_FromConst("/womid WoMClient-2.0.7");
		Chat_Send(&msg, false);
		wom_sentId = true;
	}
}

static PackedCol WoM_ParseCol(const String* value, PackedCol defaultCol) {
	Int32 argb;
	if (!Convert_TryParseInt32(value, &argb)) return defaultCol;

	PackedCol col; col.A = 255;
	col.R = (UInt8)(argb >> 16);
	col.G = (UInt8)(argb >> 8);
	col.B = (UInt8)argb;
	return col;
}

static bool WoM_ReadLine(STRING_REF const String* page, Int32* start, String* line) {
	Int32 i, offset = *start;
	if (offset == -1) return false;

	for (i = offset; i < page->length; i++) {
		char c = page->buffer[i];
		if (c != '\r' && c != '\n') continue;

		*line = String_UNSAFE_Substring(page, offset, i - offset);
		offset = i + 1;
		if (c == '\r' && offset < page->length && page->buffer[offset] == '\n') {
			offset++; /* we stop at the \r, so make sure to skip following \n */
		}

		*start = offset; return true;
	}

	*line = String_UNSAFE_SubstringAt(page, offset);

	*start = -1;
	return true;
}

static void WoM_ParseConfig(const String* page) {
	String line, key, value;
	Int32 start = 0;

	while (WoM_ReadLine(page, &start, &line)) {
		Platform_Log(&line);
		if (!String_UNSAFE_Separate(&line, '=', &key, &value)) continue;

		if (String_CaselessEqualsConst(&key, "environment.cloud")) {
			PackedCol col = WoM_ParseCol(&value, Env_DefaultCloudsCol);
			Env_SetCloudsCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.sky")) {
			PackedCol col = WoM_ParseCol(&value, Env_DefaultSkyCol);
			Env_SetSkyCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.fog")) {
			PackedCol col = WoM_ParseCol(&value, Env_DefaultFogCol);
			Env_SetFogCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.level")) {
			Int32 waterLevel;
			if (Convert_TryParseInt32(&value, &waterLevel)) {
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

	if (item.ResultData) {
		String str = String_Init(item.ResultData, item.ResultSize, item.ResultSize);
		WoM_ParseConfig(&str);
	}
	ASyncRequest_Free(&item);
}


/*########################################################################################################################*
*----------------------------------------------------Classic protocol-----------------------------------------------------*
*#########################################################################################################################*/
UInt64 mapReceiveStart;
struct InflateState mapInflateState;
struct Stream mapInflateStream;
bool mapInflateInited;
struct GZipHeader gzHeader;
Int32 mapSizeIndex, mapIndex, mapVolume;
UInt8 mapSize[4];
UInt8* map;
struct Stream mapPartStream;
struct Screen* prevScreen;
bool receivedFirstPosition;

void Classic_WriteChat(const String* text, bool partial) {
	UInt8* data = ServerConnection_WriteBuffer;
	data[0] = OPCODE_MESSAGE;
	{
		data[1] = ServerConnection_SupportsPartialMessages ? partial : ENTITIES_SELF_ID;
		Handlers_WriteString(&data[2], text);
	}
	ServerConnection_WriteBuffer += 66;
}

void Classic_WritePosition(Vector3 pos, Real32 rotY, Real32 headX) {
	UInt8* data = ServerConnection_WriteBuffer;
	data[0] = OPCODE_ENTITY_TELEPORT;
	Int32 len;
	{
		data[1] = cpe_sendHeldBlock ? Inventory_SelectedBlock : ENTITIES_SELF_ID; /* TODO: extended blocks */
		Int32 x = (Int32)(pos.X * 32);
		Int32 y = (Int32)(pos.Y * 32) + 51;
		Int32 z = (Int32)(pos.Z * 32);

		if (cpe_extEntityPos) {
			Stream_SetU32_BE(&data[2],  x);
			Stream_SetU32_BE(&data[6],  y);
			Stream_SetU32_BE(&data[10], z);
			len = 14;
		} else {
			Stream_SetU16_BE(&data[2], x);
			Stream_SetU16_BE(&data[4], y);
			Stream_SetU16_BE(&data[6], z);
			len = 8;
		}

		data[len++] = Math_Deg2Packed(rotY);
		data[len++] = Math_Deg2Packed(headX);
	}
	ServerConnection_WriteBuffer += len;
}

void Classic_WriteSetBlock(Int32 x, Int32 y, Int32 z, bool place, BlockID block) {
	UInt8* data = ServerConnection_WriteBuffer;
	data[0] = OPCODE_SET_BLOCK_CLIENT;
	{
		Stream_SetU16_BE(&data[1], x);
		Stream_SetU16_BE(&data[3], y);
		Stream_SetU16_BE(&data[5], z);
		data[7] = place;
		data[8] = block; /* TODO: extended blocks */
	}
	ServerConnection_WriteBuffer += 9;
}

void Classic_WriteLogin(const String* username, const String* verKey) {
	UInt8* data = ServerConnection_WriteBuffer;
	data[0] = OPCODE_HANDSHAKE;
	{
		data[1] = 7; /* protocol version */
		Handlers_WriteString(&data[2], username);
		Handlers_WriteString(&data[66], verKey);
		data[130] = Game_UseCPE ? 0x42 : 0x00;
	}
	ServerConnection_WriteBuffer += 131;
}

static void Classic_Handshake(UInt8* data) {
	ServerConnection_ServerName.length = 0;
	ServerConnection_ServerMOTD.length = 0;
	UInt8 protocolVer = *data++;

	Handlers_ReadString(&data, &ServerConnection_ServerName);
	Handlers_ReadString(&data, &ServerConnection_ServerMOTD);
	Chat_SetLogName(&ServerConnection_ServerName);

	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	HacksComp_SetUserType(hacks, *data, !cpe_blockPerms);
	
	String_Copy(&hacks->HacksFlags, &ServerConnection_ServerName);
	String_AppendString(&hacks->HacksFlags, &ServerConnection_ServerMOTD);
	HacksComp_UpdateState(hacks);
}

static void Classic_Ping(UInt8* data) { }

static void Classic_StartLoading(void) {
	World_Reset();
	Event_RaiseVoid(&WorldEvents_NewMap);
	Stream_ReadonlyMemory(&mapPartStream, NULL, 0);

	prevScreen = Gui_Active;
	if (prevScreen == LoadingScreen_UNSAFE_RawPointer) {
		/* otherwise replacing LoadingScreen with LoadingScreen will cause issues */
		Gui_FreeActive();
		prevScreen = NULL;
	}

	Gui_SetActive(LoadingScreen_MakeInstance(&ServerConnection_ServerName, &ServerConnection_ServerMOTD));
	WoM_CheckMotd();
	receivedFirstPosition = false;
	GZipHeader_Init(&gzHeader);

	Inflate_MakeStream(&mapInflateStream, &mapInflateState, &mapPartStream);
	mapInflateInited = true;

	mapSizeIndex = 0;
	mapIndex     = 0;
	mapReceiveStart = DateTime_CurrentUTC_MS();
}

static void Classic_LevelInit(UInt8* data) {
	if (!mapInflateInited) Classic_StartLoading();

	/* Fast map puts volume in header, doesn't bother with gzip */
	if (cpe_fastMap) {
		mapVolume = Stream_GetU32_BE(data);
		gzHeader.Done = true;
		mapSizeIndex = sizeof(UInt32);
		map = Mem_Alloc(mapVolume, sizeof(BlockID), "map blocks");
	}
}

static void Classic_LevelDataChunk(UInt8* data) {
	/* Workaround for some servers that send LevelDataChunk before LevelInit due to their async sending behaviour */
	if (!mapInflateInited) Classic_StartLoading();

	Int32 usedLength = Stream_GetU16_BE(data); data += 2;
	mapPartStream.Meta.Mem.Cur    = data;
	mapPartStream.Meta.Mem.Base   = data;
	mapPartStream.Meta.Mem.Left   = usedLength;
	mapPartStream.Meta.Mem.Length = usedLength;

	data += 1024;
	UInt8 value = *data; /* progress in original classic, but we ignore it */

	if (!gzHeader.Done) { 
		ReturnCode res = GZipHeader_Read(&mapPartStream, &gzHeader);
		if (res && res != ERR_END_OF_STREAM) ErrorHandler_Fail2(res, "reading map data");
	}

	if (gzHeader.Done) {
		if (mapSizeIndex < sizeof(UInt32)) {
			UInt8* src = mapSize + mapSizeIndex;
			UInt32 count = sizeof(UInt32) - mapSizeIndex, modified = 0;
			mapInflateStream.Read(&mapInflateStream, src, count, &modified);
			mapSizeIndex += modified;
		}

		if (mapSizeIndex == sizeof(UInt32)) {
			if (!map) {
				mapVolume = Stream_GetU32_BE(mapSize);
				map = Mem_Alloc(mapVolume, sizeof(BlockID), "map blocks");
			}

			UInt8* src = map + mapIndex;
			UInt32 count = mapVolume - mapIndex, modified = 0;
			mapInflateStream.Read(&mapInflateStream, src, count, &modified);
			mapIndex += modified;
		}
	}

	Real32 progress = !map ? 0.0f : (Real32)mapIndex / mapVolume;
	Event_RaiseReal(&WorldEvents_Loading, progress);
}

static void Classic_LevelFinalise(UInt8* data) {
	Gui_CloseActive();
	Gui_Active = prevScreen;
	prevScreen = NULL;
	Gui_CalcCursorVisible();

	Int32 mapWidth  = Stream_GetU16_BE(&data[0]);
	Int32 mapHeight = Stream_GetU16_BE(&data[2]);
	Int32 mapLength = Stream_GetU16_BE(&data[4]);

	Int32 loadingMs = (Int32)(DateTime_CurrentUTC_MS() - mapReceiveStart);
	Platform_Log1("map loading took: %i", &loadingMs);

	World_SetNewMap(map, mapVolume, mapWidth, mapHeight, mapLength);
	Event_RaiseVoid(&WorldEvents_MapLoaded);
	WoM_CheckSendWomID();

	map = NULL;
	mapInflateInited = false;
}

static void Classic_SetBlock(UInt8* data) {
	Int32 x = Stream_GetU16_BE(&data[0]);
	Int32 y = Stream_GetU16_BE(&data[2]);
	Int32 z = Stream_GetU16_BE(&data[4]);

	data += 6;
	BlockID block = Handlers_ReadBlock(data);
	if (World_IsValidPos(x, y, z)) {
		Game_UpdateBlock(x, y, z, block);
	}
}

static void Classic_AddEntity(UInt8* data) {
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
	classicTabList[id >> 3] |= (UInt8)(1 << (id & 0x7));
}

static void Classic_EntityTeleport(UInt8* data) {
	EntityID id = *data++;
	Classic_ReadAbsoluteLocation(data, id, true);
}

static void Classic_RelPosAndOrientationUpdate(UInt8* data) {
	EntityID id = *data++; Vector3 pos;
	pos.X = (Int8)(*data++) / 32.0f;
	pos.Y = (Int8)(*data++) / 32.0f;
	pos.Z = (Int8)(*data++) / 32.0f;

	Real32 rotY  = Math_Packed2Deg(*data++);
	Real32 headX = Math_Packed2Deg(*data++);
	struct LocationUpdate update; LocationUpdate_MakePosAndOri(&update, pos, rotY, headX, true);
	Handlers_UpdateLocation(id, &update, true);
}

static void Classic_RelPositionUpdate(UInt8* data) {
	EntityID id = *data++; Vector3 pos;
	pos.X = (Int8)(*data++) / 32.0f;
	pos.Y = (Int8)(*data++) / 32.0f;
	pos.Z = (Int8)(*data++) / 32.0f;

	struct LocationUpdate update; LocationUpdate_MakePos(&update, pos, true);
	Handlers_UpdateLocation(id, &update, true);
}

static void Classic_OrientationUpdate(UInt8* data) {
	EntityID id = *data++;
	Real32 rotY  = Math_Packed2Deg(*data++);
	Real32 headX = Math_Packed2Deg(*data++);

	struct LocationUpdate update; LocationUpdate_MakeOri(&update, rotY, headX);
	Handlers_UpdateLocation(id, &update, true);
}

static void Classic_RemoveEntity(UInt8* data) {
	EntityID id = *data;
	Handlers_RemoveEntity(id);
}

static void Classic_Message(UInt8* data) {
	char textBuffer[STRING_SIZE + 2];
	String text = String_FromArray(textBuffer);
	UInt8 type  = *data++;

	/* Original vanilla server uses player ids for type, 255 for server messages (&e prefix) */
	bool prepend = !cpe_useMessageTypes && type == 0xFF;
	if (prepend) { String_AppendConst(&text, "&e"); }
	Handlers_ReadString(&data, &text);
	if (!cpe_useMessageTypes) type = MSG_TYPE_NORMAL;

	/* WoM detail messages (used e.g. for fCraft server compass) */
	String detailMsg = String_FromConst("^detail.user=");
	if (String_CaselessStarts(&text, &detailMsg)) {
		text = String_UNSAFE_SubstringAt(&text, detailMsg.length);
		type = MSG_TYPE_STATUS_3;
	}

	/* Ignore ^detail.user.joined etc */
	String detailUser = String_FromConst("^detail.user");
	if (!String_CaselessStarts(&text, &detailUser)) {
		Chat_AddOf(&text, type); 
	}
}

static void Classic_Kick(UInt8* data) {
	char reasonBuffer[STRING_SIZE];
	String reason = String_FromArray(reasonBuffer);
	String title  = String_FromConst("&eLost connection to the server");

	Handlers_ReadString(&data, &reason);
	Game_Disconnect(&title, &reason);
}

static void Classic_SetPermission(UInt8* data) {
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	HacksComp_SetUserType(hacks, *data, !cpe_blockPerms);
	HacksComp_UpdateState(hacks);
}

static void Classic_ReadAbsoluteLocation(UInt8* data, EntityID id, bool interpolate) {
	Int32 x, y, z;
	if (cpe_extEntityPos) {
		x = (Int32)Stream_GetU32_BE(&data[0]);
		y = (Int32)Stream_GetU32_BE(&data[4]);
		z = (Int32)Stream_GetU32_BE(&data[8]);
		data += 12;
	} else {
		x = (Int16)Stream_GetU16_BE(&data[0]);
		y = (Int16)Stream_GetU16_BE(&data[2]);
		z = (Int16)Stream_GetU16_BE(&data[4]);
		data += 6;
	}

	y -= 51; /* Convert to feet position */
	if (id == ENTITIES_SELF_ID) y += 22;

	Vector3 pos  = { x/32.0f, y/32.0f, z/32.0f };
	Real32 rotY  = Math_Packed2Deg(*data++);
	Real32 headX = Math_Packed2Deg(*data++);

	if (id == ENTITIES_SELF_ID) receivedFirstPosition = true;
	struct LocationUpdate update; LocationUpdate_MakePosAndOri(&update, pos, rotY, headX, false);
	Handlers_UpdateLocation(id, &update, interpolate);
}

static void Classic_Reset(void) {
	mapInflateInited = false;
	receivedFirstPosition = false;

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
	if (!receivedFirstPosition) return;
	struct Entity* entity = &LocalPlayer_Instance.Base;
	Classic_WritePosition(entity->Position, entity->HeadY, entity->HeadX);
}


/*########################################################################################################################*
*------------------------------------------------------CPE protocol-------------------------------------------------------*
*#########################################################################################################################*/

Int32 cpe_serverExtensionsCount, cpe_pingTicks;
Int32 cpe_envMapVer = 2, cpe_blockDefsExtVer = 2;
bool cpe_twoWayPing, cpe_extTextures;

const char* cpe_clientExtensions[29] = {
	"ClickDistance", "CustomBlocks", "HeldBlock", "EmoteFix", "TextHotKey", "ExtPlayerList",
	"EnvColors", "SelectionCuboid", "BlockPermissions", "ChangeModel", "EnvMapAppearance",
	"EnvWeatherType", "MessageTypes", "HackControl", "PlayerClick", "FullCP437", "LongerMessages",
	"BlockDefinitions", "BlockDefinitionsExt", "BulkBlockUpdate", "TextColors", "EnvMapAspect",
	"EntityProperty", "ExtEntityPositions", "TwoWayPing", "InventoryOrder", "InstantMOTD", "FastMap",
	"ExtendedTextures",
};
static void CPE_SetMapEnvUrl(UInt8* data);

#define Ext_Deg2Packed(x) ((Int16)((x) * 65536.0f / 360.0f))
void CPE_WritePlayerClick(MouseButton button, bool buttonDown, UInt8 targetId, struct PickedPos* pos) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	UInt8* data = ServerConnection_WriteBuffer;
	data[0] = OPCODE_PLAYER_CLICK;
	{
		data[1] = button;
		data[2] = buttonDown;
		Stream_SetU16_BE(&data[3], Ext_Deg2Packed(p->HeadY));
		Stream_SetU16_BE(&data[5], Ext_Deg2Packed(p->HeadX));

		data[7] = targetId;
		Stream_SetU16_BE(&data[8],  pos->BlockPos.X);
		Stream_SetU16_BE(&data[10], pos->BlockPos.Y);
		Stream_SetU16_BE(&data[12], pos->BlockPos.Z);

		data[14] = 255;
		/* Our own face values differ from CPE block face */
		switch (pos->ClosestFace) {
		case FACE_XMAX: data[14] = 0; break;
		case FACE_XMIN: data[14] = 1; break;
		case FACE_YMAX: data[14] = 2; break;
		case FACE_YMIN: data[14] = 3; break;
		case FACE_ZMAX: data[14] = 4; break;
		case FACE_ZMIN: data[14] = 5; break;
		}
	}
	ServerConnection_WriteBuffer += 15;
}

static void CPE_WriteExtInfo(const String* appName, Int32 extensionsCount) {
	UInt8* data = ServerConnection_WriteBuffer; 
	data[0] = OPCODE_EXT_INFO;
	{
		Handlers_WriteString(&data[1], appName);
		Stream_SetU16_BE(&data[65], extensionsCount);
	}
	ServerConnection_WriteBuffer += 67;
}

static void CPE_WriteExtEntry(const String* extensionName, Int32 extensionVersion) {
	UInt8* data = ServerConnection_WriteBuffer;
	data[0] = OPCODE_EXT_ENTRY;
	{
		Handlers_WriteString(&data[1], extensionName);
		Stream_SetU32_BE(&data[65], extensionVersion);
	}
	ServerConnection_WriteBuffer += 69;
}

static void CPE_WriteCustomBlockLevel(UInt8 version) {
	UInt8* data = ServerConnection_WriteBuffer;
	data[0] = OPCODE_CUSTOM_BLOCK_LEVEL;
	{
		data[1] = version;
	}
	ServerConnection_WriteBuffer += 2;
}

static void CPE_WriteTwoWayPing(bool serverToClient, UInt16 payload) {
	UInt8* data = ServerConnection_WriteBuffer; 
	data[0] = OPCODE_TWO_WAY_PING;
	{
		data[1] = serverToClient;
		Stream_SetU16_BE(&data[2], payload);
	}
	ServerConnection_WriteBuffer += 4;
}

static void CPE_SendCpeExtInfoReply(void) {
	if (cpe_serverExtensionsCount) return;
	Int32 count = Array_Elems(cpe_clientExtensions);
	if (!Game_AllowCustomBlocks) count -= 2;

	CPE_WriteExtInfo(&ServerConnection_AppName, count);
	Net_SendPacket();
	Int32 i, ver;

	for (i = 0; i < Array_Elems(cpe_clientExtensions); i++) {
		String name = String_FromReadonly(cpe_clientExtensions[i]);
		ver = 1;
		if (String_CaselessEqualsConst(&name, "ExtPlayerList"))       ver = 2;
		if (String_CaselessEqualsConst(&name, "EnvMapAppearance"))    ver = cpe_envMapVer;
		if (String_CaselessEqualsConst(&name, "BlockDefinitionsExt")) ver = cpe_blockDefsExtVer;

		if (!Game_AllowCustomBlocks) {
			if (String_CaselessEqualsConst(&name, "BlockDefinitionsExt")) continue;
			if (String_CaselessEqualsConst(&name, "BlockDefinitions"))    continue;
		}

		CPE_WriteExtEntry(&name, ver);
		Net_SendPacket();
	}
}

static void CPE_ExtInfo(UInt8* data) {
	char appNameBuffer[STRING_SIZE];
	String appName = String_FromArray(appNameBuffer);
	Handlers_ReadString(&data, &appName);
	Chat_Add1("Server software: %s", &appName);

	String d3Server = String_FromConst("D3 server");
	if (String_CaselessStarts(&appName, &d3Server)) {
		cpe_needD3Fix = true;
	}

	/* Workaround for old MCGalaxy that send ExtEntry sync but ExtInfo async. This means
	   ExtEntry may sometimes arrive before ExtInfo, thus have to use += instead of = */
	cpe_serverExtensionsCount += Stream_GetU16_BE(data);
	CPE_SendCpeExtInfoReply();
}

static void CPE_ExtEntry(UInt8* data) {
	char extNameBuffer[STRING_SIZE];
	String ext = String_FromArray(extNameBuffer);
	Handlers_ReadString(&data, &ext);
	Int32 extVersion = Stream_GetU32_BE(data);
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
	} else if (String_CaselessEqualsConst(&ext, "ExtendedTextures")) {
		Net_PacketSizes[OPCODE_DEFINE_BLOCK]     += 3;
		Net_PacketSizes[OPCODE_DEFINE_BLOCK_EXT] += 6;
		cpe_extTextures = true;
	}
}

static void CPE_SetClickDistance(UInt8* data) {
	LocalPlayer_Instance.ReachDistance = Stream_GetU16_BE(data) / 32.0f;
}

static void CPE_CustomBlockLevel(UInt8* data) {
	CPE_WriteCustomBlockLevel(1);
	Net_SendPacket();
	Game_UseCPEBlocks = true;
	Event_RaiseVoid(&BlockEvents_PermissionsChanged);
}

static void CPE_HoldThis(UInt8* data) {
	BlockID block  = Handlers_ReadBlock(data);
	bool canChange = *data == 0;

	Inventory_CanChangeHeldBlock = true;
	Inventory_SetSelectedBlock(block);
	Inventory_CanChangeHeldBlock = canChange;
	Inventory_CanPick = block != BLOCK_AIR;
}

static void CPE_SetTextHotkey(UInt8* data) {
	char actionBuffer[STRING_SIZE];
	String action = String_FromArray(actionBuffer);
	data += STRING_SIZE; /* skip label */
	Handlers_ReadString(&data, &action);

	UInt32 keyCode = Stream_GetU32_BE(data); data += 4;
	UInt8 keyMods  = *data;
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

static void CPE_ExtAddPlayerName(UInt8* data) {
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
	UInt8 groupRank   = *data;

	String_StripCols(&playerName);
	Handlers_RemoveEndPlus(&playerName);
	Handlers_RemoveEndPlus(&listName);

	/* Workarond for server software that declares support for ExtPlayerList, but sends AddEntity then AddPlayerName */
	Int32 mask = id >> 3, bit = 1 << (id & 0x7);
	classicTabList[mask] &= (UInt8)~bit;
	Handlers_AddTablistEntry(id, &playerName, &listName, &groupName, groupRank);
}

static void CPE_ExtAddEntity(UInt8* data) {
	char displayNameBuffer[STRING_SIZE];
	char skinNameBuffer[STRING_SIZE];
	String displayName = String_FromArray(displayNameBuffer);
	String skinName    = String_FromArray(skinNameBuffer);

	UInt8 id = *data++;
	Handlers_ReadString(&data, &displayName);
	Handlers_ReadString(&data, &skinName);

	Handlers_CheckName(id, &displayName, &skinName);
	Handlers_AddEntity(data, id, &displayName, &skinName, false);
}

static void CPE_ExtRemovePlayerName(UInt8* data) {
	EntityID id = data[1];
	Handlers_RemoveTablistEntry(id);
}

static void CPE_MakeSelection(UInt8* data) {
	UInt8 selectionId = *data++;
	data += STRING_SIZE; /* label */

	Vector3I p1;
	p1.X = (Int16)Stream_GetU16_BE(&data[0]);
	p1.Y = (Int16)Stream_GetU16_BE(&data[2]);
	p1.Z = (Int16)Stream_GetU16_BE(&data[4]);

	Vector3I p2; data += 6;
	p2.X = (Int16)Stream_GetU16_BE(&data[0]);
	p2.Y = (Int16)Stream_GetU16_BE(&data[2]);
	p2.Z = (Int16)Stream_GetU16_BE(&data[4]);

	PackedCol c; data += 6;
	/* R,G,B,A are actually 16 bit unsigned integers */
	c.R = data[1]; c.G = data[3]; c.B = data[5]; c.A = data[7];
	Selections_Add(selectionId, p1, p2, c);
}

static void CPE_RemoveSelection(UInt8* data) {
	Selections_Remove(*data);
}

static void CPE_SetEnvCol(UInt8* data) {
	UInt8 variable = *data++;
	UInt16 r = Stream_GetU16_BE(&data[0]);
	UInt16 g = Stream_GetU16_BE(&data[2]);
	UInt16 b = Stream_GetU16_BE(&data[4]);
	bool invalid = r > 255 || g > 255 || b > 255;
	PackedCol col = PACKEDCOL_CONST((UInt8)r, (UInt8)g, (UInt8)b, 255);

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

static void CPE_SetBlockPermission(UInt8* data) {
	BlockID block = Handlers_ReadBlock(data);
	Block_CanPlace[block]  = *data++ != 0;
	Block_CanDelete[block] = *data++ != 0;
	Event_RaiseVoid(&BlockEvents_PermissionsChanged);
}

static void CPE_ChangeModel(UInt8* data) {
	char modelBuffer[STRING_SIZE];
	String model = String_FromArray(modelBuffer);
	UInt8 id = *data++;
	Handlers_ReadString(&data, &model);

	struct Entity* entity = Entities_List[id];
	if (entity) { Entity_SetModel(entity, &model); }
}

static void CPE_EnvSetMapAppearance(UInt8* data) {
	CPE_SetMapEnvUrl(data);
	Env_SetSidesBlock(data[64]);
	Env_SetEdgeBlock(data[65]);
	Env_SetEdgeHeight((Int16)Stream_GetU16_BE(&data[66]));
	if (cpe_envMapVer == 1) return;

	/* Version 2 */
	Env_SetCloudsHeight((Int16)Stream_GetU16_BE(&data[68]));
	Int16 maxViewDist = (Int16)Stream_GetU16_BE(&data[70]);
	Game_MaxViewDistance = maxViewDist <= 0 ? 32768 : maxViewDist;
	Game_SetViewDistance(Game_UserViewDistance);
}

static void CPE_EnvWeatherType(UInt8* data) {
	Env_SetWeather(*data);
}

static void CPE_HackControl(UInt8* data) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	p->Hacks.CanFly                  = *data++ != 0;
	p->Hacks.CanNoclip               = *data++ != 0;
	p->Hacks.CanSpeed                = *data++ != 0;
	p->Hacks.CanRespawn              = *data++ != 0;
	p->Hacks.CanUseThirdPersonCamera = *data++ != 0;
	LocalPlayer_CheckHacksConsistency();

	UInt16 jumpHeight = Stream_GetU16_BE(data);
	struct PhysicsComp* physics = &p->Physics;
	if (jumpHeight == UInt16_MaxValue) { /* special value of -1 to reset default */
		physics->JumpVel = HacksComp_CanJumpHigher(&p->Hacks) ? physics->UserJumpVel : 0.42f;
	} else {
		PhysicsComp_CalculateJumpVelocity(physics, jumpHeight / 32.0f);
	}

	physics->ServerJumpVel = physics->JumpVel;
	Event_RaiseVoid(&UserEvents_HackPermissionsChanged);
}

static void CPE_ExtAddEntity2(UInt8* data) {
	char displayNameBuffer[STRING_SIZE];
	char skinNameBuffer[STRING_SIZE];
	String displayName = String_FromArray(displayNameBuffer);
	String skinName    = String_FromArray(skinNameBuffer);

	UInt8 id = *data++;
	Handlers_ReadString(&data, &displayName);
	Handlers_ReadString(&data, &skinName);

	Handlers_CheckName(id, &displayName, &skinName);
	Handlers_AddEntity(data, id, &displayName, &skinName, true);
}

#define BULK_MAX_BLOCKS 256
static void CPE_BulkBlockUpdate(UInt8* data) {
	Int32 i, count = *data++ + 1;

	UInt32 indices[BULK_MAX_BLOCKS];
	for (i = 0; i < count; i++) {
		indices[i] = Stream_GetU32_BE(data); data += 4;
	}
	data += (BULK_MAX_BLOCKS - count) * sizeof(Int32);

	Int32 x, y, z;
	for (i = 0; i < count; i++) {
		Int32 index = indices[i];
		if (index < 0 || index >= World_BlocksSize) continue;
		World_Unpack(index, x, y, z);

		if (World_IsValidPos(x, y, z)) {
			Game_UpdateBlock(x, y, z, data[i]);
		}
	}
}

static void CPE_SetTextColor(UInt8* data) {
	PackedCol c;
	c.R = *data++; c.G = *data++; c.B = *data++; c.A = *data++;

	UInt8 code = *data;
	/* disallow space, null, and colour code specifiers */
	if (code == '\0' || code == ' ' || code == 0xFF) return;
	if (code == '%' || code == '&') return;

	Drawer2D_Cols[code] = c;
	Event_RaiseInt(&ChatEvents_ColCodeChanged, code);
}

static void CPE_SetMapEnvUrl(UInt8* data) {
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

static void CPE_SetMapEnvProperty(UInt8* data) {
	UInt8 type  = *data++;
	Int32 value = (Int32)Stream_GetU32_BE(data);
	Math_Clamp(value, -0xFFFFFF, 0xFFFFFF);
	Int32 maxBlock = BLOCK_COUNT - 1;

	switch (type) {
	case 0:
		Math_Clamp(value, 0, maxBlock);
		Env_SetSidesBlock((BlockID)value); break;
	case 1:
		Math_Clamp(value, 0, maxBlock);
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

static void CPE_SetEntityProperty(UInt8* data) {
	UInt8 id    = *data++;
	UInt8 type  = *data++;
	Int32 value = (Int32)Stream_GetU32_BE(data);

	struct Entity* entity = Entities_List[id];
	if (!entity) return;
	struct LocationUpdate update = { 0 };

	Real32 scale;
	switch (type) {
	case 0:
		update.Flags |= LOCATIONUPDATE_FLAG_ROTX;
		update.RotX = LocationUpdate_Clamp((Real32)value); break;
	case 1:
		update.Flags |= LOCATIONUPDATE_FLAG_HEADY;
		update.HeadY = LocationUpdate_Clamp((Real32)value); break;
	case 2:
		update.Flags |= LOCATIONUPDATE_FLAG_ROTZ;
		update.RotZ = LocationUpdate_Clamp((Real32)value); break;

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

static void CPE_TwoWayPing(UInt8* data) {
	UInt8 serverToClient = *data++;
	UInt16 payload       = Stream_GetU16_BE(data);

	if (serverToClient) {
		CPE_WriteTwoWayPing(true, payload); /* server to client reply */
		Net_SendPacket();
	} else { PingList_Update(payload); }
}

static void CPE_SetInventoryOrder(UInt8* data) {
	BlockID block = Handlers_ReadBlock(data);
	BlockID order = Handlers_ReadBlock(data);

	Inventory_Remove(block);
	if (order) { Inventory_Map[order - 1] = block; }
}

static void CPE_Reset(void) {
	cpe_serverExtensionsCount = 0; cpe_pingTicks = 0;
	cpe_sendHeldBlock = false; cpe_useMessageTypes = false;
	cpe_envMapVer = 2; cpe_blockDefsExtVer = 2;
	cpe_needD3Fix = false; cpe_extEntityPos = false; cpe_twoWayPing = false; 
	cpe_extTextures = false; cpe_fastMap = false;
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

static TextureLoc BlockDefs_Tex(UInt8** ptr) {
	TextureLoc loc; UInt8* data = *ptr;

	if (!cpe_extTextures) {
		loc = *data++;
	} else {
		loc = Stream_GetU16_BE(data) % ATLAS1D_MAX_ATLASES; data += 2;
	}

	*ptr = data; return loc;
}

static BlockID BlockDefs_DefineBlockCommonStart(UInt8** ptr, bool uniqueSideTexs) {
	UInt8* data = *ptr;
	char nameBuffer[STRING_SIZE];
	String name = String_FromArray(nameBuffer);

	BlockID block = Handlers_ReadBlock(data);
	bool didBlockLight = Block_BlocksLight[block];
	Block_ResetProps(block);
	
	Handlers_ReadString(&data, &name);
	Block_SetName(block, &name);
	Block_SetCollide(block, *data++);

	Real32 multiplierExponent = (*data++ - 128) / 64.0f;
	#define LOG_2 0.693147180559945
	Block_SpeedMultiplier[block] = (Real32)Math_Exp(LOG_2 * multiplierExponent); /* pow(2, x) */

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

	UInt8 sound = *data++;
	Block_StepSounds[block] = sound;
	Block_DigSounds[block] = sound;
	if (sound == SOUND_GLASS) Block_StepSounds[block] = SOUND_STONE;

	Block_FullBright[block] = *data++ != 0;
	*ptr = data;
	return block;
}

static void BlockDefs_DefineBlockCommonEnd(UInt8* data, UInt8 shape, BlockID block) {
	UInt8 blockDraw = *data++;
	if (shape == 0) {
		Block_SpriteOffset[block] = blockDraw;
		blockDraw = DRAW_SPRITE;
	}
	Block_Draw[block] = blockDraw;

	UInt8 density = *data++;
	Block_FogDensity[block] = density == 0 ? 0.0f : (density + 1) / 128.0f;

	PackedCol c;
	c.R = *data++; c.G = *data++; c.B = *data++; c.A = 255;
	Block_FogCol[block] = c;
	Block_DefineCustom(block);
}

static void BlockDefs_DefineBlock(UInt8* data) {
	BlockID block = BlockDefs_DefineBlockCommonStart(&data, false);

	UInt8 shape = *data++;
	if (shape > 0 && shape <= 16) {
		Block_MaxBB[block].Y = shape / 16.0f;
	}

	BlockDefs_DefineBlockCommonEnd(data, shape, block);
	/* Update sprite BoundingBox if necessary */
	if (Block_Draw[block] == DRAW_SPRITE) {
		Block_RecalculateBB(block);
	}
}

static void BlockDefs_UndefineBlock(UInt8* data) {
	BlockID block = Handlers_ReadBlock(data);
	bool didBlockLight = Block_BlocksLight[block];

	Block_ResetProps(block);
	BlockDefs_OnBlockUpdated(block, didBlockLight);
	Block_UpdateCulling(block);

	Inventory_Remove(block);
	if (block < BLOCK_CPE_COUNT) { Inventory_AddDefault(block); }

	Block_SetCustomDefined(block, false);
	Event_RaiseVoid(&BlockEvents_BlockDefChanged);
}

#define BlockDefs_ReadCoord(x) x = *data++ / 16.0f; if (x > 1.0f) x = 1.0f;
static void BlockDefs_DefineBlockExt(UInt8* data) {
	BlockID block = BlockDefs_DefineBlockCommonStart(&data, cpe_blockDefsExtVer >= 2);

	Vector3 minBB;
	BlockDefs_ReadCoord(minBB.X);
	BlockDefs_ReadCoord(minBB.Y);
	BlockDefs_ReadCoord(minBB.Z);

	Vector3 maxBB;
	BlockDefs_ReadCoord(maxBB.X);
	BlockDefs_ReadCoord(maxBB.Y);
	BlockDefs_ReadCoord(maxBB.Z);

	Block_MinBB[block] = minBB;
	Block_MaxBB[block] = maxBB;
	BlockDefs_DefineBlockCommonEnd(data, 1, block);
}

#if FALSE
void HandleDefineModel(UInt8* data) {
	int start = reader.index - 1;
	UInt8 id   = *data++;
	UInt8 type = *data++;
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
	Net_Set(OPCODE_DEFINE_BLOCK, BlockDefs_DefineBlock, 80);
	Net_Set(OPCODE_UNDEFINE_BLOCK, BlockDefs_UndefineBlock, 2);
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
