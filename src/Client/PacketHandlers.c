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
#include "IModel.h"
#include "Funcs.h"
#include "Lighting.h"
#include "AsyncDownloader.h"
#include "Drawer2D.h"
#include "ErrorHandler.h"
#include "TexturePack.h"
#include "Gui.h"

/*########################################################################################################################*
*-----------------------------------------------------Common handlers-----------------------------------------------------*
*#########################################################################################################################*/
UInt8 classicTabList[256 >> 3];

#define Handlers_ReadBlock(stream) Stream_ReadU8(stream)
#define Handlers_WriteBlock(stream, value) Stream_WriteU8(stream, value)

static String Handlers_ReadString(struct Stream* stream, STRING_REF UChar* strBuffer) {
	UChar buffer[STRING_SIZE];
	Stream_Read(stream, buffer, sizeof(buffer));
	Int32 i, length = 0;

	for (i = STRING_SIZE - 1; i >= 0; i--) {
		UChar code = buffer[i];
		if (code == '\0' || code == ' ') continue;
		length = i + 1; break;
	}

	String str = String_InitAndClear(strBuffer, STRING_SIZE);
	for (i = 0; i < length; i++) { String_Append(&str, buffer[i]); }
	return str;
}

static void Handlers_WriteString(struct Stream* stream, STRING_PURE String* value) {
	UChar buffer[STRING_SIZE];

	Int32 i, count = min(value->length, STRING_SIZE);
	for (i = 0; i < count; i++) {
		UChar c = value->buffer[i];
		if (c == '&') c = '%'; /* escape colour codes */
		buffer[i] = c;
	}

	for (; i < STRING_SIZE; i++) { buffer[i] = ' '; }
	Stream_Write(stream, buffer, STRING_SIZE);
}

static void Handlers_RemoveEndPlus(STRING_TRANSIENT String* value) {
	/* Workaround for MCDzienny (and others) use a '+' at the end to distinguish classicube.net accounts */
	/* from minecraft.net accounts. Unfortunately they also send this ending + to the client. */
	if (!value->length) return;
	if (value->buffer[value->length - 1] != '+') return;
	String_DeleteAt(value, value->length - 1);
}

static void Handlers_AddTablistEntry(EntityID id, STRING_PURE String* playerName, STRING_PURE String* listName, STRING_PURE String* groupName, UInt8 groupRank) {
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

static void Handlers_CheckName(EntityID id, STRING_TRANSIENT String* displayName, STRING_TRANSIENT String* skinName) {
	String_StripCols(skinName);
	Handlers_RemoveEndPlus(displayName);
	Handlers_RemoveEndPlus(skinName);
	/* Server is only allowed to change our own name colours. */
	if (id != ENTITIES_SELF_ID) return;

	UChar nameNoColsBuffer[String_BufferSize(STRING_SIZE)];
	String nameNoCols = String_InitAndClearArray(nameNoColsBuffer);
	String_AppendColorless(&nameNoCols, displayName);

	if (!String_Equals(&nameNoCols, &Game_Username)) { String_Set(displayName, &Game_Username); }
	if (!skinName->length) { String_Set(skinName, &Game_Username); }
}

static void Classic_ReadAbsoluteLocation(struct Stream* stream, EntityID id, bool interpolate);
static void Handlers_AddEntity(EntityID id, STRING_TRANSIENT String* displayName, STRING_TRANSIENT String* skinName, bool readPosition) {
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

		String player_name = String_InitAndClearArray(p->DisplayNameRaw);
		String_Set(&player_name, displayName);
		String player_skin = String_InitAndClearArray(p->SkinNameRaw);
		String_Set(&player_skin, skinName);
		Player_UpdateName((struct Player*)p);
	}

	if (!readPosition) return;
	struct Stream* stream = ServerConnection_ReadStream();
	Classic_ReadAbsoluteLocation(stream, id, false);
	if (id != ENTITIES_SELF_ID) return;

	p->Spawn = p->Base.Position;
	p->SpawnRotY = p->Base.HeadY;
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

UChar wom_identifierBuffer[String_BufferSize(STRING_SIZE)];
String wom_identifier = String_FromEmptyArray(wom_identifierBuffer);
Int32 wom_counter;
bool wom_sendId, wom_sentId;

static void WoM_UpdateIdentifier(void) {
	String_Clear(&wom_identifier);
	String_Format1(&wom_identifier, "womenv_%i", &wom_counter);
}

static void WoM_CheckMotd(void) {
	String motd = ServerConnection_ServerMOTD;
	if (!motd.length) return;

	String cfg = String_FromConst("cfg=");
	Int32 index = String_IndexOfString(&motd, &cfg);
	if (Game_PureClassic || index == -1) return;

	UChar urlBuffer[String_BufferSize(STRING_SIZE)];
	String url = String_InitAndClearArray(urlBuffer);
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

static PackedCol WoM_ParseCol(STRING_PURE String* value, PackedCol defaultCol) {
	Int32 argb;
	if (!Convert_TryParseInt32(value, &argb)) return defaultCol;

	PackedCol col; col.A = 255;
	col.R = (UInt8)(argb >> 16);
	col.G = (UInt8)(argb >> 8);
	col.B = (UInt8)argb;
	return col;
}

static bool WoM_ReadLine(STRING_REF String* page, Int32* start, STRING_TRANSIENT String* line) {
	Int32 i, offset = *start;
	if (offset == -1) return false;

	for (i = offset; i < page->length; i++) {
		UChar c = page->buffer[i];
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

static void WoM_ParseConfig(STRING_PURE String* page) {
	String line;
	Int32 start = 0;

	while (WoM_ReadLine(page, &start, &line)) {
		Platform_Log(&line);
		Int32 sepIndex = String_IndexOf(&line, '=', 0);
		if (sepIndex == -1) continue;

		String key = String_UNSAFE_Substring(&line, 0, sepIndex);
		String_UNSAFE_TrimEnd(&key);
		String value = String_UNSAFE_SubstringAt(&line, sepIndex + 1);
		String_UNSAFE_TrimStart(&value);

		if (String_CaselessEqualsConst(&key, "environment.cloud")) {
			PackedCol col = WoM_ParseCol(&value, WorldEnv_DefaultCloudsCol);
			WorldEnv_SetCloudsCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.sky")) {
			PackedCol col = WoM_ParseCol(&value, WorldEnv_DefaultSkyCol);
			WorldEnv_SetSkyCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.fog")) {
			PackedCol col = WoM_ParseCol(&value, WorldEnv_DefaultFogCol);
			WorldEnv_SetFogCol(col);
		} else if (String_CaselessEqualsConst(&key, "environment.level")) {
			Int32 waterLevel;
			if (Convert_TryParseInt32(&value, &waterLevel)) {
				WorldEnv_SetEdgeHeight(waterLevel);
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
DateTime mapReceiveStart;
struct InflateState mapInflateState;
struct Stream mapInflateStream;
bool mapInflateInited;
struct GZipHeader gzHeader;
Int32 mapSizeIndex, mapIndex, mapVolume;
UInt8 mapSize[4];
UInt8* map;
struct Stream mapPartStream;
struct Screen* prevScreen;
bool prevCursorVisible, receivedFirstPosition;

void Classic_WriteChat(struct Stream* stream, STRING_PURE String* text, bool partial) {
	Int32 payload = !ServerConnection_SupportsPartialMessages ? ENTITIES_SELF_ID : (partial ? 1 : 0);
	Stream_WriteU8(stream, OPCODE_MESSAGE);
	Stream_WriteU8(stream, (UInt8)payload);
	Handlers_WriteString(stream, text);
}

void Classic_WritePosition(struct Stream* stream, Vector3 pos, Real32 rotY, Real32 headX) {
	Int32 payload = cpe_sendHeldBlock ? Inventory_SelectedBlock : ENTITIES_SELF_ID;
	Stream_WriteU8(stream, OPCODE_ENTITY_TELEPORT);
	Handlers_WriteBlock(stream, (BlockID)payload); /* held block when using HeldBlock, otherwise just 255 */

	if (cpe_extEntityPos) {
		Stream_WriteI32_BE(stream, (Int32)(pos.X * 32));
		Stream_WriteI32_BE(stream, (Int32)((Int32)(pos.Y * 32) + 51));
		Stream_WriteI32_BE(stream, (Int32)(pos.Z * 32));
	} else {
		Stream_WriteI16_BE(stream, (Int16)(pos.X * 32));
		Stream_WriteI16_BE(stream, (Int16)((Int32)(pos.Y * 32) + 51));
		Stream_WriteI16_BE(stream, (Int16)(pos.Z * 32));
	}
	Stream_WriteU8(stream, Math_Deg2Packed(rotY));
	Stream_WriteU8(stream, Math_Deg2Packed(headX));
}

void Classic_WriteSetBlock(struct Stream* stream, Int32 x, Int32 y, Int32 z, bool place, BlockID block) {
	Stream_WriteU8(stream, OPCODE_SET_BLOCK_CLIENT);
	Stream_WriteU16_BE(stream, x);
	Stream_WriteU16_BE(stream, y);
	Stream_WriteU16_BE(stream, z);
	Stream_WriteU8(stream, place ? 1 : 0);
	Handlers_WriteBlock(stream, block);
}

void Classic_WriteLogin(struct Stream* stream, STRING_PURE String* username, STRING_PURE String* verKey) {
	UInt8 payload = Game_UseCPE ? 0x42 : 0x00;
	Stream_WriteU8(stream, OPCODE_HANDSHAKE);

	Stream_WriteU8(stream, 7); /* protocol version */
	Handlers_WriteString(stream, username);
	Handlers_WriteString(stream, verKey);
	Stream_WriteU8(stream, payload);
}

static void Classic_Handshake(struct Stream* stream) {
	UInt8 protocolVer = Stream_ReadU8(stream);
	ServerConnection_ServerName = Handlers_ReadString(stream, ServerConnection_ServerName.buffer);
	ServerConnection_ServerMOTD = Handlers_ReadString(stream, ServerConnection_ServerMOTD.buffer);
	Chat_SetLogName(&ServerConnection_ServerName);

	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	HacksComp_SetUserType(hacks, Stream_ReadU8(stream), !cpe_blockPerms);
	
	String_Set(&hacks->HacksFlags, &ServerConnection_ServerName);
	String_AppendString(&hacks->HacksFlags, &ServerConnection_ServerMOTD);
	HacksComp_UpdateState(hacks);
}

static void Classic_Ping(struct Stream* stream) { }

static void Classic_StartLoading(struct Stream* stream) {
	World_Reset();
	Event_RaiseVoid(&WorldEvents_NewMap);
	mapPartStream = *stream;

	prevScreen = Gui_Active;
	if (prevScreen == LoadingScreen_UNSAFE_RawPointer) {
		/* otherwise replacing LoadingScreen with LoadingScreen will cause issues */
		Gui_FreeActive();
		prevScreen = NULL;
	}
	prevCursorVisible = Game_GetCursorVisible();

	Gui_SetActive(LoadingScreen_MakeInstance(&ServerConnection_ServerName, &ServerConnection_ServerMOTD));
	WoM_CheckMotd();
	receivedFirstPosition = false;
	GZipHeader_Init(&gzHeader);

	Inflate_MakeStream(&mapInflateStream, &mapInflateState, &mapPartStream);
	mapInflateInited = true;

	mapSizeIndex = 0;
	mapIndex = 0;
	Platform_CurrentUTCTime(&mapReceiveStart);
}

static void Classic_LevelInit(struct Stream* stream) {
	if (!mapInflateInited) Classic_StartLoading(stream);

	/* Fast map puts volume in header, doesn't bother with gzip */
	if (cpe_fastMap) {
		mapVolume = Stream_ReadU32_BE(stream);
		gzHeader.Done = true;
		mapSizeIndex = sizeof(UInt32);
		map = Platform_MemAlloc(mapVolume, sizeof(BlockID));
		if (!map) ErrorHandler_Fail("Failed to allocate memory for map");
	}
}

static void Classic_LevelDataChunk(struct Stream* stream) {
	/* Workaround for some servers that send LevelDataChunk before LevelInit due to their async sending behaviour */
	if (!mapInflateInited) Classic_StartLoading(stream);

	Int32 usedLength = Stream_ReadU16_BE(stream);
	mapPartStream.Meta_Mem_Cur    = stream->Meta_Mem_Cur;
	mapPartStream.Meta_Mem_Base   = stream->Meta_Mem_Cur;
	mapPartStream.Meta_Mem_Left   = usedLength;
	mapPartStream.Meta_Mem_Length = usedLength;

	Stream_Skip(stream, 1024);
	UInt8 value = Stream_ReadU8(stream); /* progress in original classic, but we ignore it */

	if (!gzHeader.Done) { GZipHeader_Read(&mapPartStream, &gzHeader); }
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
				map = Platform_MemAlloc(mapVolume, sizeof(BlockID));
				if (!map) ErrorHandler_Fail("Failed to allocate memory for map");
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

static void Classic_LevelFinalise(struct Stream* stream) {
	Gui_ReplaceActive(NULL);
	Gui_Active = prevScreen;
	if (prevScreen && prevCursorVisible != Game_GetCursorVisible()) {
		Game_SetCursorVisible(prevCursorVisible);
	}
	prevScreen = NULL;

	Int32 mapWidth  = Stream_ReadU16_BE(stream);
	Int32 mapHeight = Stream_ReadU16_BE(stream);
	Int32 mapLength = Stream_ReadU16_BE(stream);

	DateTime now; Platform_CurrentUTCTime(&now);
	Int32 loadingMs = (Int32)DateTime_MsBetween(&mapReceiveStart, &now);
	Platform_Log1("map loading took: %i", &loadingMs);

	World_SetNewMap(map, mapVolume, mapWidth, mapHeight, mapLength);
	Event_RaiseVoid(&WorldEvents_MapLoaded);
	WoM_CheckSendWomID();

	map = NULL;
	mapInflateInited = false;
}

static void Classic_SetBlock(struct Stream* stream) {
	Int32 x = Stream_ReadU16_BE(stream);
	Int32 y = Stream_ReadU16_BE(stream);
	Int32 z = Stream_ReadU16_BE(stream);

	BlockID block = Handlers_ReadBlock(stream);
	if (World_IsValidPos(x, y, z)) {
		Game_UpdateBlock(x, y, z, block);
	}
}

static void Classic_AddEntity(struct Stream* stream) {
	UChar nameBuffer[String_BufferSize(STRING_SIZE)];
	UChar skinBuffer[String_BufferSize(STRING_SIZE)];

	EntityID id = Stream_ReadU8(stream);
	String name = Handlers_ReadString(stream, nameBuffer);
	String skin = String_InitAndClearArray(skinBuffer);

	Handlers_CheckName(id, &name, &skin);
	Handlers_AddEntity(id, &name, &skin, true);

	/* Workaround for some servers that declare support for ExtPlayerList but don't send ExtAddPlayerName */
	String group = String_FromConst("Players");
	Handlers_AddTablistEntry(id, &name, &name, &group, 0);
	classicTabList[id >> 3] |= (UInt8)(1 << (id & 0x7));
}

static void Classic_EntityTeleport(struct Stream* stream) {
	EntityID id = Stream_ReadU8(stream);
	Classic_ReadAbsoluteLocation(stream, id, true);
}

static void Classic_RelPosAndOrientationUpdate(struct Stream* stream) {
	EntityID id = Stream_ReadU8(stream);
	Vector3 pos;
	pos.X = ((Int8)Stream_ReadU8(stream)) / 32.0f;
	pos.Y = ((Int8)Stream_ReadU8(stream)) / 32.0f;
	pos.Z = ((Int8)Stream_ReadU8(stream)) / 32.0f;

	Real32 rotY  = Math_Packed2Deg(Stream_ReadU8(stream));
	Real32 headX = Math_Packed2Deg(Stream_ReadU8(stream));
	struct LocationUpdate update; LocationUpdate_MakePosAndOri(&update, pos, rotY, headX, true);
	Handlers_UpdateLocation(id, &update, true);
}

static void Classic_RelPositionUpdate(struct Stream* stream) {
	EntityID id = Stream_ReadU8(stream);
	Vector3 pos;
	pos.X = ((Int8)Stream_ReadU8(stream)) / 32.0f;
	pos.Y = ((Int8)Stream_ReadU8(stream)) / 32.0f;
	pos.Z = ((Int8)Stream_ReadU8(stream)) / 32.0f;

	struct LocationUpdate update; LocationUpdate_MakePos(&update, pos, true);
	Handlers_UpdateLocation(id, &update, true);
}

static void Classic_OrientationUpdate(struct Stream* stream) {
	EntityID id = Stream_ReadU8(stream);
	Real32 rotY  = Math_Packed2Deg(Stream_ReadU8(stream));
	Real32 headX = Math_Packed2Deg(Stream_ReadU8(stream));

	struct LocationUpdate update; LocationUpdate_MakeOri(&update, rotY, headX);
	Handlers_UpdateLocation(id, &update, true);
}

static void Classic_RemoveEntity(struct Stream* stream) {
	EntityID id = Stream_ReadU8(stream);
	Handlers_RemoveEntity(id);
}

static void Classic_Message(struct Stream* stream) {
	UChar textBuffer[String_BufferSize(STRING_SIZE) + 2];
	UInt8 type = Stream_ReadU8(stream);
	String text = Handlers_ReadString(stream, textBuffer);

	/* Original vanilla server uses player ids for type, 255 for server messages (&e prefix) */
	bool prepend = !cpe_useMessageTypes && type == 0xFF;
	if (prepend) {
		text.capacity += 2;
		String_InsertAt(&text, 0, 'e');
		String_InsertAt(&text, 0, '&');
	}
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

static void Classic_Kick(struct Stream* stream) {
	UChar reasonBuffer[String_BufferSize(STRING_SIZE)];
	String reason = Handlers_ReadString(stream, reasonBuffer);
	String title = String_FromConst("&eLost connection to the server");
	Game_Disconnect(&title, &reason);
}

static void Classic_SetPermission(struct Stream* stream) {
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	HacksComp_SetUserType(hacks, Stream_ReadU8(stream), !cpe_blockPerms);
	HacksComp_UpdateState(hacks);
}

static void Classic_ReadAbsoluteLocation(struct Stream* stream, EntityID id, bool interpolate) {
	Int32 x, y, z;
	if (cpe_extEntityPos) {
		x = Stream_ReadI32_BE(stream); y = Stream_ReadI32_BE(stream); z = Stream_ReadI32_BE(stream);
	} else {
		x = Stream_ReadI16_BE(stream); y = Stream_ReadI16_BE(stream); z = Stream_ReadI16_BE(stream);
	}

	y -= 51; /* Convert to feet position */
	if (id == ENTITIES_SELF_ID) y += 22;

	Vector3 pos  = VECTOR3_CONST(x / 32.0f, y / 32.0f, z / 32.0f);
	Real32 rotY  = Math_Packed2Deg(Stream_ReadU8(stream));
	Real32 headX = Math_Packed2Deg(Stream_ReadU8(stream));

	if (id == ENTITIES_SELF_ID) receivedFirstPosition = true;
	struct LocationUpdate update; LocationUpdate_MakePosAndOri(&update, pos, rotY, headX, false);
	Handlers_UpdateLocation(id, &update, interpolate);
}

static void Classic_Reset(void) {
	mapInflateInited = false;
	receivedFirstPosition = false;

	Net_Set(OPCODE_HANDSHAKE, Classic_Handshake, 131);
	Net_Set(OPCODE_PING, Classic_Ping, 1);
	Net_Set(OPCODE_LEVEL_INIT, Classic_LevelInit, 1);
	Net_Set(OPCODE_LEVEL_DATA_CHUNK, Classic_LevelDataChunk, 1028);
	Net_Set(OPCODE_LEVEL_FINALISE, Classic_LevelFinalise, 7);
	Net_Set(OPCODE_SET_BLOCK, Classic_SetBlock, 8);

	Net_Set(OPCODE_ADD_ENTITY, Classic_AddEntity, 74);
	Net_Set(OPCODE_ENTITY_TELEPORT, Classic_EntityTeleport, 10);
	Net_Set(OPCODE_RELPOS_AND_ORIENTATION_UPDATE, Classic_RelPosAndOrientationUpdate, 7);
	Net_Set(OPCODE_RELPOS_UPDATE, Classic_RelPositionUpdate, 5);
	Net_Set(OPCODE_ORIENTATION_UPDATE, Classic_OrientationUpdate, 4);
	Net_Set(OPCODE_REMOVE_ENTITY, Classic_RemoveEntity, 2);

	Net_Set(OPCODE_MESSAGE, Classic_Message, 66);
	Net_Set(OPCODE_KICK, Classic_Kick, 65);
	Net_Set(OPCODE_SET_PERMISSION, Classic_SetPermission, 2);
}

static void Classic_Tick(void) {
	if (!receivedFirstPosition) return;
	struct Stream* stream = ServerConnection_WriteStream();
	struct Entity* entity = &LocalPlayer_Instance.Base;
	Classic_WritePosition(stream, entity->Position, entity->HeadY, entity->HeadX);
}


/*########################################################################################################################*
*------------------------------------------------------CPE protocol-------------------------------------------------------*
*#########################################################################################################################*/

Int32 cpe_serverExtensionsCount, cpe_pingTicks;
Int32 cpe_envMapVer = 2, cpe_blockDefsExtVer = 2;
bool cpe_twoWayPing;

const UChar* cpe_clientExtensions[28] = {
	"ClickDistance", "CustomBlocks", "HeldBlock", "EmoteFix", "TextHotKey", "ExtPlayerList",
	"EnvColors", "SelectionCuboid", "BlockPermissions", "ChangeModel", "EnvMapAppearance",
	"EnvWeatherType", "MessageTypes", "HackControl", "PlayerClick", "FullCP437", "LongerMessages",
	"BlockDefinitions", "BlockDefinitionsExt", "BulkBlockUpdate", "TextColors", "EnvMapAspect",
	"EntityProperty", "ExtEntityPositions", "TwoWayPing", "InventoryOrder", "InstantMOTD", "FastMap",
};
static void CPE_SetMapEnvUrl(struct Stream* stream);

#define Ext_Deg2Packed(x) ((Int16)((x) * 65536.0f / 360.0f))
void CPE_WritePlayerClick(struct Stream* stream, MouseButton button, bool buttonDown, UInt8 targetId, struct PickedPos* pos) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	Stream_WriteU8(stream, OPCODE_CPE_PLAYER_CLICK);
	Stream_WriteU8(stream, button);
	Stream_WriteU8(stream, buttonDown ? 0 : 1);
	Stream_WriteI16_BE(stream, Ext_Deg2Packed(p->HeadY));
	Stream_WriteI16_BE(stream, Ext_Deg2Packed(p->HeadX));

	Stream_WriteU8(stream, targetId);
	Stream_WriteI16_BE(stream, pos->BlockPos.X);
	Stream_WriteI16_BE(stream, pos->BlockPos.Y);
	Stream_WriteI16_BE(stream, pos->BlockPos.Z);

	UInt8 face = 255;
	/* Our own face values differ from CPE block face */
	switch (pos->ClosestFace) {
	case FACE_XMAX: face = 0; break;
	case FACE_XMIN: face = 1; break;
	case FACE_YMAX: face = 2; break;
	case FACE_YMIN: face = 3; break;
	case FACE_ZMAX: face = 4; break;
	case FACE_ZMIN: face = 5; break;
	}
	Stream_WriteU8(stream, face);
}

static void CPE_WriteExtInfo(struct Stream* stream, STRING_PURE String* appName, Int32 extensionsCount) {
	Stream_WriteU8(stream, OPCODE_CPE_EXT_INFO);
	Handlers_WriteString(stream, appName);
	Stream_WriteU16_BE(stream, extensionsCount);
}

static void CPE_WriteExtEntry(struct Stream* stream, STRING_PURE String* extensionName, Int32 extensionVersion) {
	Stream_WriteU8(stream, OPCODE_CPE_EXT_ENTRY);
	Handlers_WriteString(stream, extensionName);
	Stream_WriteI32_BE(stream, extensionVersion);
}

static void CPE_WriteCustomBlockLevel(struct Stream* stream, UInt8 version) {
	Stream_WriteU8(stream, OPCODE_CPE_CUSTOM_BLOCK_LEVEL);
	Stream_WriteU8(stream, version);
}

static void CPE_WriteTwoWayPing(struct Stream* stream, bool serverToClient, UInt16 data) {
	Stream_WriteU8(stream, OPCODE_CPE_TWO_WAY_PING);
	Stream_WriteU8(stream, serverToClient ? 1 : 0);
	Stream_WriteU16_BE(stream, data);
}

static void CPE_SendCpeExtInfoReply(void) {
	if (cpe_serverExtensionsCount != 0) return;
	Int32 count = Array_Elems(cpe_clientExtensions);
	if (!Game_AllowCustomBlocks) count -= 2;
	struct Stream* stream = ServerConnection_WriteStream();

	CPE_WriteExtInfo(stream, &ServerConnection_AppName, count);
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

		CPE_WriteExtEntry(stream, &name, ver);
		Net_SendPacket();
	}
}

static void CPE_ExtInfo(struct Stream* stream) {
	UChar appNameBuffer[String_BufferSize(STRING_SIZE)];
	String appName = Handlers_ReadString(stream, appNameBuffer);

	UChar logMsgBuffer[String_BufferSize(STRING_SIZE)];
	String logMsg = String_InitAndClearArray(logMsgBuffer);
	String_Format1(&logMsg, "Server software: %s", &appName);
	Chat_Add(&logMsg);

	String d3Server = String_FromConst("D3 server");
	if (String_CaselessStarts(&appName, &d3Server)) {
		cpe_needD3Fix = true;
	}

	/* Workaround for old MCGalaxy that send ExtEntry sync but ExtInfo async. This means
	   ExtEntry may sometimes arrive before ExtInfo, thus have to use += instead of = */
	cpe_serverExtensionsCount += Stream_ReadU16_BE(stream);
	CPE_SendCpeExtInfoReply();
}

static void CPE_ExtEntry(struct Stream* stream) {
	UChar extNameBuffer[String_BufferSize(STRING_SIZE)];
	String ext = Handlers_ReadString(stream, extNameBuffer);
	Int32 extVersion = Stream_ReadI32_BE(stream);
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
		Net_PacketSizes[OPCODE_CPE_ENV_SET_MAP_APPEARANCE] += 4;
	} else if (String_CaselessEqualsConst(&ext, "LongerMessages")) {
		ServerConnection_SupportsPartialMessages = true;
	} else if (String_CaselessEqualsConst(&ext, "FullCP437")) {
		ServerConnection_SupportsFullCP437 = true;
	} else if (String_CaselessEqualsConst(&ext, "BlockDefinitionsExt")) {
		cpe_blockDefsExtVer = extVersion;
		if (extVersion == 1) return;
		Net_PacketSizes[OPCODE_CPE_DEFINE_BLOCK_EXT] += 3;
	} else if (String_CaselessEqualsConst(&ext, "ExtEntityPositions")) {
		Net_PacketSizes[OPCODE_ENTITY_TELEPORT] += 6;
		Net_PacketSizes[OPCODE_ADD_ENTITY] += 6;
		Net_PacketSizes[OPCODE_CPE_EXT_ADD_ENTITY2] += 6;
		cpe_extEntityPos = true;
	} else if (String_CaselessEqualsConst(&ext, "TwoWayPing")) {
		cpe_twoWayPing = true;
	} else if (String_CaselessEqualsConst(&ext, "FastMap")) {
		Net_PacketSizes[OPCODE_LEVEL_INIT] += 4;
		cpe_fastMap = true;
	}
}

static void CPE_SetClickDistance(struct Stream* stream) {
	LocalPlayer_Instance.ReachDistance = Stream_ReadU16_BE(stream) / 32.0f;
}

static void CPE_CustomBlockLevel(struct Stream* stream) {
	UInt8 supportLevel = Stream_ReadU8(stream);
	stream = ServerConnection_WriteStream();
	CPE_WriteCustomBlockLevel(stream, 1);
	Net_SendPacket();
	Game_UseCPEBlocks = true;
	Event_RaiseVoid(&BlockEvents_PermissionsChanged);
}

static void CPE_HoldThis(struct Stream* stream) {
	BlockID block  = Handlers_ReadBlock(stream);
	bool canChange = Stream_ReadU8(stream) == 0;

	Inventory_CanChangeHeldBlock = true;
	Inventory_SetSelectedBlock(block);
	Inventory_CanChangeHeldBlock = canChange;
	Inventory_CanPick = block != BLOCK_AIR;
}

static void CPE_SetTextHotkey(struct Stream* stream) {
	UChar labelBuffer[String_BufferSize(STRING_SIZE)];
	UChar actionBuffer[String_BufferSize(STRING_SIZE)];
	String label  = Handlers_ReadString(stream, labelBuffer);
	String action = Handlers_ReadString(stream, actionBuffer);

	UInt32 keyCode = Stream_ReadU32_BE(stream);
	UInt8 keyMods  = Stream_ReadU8(stream);
	if (keyCode > 255) return;

	Key key = Hotkeys_LWJGL[keyCode];
	if (key == Key_None) return;
	Platform_Log3("CPE hotkey added: %c, %b: %s", Key_Names[key], &keyMods, &action);

	if (!action.length) {
		Hotkeys_Remove(key, keyMods);
	} else if (action.buffer[action.length - 1] == '\n') {
		action = String_UNSAFE_Substring(&action, 0, action.length - 1);
		Hotkeys_Add(key, keyMods, &action, false);
	} else { /* more input needed by user */
		Hotkeys_Add(key, keyMods, &action, true);
	}
}

static void CPE_ExtAddPlayerName(struct Stream* stream) {
	UChar playerNameBuffer[String_BufferSize(STRING_SIZE)];
	UChar listNameBuffer[String_BufferSize(STRING_SIZE)];
	UChar groupNameBuffer[String_BufferSize(STRING_SIZE)];

	Int32 id = Stream_ReadU16_BE(stream) & 0xFF;
	String playerName = Handlers_ReadString(stream, playerNameBuffer);
	String listName   = Handlers_ReadString(stream, listNameBuffer);
	String groupName  = Handlers_ReadString(stream, groupNameBuffer);
	UInt8 groupRank = Stream_ReadU8(stream);

	String_StripCols(&playerName);
	Handlers_RemoveEndPlus(&playerName);
	Handlers_RemoveEndPlus(&listName);

	/* Workarond for server software that declares support for ExtPlayerList, but sends AddEntity then AddPlayerName */
	Int32 mask = id >> 3, bit = 1 << (id & 0x7);
	classicTabList[mask] &= (UInt8)~bit;
	Handlers_AddTablistEntry((EntityID)id, &playerName, &listName, &groupName, groupRank);
}

static void CPE_ExtAddEntity(struct Stream* stream) {
	UChar displayNameBuffer[String_BufferSize(STRING_SIZE)];
	UChar skinNameBuffer[String_BufferSize(STRING_SIZE)];

	UInt8 id = Stream_ReadU8(stream);	
	String displayName = Handlers_ReadString(stream, displayNameBuffer);	
	String skinName    = Handlers_ReadString(stream, skinNameBuffer);

	Handlers_CheckName(id, &displayName, &skinName);
	Handlers_AddEntity(id, &displayName, &skinName, false);
}

static void CPE_ExtRemovePlayerName(struct Stream* stream) {
	Int32 id = Stream_ReadU16_BE(stream) & 0xFF;
	Handlers_RemoveTablistEntry((EntityID)id);
}

static void CPE_MakeSelection(struct Stream* stream) {
	UChar labelBuffer[String_BufferSize(STRING_SIZE)];
	UInt8 selectionId = Stream_ReadU8(stream);
	String label = Handlers_ReadString(stream, labelBuffer);

	Vector3I p1;
	p1.X = Stream_ReadI16_BE(stream);
	p1.Y = Stream_ReadI16_BE(stream);
	p1.Z = Stream_ReadI16_BE(stream);

	Vector3I p2;
	p2.X = Stream_ReadI16_BE(stream);
	p2.Y = Stream_ReadI16_BE(stream);
	p2.Z = Stream_ReadI16_BE(stream);

	PackedCol col;
	col.R = (UInt8)Stream_ReadU16_BE(stream);
	col.G = (UInt8)Stream_ReadU16_BE(stream);
	col.B = (UInt8)Stream_ReadU16_BE(stream);
	col.A = (UInt8)Stream_ReadU16_BE(stream);

	Selections_Add(selectionId, p1, p2, col);
}

static void CPE_RemoveSelection(struct Stream* stream) {
	UInt8 selectionId = Stream_ReadU8(stream);
	Selections_Remove(selectionId);
}

static void CPE_SetEnvCol(struct Stream* stream) {
	UInt8 variable = Stream_ReadU8(stream);
	UInt16 r = Stream_ReadU16_BE(stream);
	UInt16 g = Stream_ReadU16_BE(stream);
	UInt16 b = Stream_ReadU16_BE(stream);
	bool invalid = r > 255 || g > 255 || b > 255;
	PackedCol col = PACKEDCOL_CONST((UInt8)r, (UInt8)g, (UInt8)b, 255);

	if (variable == 0) {
		WorldEnv_SetSkyCol(invalid ? WorldEnv_DefaultSkyCol : col);
	} else if (variable == 1) {
		WorldEnv_SetCloudsCol(invalid ? WorldEnv_DefaultCloudsCol : col);
	} else if (variable == 2) {
		WorldEnv_SetFogCol(invalid ? WorldEnv_DefaultFogCol : col);
	} else if (variable == 3) {
		WorldEnv_SetShadowCol(invalid ? WorldEnv_DefaultShadowCol : col);
	} else if (variable == 4) {
		WorldEnv_SetSunCol(invalid ? WorldEnv_DefaultSunCol : col);
	}
}

static void CPE_SetBlockPermission(struct Stream* stream) {
	BlockID block = Handlers_ReadBlock(stream);
	Block_CanPlace[block]  = Stream_ReadU8(stream) != 0;
	Block_CanDelete[block] = Stream_ReadU8(stream) != 0;
	Event_RaiseVoid(&BlockEvents_PermissionsChanged);
}

static void CPE_ChangeModel(struct Stream* stream) {
	UChar modelNameBuffer[String_BufferSize(STRING_SIZE)];
	UInt8 id = Stream_ReadU8(stream);
	String modelName = Handlers_ReadString(stream, modelNameBuffer);

	struct Entity* entity = Entities_List[id];
	if (entity) { Entity_SetModel(entity, &modelName); }
}

static void CPE_EnvSetMapAppearance(struct Stream* stream) {
	CPE_SetMapEnvUrl(stream);
	WorldEnv_SetSidesBlock(Stream_ReadU8(stream));
	WorldEnv_SetEdgeBlock(Stream_ReadU8(stream));
	WorldEnv_SetEdgeHeight(Stream_ReadI16_BE(stream));
	if (cpe_envMapVer == 1) return;

	/* Version 2 */
	WorldEnv_SetCloudsHeight(Stream_ReadI16_BE(stream));
	Int16 maxViewDist = Stream_ReadI16_BE(stream);
	Game_MaxViewDistance = maxViewDist <= 0 ? 32768 : maxViewDist;
	Game_SetViewDistance(Game_UserViewDistance, false);
}

static void CPE_EnvWeatherType(struct Stream* stream) {
	WorldEnv_SetWeather(Stream_ReadU8(stream));
}

static void CPE_HackControl(struct Stream* stream) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	p->Hacks.CanFly                  = Stream_ReadU8(stream) != 0;
	p->Hacks.CanNoclip               = Stream_ReadU8(stream) != 0;
	p->Hacks.CanSpeed                = Stream_ReadU8(stream) != 0;
	p->Hacks.CanRespawn              = Stream_ReadU8(stream) != 0;
	p->Hacks.CanUseThirdPersonCamera = Stream_ReadU8(stream) != 0;
	LocalPlayer_CheckHacksConsistency();

	UInt16 jumpHeight = Stream_ReadU16_BE(stream);
	struct PhysicsComp* physics = &p->Physics;
	if (jumpHeight == UInt16_MaxValue) { /* special value of -1 to reset default */
		physics->JumpVel = HacksComp_CanJumpHigher(&p->Hacks) ? physics->UserJumpVel : 0.42f;
	} else {
		PhysicsComp_CalculateJumpVelocity(physics, jumpHeight / 32.0f);
	}

	physics->ServerJumpVel = physics->JumpVel;
	Event_RaiseVoid(&UserEvents_HackPermissionsChanged);
}

static void CPE_ExtAddEntity2(struct Stream* stream) {
	UChar displayNameBuffer[String_BufferSize(STRING_SIZE)];
	UChar skinNameBuffer[String_BufferSize(STRING_SIZE)];
	UInt8 id = Stream_ReadU8(stream);
	String displayName = Handlers_ReadString(stream, displayNameBuffer);
	String skinName    = Handlers_ReadString(stream, skinNameBuffer);

	Handlers_CheckName(id, &displayName, &skinName);
	Handlers_AddEntity(id, &displayName, &skinName, true);
}

#define BULK_MAX_BLOCKS 256
static void CPE_BulkBlockUpdate(struct Stream* stream) {
	Int32 i, count = Stream_ReadU8(stream) + 1;

	Int32 indices[BULK_MAX_BLOCKS];
	for (i = 0; i < count; i++) {
		indices[i] = Stream_ReadI32_BE(stream);
	}
	Stream_Skip(stream, (BULK_MAX_BLOCKS - count) * (UInt32)sizeof(Int32));

	BlockID blocks[BULK_MAX_BLOCKS];
	UInt8* recvBuffer = stream->Meta_Mem_Cur;
	for (i = 0; i < count; i++) { blocks[i] = recvBuffer[i]; }
	Stream_Skip(stream, BULK_MAX_BLOCKS);

	Int32 x, y, z;
	for (i = 0; i < count; i++) {
		Int32 index = indices[i];
		if (index < 0 || index >= World_BlocksSize) continue;
		World_Unpack(index, x, y, z);

		if (World_IsValidPos(x, y, z)) {
			Game_UpdateBlock(x, y, z, blocks[i]);
		}
	}
}

static void CPE_SetTextColor(struct Stream* stream) {
	PackedCol col;
	col.R = Stream_ReadU8(stream);
	col.G = Stream_ReadU8(stream);
	col.B = Stream_ReadU8(stream);
	col.A = Stream_ReadU8(stream);

	UInt8 code = Stream_ReadU8(stream);
	/* disallow space, null, and colour code specifiers */
	if (code == '\0' || code == ' ' || code == 0xFF) return;
	if (code == '%' || code == '&') return;

	Drawer2D_Cols[code] = col;
	Event_RaiseInt(&ChatEvents_ColCodeChanged, code);
}

static void CPE_SetMapEnvUrl(struct Stream* stream) {
	UChar urlBuffer[String_BufferSize(STRING_SIZE)];
	String url = Handlers_ReadString(stream, urlBuffer);
	if (!Game_AllowServerTextures) return;

	if (!url.length) {
		/* don't extract default texture pack if we can */
		if (World_TextureUrl.length) TexturePack_ExtractDefault();
	} else if (Utils_IsUrlPrefix(&url, 0)) {
		ServerConnection_RetrieveTexturePack(&url);
	}
	Platform_Log1("Image url: %s", &url);
}

static void CPE_SetMapEnvProperty(struct Stream* stream) {
	UInt8 type = Stream_ReadU8(stream);
	Int32 value = Stream_ReadI32_BE(stream);
	Math_Clamp(value, -0xFFFFFF, 0xFFFFFF);
	Int32 maxBlock = BLOCK_COUNT - 1;

	switch (type) {
	case 0:
		Math_Clamp(value, 0, maxBlock);
		WorldEnv_SetSidesBlock((BlockID)value); break;
	case 1:
		Math_Clamp(value, 0, maxBlock);
		WorldEnv_SetEdgeBlock((BlockID)value); break;
	case 2:
		WorldEnv_SetEdgeHeight(value); break;
	case 3:
		WorldEnv_SetCloudsHeight(value); break;
	case 4:
		Math_Clamp(value, -0x7FFF, 0x7FFF);
		Game_MaxViewDistance = value <= 0 ? 32768 : value;
		Game_SetViewDistance(Game_UserViewDistance, false); break;
	case 5:
		WorldEnv_SetCloudsSpeed(value / 256.0f); break;
	case 6:
		WorldEnv_SetWeatherSpeed(value / 256.0f); break;
	case 7:
		Math_Clamp(value, 0, UInt8_MaxValue);
		WorldEnv_SetWeatherFade(value / 128.0f); break;
	case 8:
		WorldEnv_SetExpFog(value != 0); break;
	case 9:
		WorldEnv_SetSidesOffset(value); break;
	case 10:
		WorldEnv_SetSkyboxHorSpeed(value / 1024.0f); break;
	case 11:
		WorldEnv_SetSkyboxVerSpeed(value / 1024.0f); break;
	}
}

static void CPE_SetEntityProperty(struct Stream* stream) {
	UInt8 id = Stream_ReadU8(stream);
	UInt8 type = Stream_ReadU8(stream);
	Int32 value = Stream_ReadI32_BE(stream);

	struct Entity* entity = Entities_List[id];
	if (!entity) return;
	struct LocationUpdate update; LocationUpdate_Empty(&update);

	Real32 scale;
	switch (type) {
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

static void CPE_TwoWayPing(struct Stream* stream) {
	bool serverToClient = Stream_ReadU8(stream) != 0;
	UInt16 data = Stream_ReadU16_BE(stream);
	if (!serverToClient) { PingList_Update(data); return; }

	stream = ServerConnection_WriteStream();
	CPE_WriteTwoWayPing(stream, true, data); /* server to client reply */
	Net_SendPacket();
}

static void CPE_SetInventoryOrder(struct Stream* stream) {
	BlockID block = Handlers_ReadBlock(stream);
	BlockID order = Handlers_ReadBlock(stream);

	Inventory_Remove(block);
	if (order != 255 && order != 0) {
		Inventory_Map[order - 1] = block;
	}
}

static void CPE_Reset(void) {
	cpe_serverExtensionsCount = 0; cpe_pingTicks = 0;
	cpe_sendHeldBlock = false; cpe_useMessageTypes = false;
	cpe_envMapVer = 2; cpe_blockDefsExtVer = 2;
	cpe_needD3Fix = false; cpe_extEntityPos = false; cpe_twoWayPing = false; cpe_fastMap = false;
	Game_UseCPEBlocks = false;
	if (!Game_UseCPE) return;

	Net_Set(OPCODE_CPE_EXT_INFO, CPE_ExtInfo, 67);
	Net_Set(OPCODE_CPE_EXT_ENTRY, CPE_ExtEntry, 69);
	Net_Set(OPCODE_CPE_SET_CLICK_DISTANCE, CPE_SetClickDistance, 3);
	Net_Set(OPCODE_CPE_CUSTOM_BLOCK_LEVEL, CPE_CustomBlockLevel, 2);
	Net_Set(OPCODE_CPE_HOLD_THIS, CPE_HoldThis, 3);
	Net_Set(OPCODE_CPE_SET_TEXT_HOTKEY, CPE_SetTextHotkey, 134);

	Net_Set(OPCODE_CPE_EXT_ADD_PLAYER_NAME, CPE_ExtAddPlayerName, 196);
	Net_Set(OPCODE_CPE_EXT_ADD_ENTITY, CPE_ExtAddEntity, 130);
	Net_Set(OPCODE_CPE_EXT_REMOVE_PLAYER_NAME, CPE_ExtRemovePlayerName, 3);

	Net_Set(OPCODE_CPE_ENV_SET_COLOR, CPE_SetEnvCol, 8);
	Net_Set(OPCODE_CPE_MAKE_SELECTION, CPE_MakeSelection, 86);
	Net_Set(OPCODE_CPE_REMOVE_SELECTION, CPE_RemoveSelection, 2);
	Net_Set(OPCODE_CPE_SET_BLOCK_PERMISSION, CPE_SetBlockPermission, 4);
	Net_Set(OPCODE_CPE_SET_MODEL, CPE_ChangeModel, 66);
	Net_Set(OPCODE_CPE_ENV_SET_MAP_APPEARANCE, CPE_EnvSetMapAppearance, 69);
	Net_Set(OPCODE_CPE_ENV_SET_WEATHER, CPE_EnvWeatherType, 2);
	Net_Set(OPCODE_CPE_HACK_CONTROL, CPE_HackControl, 8);
	Net_Set(OPCODE_CPE_EXT_ADD_ENTITY2, CPE_ExtAddEntity2, 138);

	Net_Set(OPCODE_CPE_BULK_BLOCK_UPDATE, CPE_BulkBlockUpdate, 1282);
	Net_Set(OPCODE_CPE_SET_TEXT_COLOR, CPE_SetTextColor, 6);
	Net_Set(OPCODE_CPE_ENV_SET_MAP_URL, CPE_SetMapEnvUrl, 65);
	Net_Set(OPCODE_CPE_ENV_SET_MAP_PROPERTY, CPE_SetMapEnvProperty, 6);
	Net_Set(OPCODE_CPE_SET_ENTITY_PROPERTY, CPE_SetEntityProperty, 7);
	Net_Set(OPCODE_CPE_TWO_WAY_PING, CPE_TwoWayPing, 4);
	Net_Set(OPCODE_CPE_SET_INVENTORY_ORDER, CPE_SetInventoryOrder, 3);
}

static void CPE_Tick(void) {
	cpe_pingTicks++;
	if (cpe_pingTicks >= 20 && cpe_twoWayPing) {
		struct Stream* stream = ServerConnection_WriteStream();
		CPE_WriteTwoWayPing(stream, false, PingList_NextPingData());
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

static BlockID BlockDefs_DefineBlockCommonStart(struct Stream* stream, bool uniqueSideTexs) {
	BlockID block = Handlers_ReadBlock(stream);
	bool didBlockLight = Block_BlocksLight[block];
	Block_ResetProps(block);

	UChar nameBuffer[String_BufferSize(STRING_SIZE)];
	String name = Handlers_ReadString(stream, nameBuffer);
	Block_SetName(block, &name);
	Block_SetCollide(block, Stream_ReadU8(stream));

	Real32 multiplierExponent = (Stream_ReadU8(stream) - 128) / 64.0f;
	#define LOG_2 0.693147180559945
	Block_SpeedMultiplier[block] = (Real32)Math_Exp(LOG_2 * multiplierExponent); /* pow(2, x) */

	Block_SetTex(Stream_ReadU8(stream), FACE_YMAX, block);
	if (uniqueSideTexs) {
		Block_SetTex(Stream_ReadU8(stream), FACE_XMIN, block);
		Block_SetTex(Stream_ReadU8(stream), FACE_XMAX, block);
		Block_SetTex(Stream_ReadU8(stream), FACE_ZMIN, block);
		Block_SetTex(Stream_ReadU8(stream), FACE_ZMAX, block);
	} else {
		Block_SetSide(Stream_ReadU8(stream), block);
	}
	Block_SetTex(Stream_ReadU8(stream), FACE_YMIN, block);

	Block_BlocksLight[block] = Stream_ReadU8(stream) == 0;
	BlockDefs_OnBlockUpdated(block, didBlockLight);

	UInt8 sound = Stream_ReadU8(stream);
	Block_StepSounds[block] = sound;
	Block_DigSounds[block] = sound;
	if (sound == SOUND_GLASS) Block_StepSounds[block] = SOUND_STONE;

	Block_FullBright[block] = Stream_ReadU8(stream) != 0;
	return block;
}

static void BlockDefs_DefineBlockCommonEnd(struct Stream* stream, UInt8 shape, BlockID block) {
	UInt8 blockDraw = Stream_ReadU8(stream);
	if (shape == 0) {
		Block_SpriteOffset[block] = blockDraw;
		blockDraw = DRAW_SPRITE;
	}
	Block_Draw[block] = blockDraw;
	UInt8 fogDensity = Stream_ReadU8(stream);
	Block_FogDensity[block] = fogDensity == 0 ? 0 : (fogDensity + 1) / 128.0f;

	PackedCol col; col.A = 255;
	col.R = Stream_ReadU8(stream);
	col.G = Stream_ReadU8(stream);
	col.B = Stream_ReadU8(stream);	
	Block_FogCol[block] = col;
	Block_DefineCustom(block);
}

static void BlockDefs_DefineBlock(struct Stream* stream) {
	BlockID block = BlockDefs_DefineBlockCommonStart(stream, false);

	UInt8 shape = Stream_ReadU8(stream);
	if (shape > 0 && shape <= 16) {
		Block_MaxBB[block].Y = shape / 16.0f;
	}

	BlockDefs_DefineBlockCommonEnd(stream, shape, block);
	/* Update sprite BoundingBox if necessary */
	if (Block_Draw[block] == DRAW_SPRITE) {
		Block_RecalculateBB(block);
	}
}

static void BlockDefs_UndefineBlock(struct Stream* stream) {
	BlockID block = Handlers_ReadBlock(stream);
	bool didBlockLight = Block_BlocksLight[block];

	Block_ResetProps(block);
	BlockDefs_OnBlockUpdated(block, didBlockLight);
	Block_UpdateCulling(block);

	Inventory_Remove(block);
	if (block < BLOCK_CPE_COUNT) { Inventory_AddDefault(block); }

	Block_SetCustomDefined(block, false);
	Event_RaiseVoid(&BlockEvents_BlockDefChanged);
}

#define BlockDefs_ReadCoord(x) x = Stream_ReadU8(stream) / 16.0f; if (x > 1.0f) x = 1.0f;
static void BlockDefs_DefineBlockExt(struct Stream* stream) {
	BlockID block = BlockDefs_DefineBlockCommonStart(stream, cpe_blockDefsExtVer >= 2);

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
	BlockDefs_DefineBlockCommonEnd(stream, 1, block);
}

#if FALSE
void HandleDefineModel(void) {
	int start = reader.index - 1;
	UInt8 id = Stream_ReadU8(stream);
	CustomModel model = null;

	switch (Stream_ReadU8(stream)) {
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
	Net_Set(OPCODE_CPE_DEFINE_BLOCK, BlockDefs_DefineBlock, 80);
	Net_Set(OPCODE_CPE_UNDEFINE_BLOCK, BlockDefs_UndefineBlock, 2);
	Net_Set(OPCODE_CPE_DEFINE_BLOCK_EXT, BlockDefs_DefineBlockExt, 85);
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
