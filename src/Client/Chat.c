#include "Chat.h"
#include "Stream.h"
#include "Platform.h"
#include "Event.h"
#include "Game.h"
#include "ErrorHandler.h"
#include "ServerConnection.h"
#include "World.h"
#include "Inventory.h"
#include "Entity.h"
#include "Window.h"
#include "GraphicsAPI.h"
#include "Funcs.h"
#include "Block.h"
#include "EnvRenderer.h"
#include "BordersRenderer.h"

#define CHAT_LOGTIMES_DEF_ELEMS 256
#define CHAT_LOGTIMES_EXPAND_ELEMS 512
Int64 Chat_DefaultLogTimes[CHAT_LOGTIMES_DEF_ELEMS];
Int64* Chat_LogTimes = Chat_DefaultLogTimes;
UInt32 Chat_LogTimesCount = CHAT_LOGTIMES_DEF_ELEMS, Chat_LogTimesUsed;

void Chat_GetLogTime(UInt32 index, Int64* timeMs) {
	if (index >= Chat_LogTimesUsed) ErrorHandler_Fail("Tries to get time past LogTime end");
	*timeMs = Chat_LogTimes[index];
}

void Chat_AppendLogTime(void) {
	DateTime now; Platform_CurrentUTCTime(&now);
	UInt32 count = Chat_LogTimesUsed;

	if (count == Chat_LogTimesCount) {
		StringsBuffer_Resize(&Chat_LogTimes, &Chat_LogTimesCount, sizeof(Int64),
			CHAT_LOGTIMES_DEF_ELEMS, CHAT_LOGTIMES_EXPAND_ELEMS);
	}
	Chat_LogTimes[Chat_LogTimesUsed++] = DateTime_TotalMs(&now);
}

void ChatLine_Make(ChatLine* line, STRING_TRANSIENT String* text) {
	String dst = String_InitAndClearArray(line->Buffer);
	String_AppendString(&dst, text);
	Platform_CurrentUTCTime(&line->Received);
}

UInt8 Chat_LogNameBuffer[String_BufferSize(STRING_SIZE)];
String Chat_LogName = String_FromEmptyArray(Chat_LogNameBuffer);
Stream Chat_LogStream;
DateTime ChatLog_LastLogDate;

void Chat_CloseLog(void) {
	if (Chat_LogStream.Meta_File == NULL) return;
	ReturnCode code = Chat_LogStream.Close(&Chat_LogStream);
	ErrorHandler_CheckOrFail(code, "Chat - closing log file");
}

bool Chat_AllowedLogChar(UInt8 c) {
	return
		c == '{' || c == '}' || c == '[' || c == ']' || c == '(' || c == ')' ||
		(c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void Chat_SetLogName(STRING_PURE String* name) {
	if (Chat_LogName.length > 0) return;
	String_Clear(&Chat_LogName);

	UInt8 noColsBuffer[String_BufferSize(STRING_SIZE)];
	String noColsName = String_InitAndClearArray(noColsBuffer);
	String_AppendColorless(&noColsName, name);

	Int32 i;
	for (i = 0; i < noColsName.length; i++) {
		if (Chat_AllowedLogChar(noColsName.buffer[i])) {
			String_Append(&Chat_LogName, noColsName.buffer[i]);
		}
	}
}

void Chat_OpenLog(DateTime* now) {
	String logsDir = String_FromConst("logs");
	if (!Platform_DirectoryExists(&logsDir)) {
		Platform_DirectoryCreate(&logsDir);
	}

	/* Ensure multiple instances do not end up overwriting each other's log entries. */
	Int32 i, year = now->Year, month = now->Month, day = now->Day;
	for (i = 0; i < 20; i++) {
		UInt8 pathBuffer[String_BufferSize(FILENAME_SIZE)];
		String path = String_InitAndClearArray(pathBuffer);
		String_Format4(&path, "logs%r%p4-%p2-%p2", &Platform_DirectorySeparator, &year, &month, &day);

		if (i > 0) {
			String_Format2(&path, "%s _%i.log", &Chat_LogName, &i);
		} else {
			String_Format1(&path, "%s.log", &Chat_LogName);
		}

		void* file;
		ReturnCode code = Platform_FileAppend(&file, &path);
		if (code != 0 && code != ReturnCode_FileShareViolation) {
			ErrorHandler_FailWithCode(code, "Chat - opening log file");
		}

		Stream_FromFile(&Chat_LogStream, file, &path);
		return;
	}

	Chat_LogStream.Meta_File = NULL;
	String failedMsg = String_FromConst("Failed to open chat log file after 20 tries, giving up");
	ErrorHandler_Log(&failedMsg);
}

void Chat_AppendLog(STRING_PURE String* text) {
	if (Chat_LogName.length == 0 || !Game_ChatLogging) return;
	DateTime now; Platform_CurrentLocalTime(&now);

	if (now.Day != ChatLog_LastLogDate.Day || now.Month != ChatLog_LastLogDate.Month || now.Year != ChatLog_LastLogDate.Year) {
		Chat_CloseLog();
		Chat_OpenLog(&now);
	}

	ChatLog_LastLogDate = now;
	if (Chat_LogStream.Meta_File == NULL) return;
	UInt8 logBuffer[String_BufferSize(STRING_SIZE * 2)];
	String str = String_InitAndClearArray(logBuffer);

	/* [HH:mm:ss] text */
	Int32 hour = now.Hour, minute = now.Minute, second = now.Second;
	String_Format3(&str, "[%p2:%p2:%p2] ", &hour, &minute, &second);
	String_AppendColorless(&str, text);

	Stream_WriteLine(&Chat_LogStream, &str);
}

void Chat_Add(STRING_PURE String* text) { Chat_AddOf(text, MSG_TYPE_NORMAL); }
void Chat_AddOf(STRING_PURE String* text, Int32 msgType) {
	if (msgType == MSG_TYPE_NORMAL) {
		StringsBuffer_Add(&Chat_Log, text);
		Chat_AppendLog(text);
		Chat_AppendLogTime();
	} else if (msgType >= MSG_TYPE_STATUS_1 && msgType <= MSG_TYPE_STATUS_3) {
		ChatLine_Make(&Chat_Status[msgType - MSG_TYPE_STATUS_1], text);
	} else if (msgType >= MSG_TYPE_BOTTOMRIGHT_1 && msgType <= MSG_TYPE_BOTTOMRIGHT_3) {
		ChatLine_Make(&Chat_BottomRight[msgType - MSG_TYPE_BOTTOMRIGHT_1], text);
	} else if (msgType == MSG_TYPE_ANNOUNCEMENT) {
		ChatLine_Make(&Chat_Announcement, text);
	} else if (msgType >= MSG_TYPE_CLIENTSTATUS_1 && msgType <= MSG_TYPE_CLIENTSTATUS_3) {
		ChatLine_Make(&Chat_ClientStatus[msgType - MSG_TYPE_CLIENTSTATUS_1], text);
	}
	Event_RaiseChat(&ChatEvents_ChatReceived, text, msgType);
}


/*########################################################################################################################*
*---------------------------------------------------------Commands--------------------------------------------------------*
*#########################################################################################################################*/
typedef struct ChatCommand_ {
	const UInt8* Name;
	const UInt8* Help[5];
	void (*Execute)(STRING_PURE String* args, UInt32 argsCount);
	bool SingleplayerOnly;
} ChatCommand;
typedef void (*ChatCommandConstructor)(ChatCommand* cmd);

#define COMMANDS_PREFIX "/client"
#define COMMANDS_PREFIX_SPACE "/client "
ChatCommand commands_list[8];
Int32 commands_count;

bool Commands_IsCommandPrefix(STRING_PURE String* input) {
	if (input->length == 0) return false;
	if (ServerConnection_IsSinglePlayer && input->buffer[0] == '/')
		return true;

	String prefixSpace = String_FromConst(COMMANDS_PREFIX_SPACE);
	String prefix      = String_FromConst(COMMANDS_PREFIX);
	return String_CaselessStarts(input, &prefixSpace)
		|| String_CaselessEquals(input, &prefix);
}

void Commands_Register(ChatCommandConstructor constructor) {
	if (commands_count == Array_Elems(commands_list)) {
		ErrorHandler_Fail("Commands_Register - hit max client commands");
	}

	ChatCommand command = { 0 };
	constructor(&command);
	commands_list[commands_count++] = command;
}

void Commands_Log(const UInt8* format, void* a1) {
	UInt8 strBuffer[String_BufferSize(STRING_SIZE * 2)];
	String str = String_InitAndClearArray(strBuffer);
	String_Format1(&str, format, a1);
	Chat_Add(&str);
}

ChatCommand* Commands_GetMatch(STRING_PURE String* cmdName) {
	ChatCommand* match = NULL;
	Int32 i;
	for (i = 0; i < commands_count; i++) {
		ChatCommand* cmd = &commands_list[i];
		String name = String_FromReadonly(cmd->Name);
		if (!String_CaselessStarts(&name, cmdName)) continue;

		if (match != NULL) {
			Commands_Log("&e/client: Multiple commands found that start with: \"&f%s&e\".", cmdName);
			return NULL;
		}
		match = cmd;
	}

	if (match == NULL) {
		Commands_Log("&e/client: Unrecognised command: \"&f%s&e\".", cmdName);
		Chat_AddRaw(tmp, "&e/client: Type &a/client &efor a list of commands.");
		return NULL;
	}
	if (match->SingleplayerOnly && !ServerConnection_IsSinglePlayer) {
		Commands_Log("&e/client: \"&f%s&e\" can only be used in singleplayer.", cmdName);
		return NULL;
	}
	return match;
}

void Commands_PrintDefined(void) {
	UInt8 strBuffer[String_BufferSize(STRING_SIZE)];
	String str = String_InitAndClearArray(strBuffer);
	Int32 i;

	for (i = 0; i < commands_count; i++) {
		ChatCommand* cmd = &commands_list[i];
		String name = String_FromReadonly(cmd->Name);

		if ((str.length + name.length + 2) > str.capacity) {
			Chat_Add(&str);
			String_Clear(&str);
		}
		String_AppendString(&str, &name);
		String_AppendConst(&str, ", ");
	}

	if (str.length > 0) { Chat_Add(&str); }
}

void Commands_Execute(STRING_PURE String* input) {
	String text        = *input;
	String prefixSpace = String_FromConst(COMMANDS_PREFIX_SPACE);
	String prefix      = String_FromConst(COMMANDS_PREFIX);

	if (String_CaselessStarts(&text, &prefixSpace)) { /* /clientcommand args */
		text = String_UNSAFE_SubstringAt(&text, prefixSpace.length);
	} else if (String_CaselessStarts(&text, &prefix)) { /* /client command args */
		text = String_UNSAFE_SubstringAt(&text, prefix.length);
	} else { /* /command args */
		text = String_UNSAFE_SubstringAt(&text, 1);
	}

	if (text.length == 0) { /* only / or /client */
		Chat_AddRaw(tmp1, "&eList of client commands:");
		Commands_PrintDefined();
		Chat_AddRaw(tmp2, "&eTo see help for a command, type &a/client help [cmd name]");
		return;
	}

	String args[10];
	UInt32 argsCount = Array_Elems(args);
	String_UNSAFE_Split(&text, ' ', args, &argsCount);

	ChatCommand* cmd = Commands_GetMatch(&args[0]);
	if (cmd == NULL) return;
	cmd->Execute(args, argsCount);
}


/*########################################################################################################################*
*-------------------------------------------------------Help command------------------------------------------------------*
*#########################################################################################################################*/
void HelpCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	if (argsCount == 1) {
		Chat_AddRaw(tmp1, "&eList of client commands:");
		Commands_PrintDefined();
		Chat_AddRaw(tmp2, "&eTo see help for a command, type /client help [cmd name]");
	} else {
		ChatCommand* cmd = Commands_GetMatch(&args[1]);
		if (cmd == NULL) return;

		Int32 i;
		for (i = 0; i < Array_Elems(cmd->Help); i++) {
			if (cmd->Help[i] == NULL) continue;
			String help = String_FromReadonly(cmd->Help[i]);
			Chat_Add(&help);
		}
	}
}

void HelpCommand_Make(ChatCommand* cmd) {
	cmd->Name    = "Help";
	cmd->Help[0] = "&a/client help [command name]";
	cmd->Help[1] = "&eDisplays the help for the given command.";
	cmd->Execute = HelpCommand_Execute;
}


/*########################################################################################################################*
*------------------------------------------------------GpuInfo command----------------------------------------------------*
*#########################################################################################################################*/
void GpuInfoCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	Int32 i;
	for (i = 0; i < Array_Elems(Gfx_ApiInfo); i++) {
		if (Gfx_ApiInfo[i].length == 0) continue;
		UInt8 msgBuffer[String_BufferSize(STRING_SIZE)];
		String msg = String_InitAndClearArray(msgBuffer);

		String_AppendConst(&msg, "&a");
		String_AppendString(&msg, &Gfx_ApiInfo[i]);
		Chat_Add(&msg);
	}
}

void GpuInfoCommand_Make(ChatCommand* cmd) {
	cmd->Name    = "GpuInfo";
	cmd->Help[0] = "&a/client gpuinfo";
	cmd->Help[1] = "&eDisplays information about your GPU.";
	cmd->Execute = GpuInfoCommand_Execute;
}


/*########################################################################################################################*
*----------------------------------------------------RenderType command---------------------------------------------------*
*#########################################################################################################################*/
void RenderTypeCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	if (argsCount == 1) {
		Chat_AddRaw(tmp, "&e/client: &cYou didn't specify a new render type."); return;
	}

	Int32 flags = Game_CalcRenderType(&args[1]);
	if (flags >= 0) {
		BordersRenderer_UseLegacyMode((flags & 1));
		EnvRenderer_UseLegacyMode(    (flags & 1));
		EnvRenderer_UseMinimalMode(   (flags & 2));

		Options_Set(OPT_RENDER_TYPE, &args[1]);
		Commands_Log("&e/client: &fRender type is now %s.", &args[1]);
	} else {
		Commands_Log("&e/client: &cUnrecognised render type &f\"%s\"&c.", &args[1]);
	}
}

void RenderTypeCommand_Make(ChatCommand* cmd) {
	cmd->Name    = "RenderType";
	cmd->Help[0] = "&a/client rendertype [normal/legacy/legacyfast]";
	cmd->Help[1] = "&bnormal: &eDefault renderer, with all environmental effects enabled.";
	cmd->Help[2] = "&blegacy: &eMay be slightly slower than normal, but produces the same environmental effects.";
	cmd->Help[3] = "&blegacyfast: &eSacrifices clouds, fog and overhead sky for faster performance.";
	cmd->Help[4] = "&bnormalfast: &eSacrifices clouds, fog and overhead sky for faster performance.";
	cmd->Execute = RenderTypeCommand_Execute;
}

void ResolutionCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	Int32 width, height;
	if (argsCount < 3) {
		Chat_AddRaw(tmp, "&e/client: &cYou didn't specify width and height");
	} else if (!Convert_TryParseInt32(&args[1], &width) || !Convert_TryParseInt32(&args[2], &height)) {
		Chat_AddRaw(tmp, "&e/client: &cWidth and height must be integers.");
	} else if (width <= 0 || height <= 0) {
		Chat_AddRaw(tmp, "&e/client: &cWidth and height must be above 0.");
	} else {
		Size2D size = { width, height };
		Window_SetClientSize(size);
		Options_SetInt32(OPT_WINDOW_WIDTH, width);
		Options_SetInt32(OPT_WINDOW_HEIGHT, height);
	}
}

void ResolutionCommand_Make(ChatCommand* cmd) {
	cmd->Name    = "Resolution";
	cmd->Help[0] = "&a/client resolution [width] [height]";
	cmd->Help[1] = "&ePrecisely sets the size of the rendered window.";
	cmd->Execute = ResolutionCommand_Execute;
}

void ModelCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	if (argsCount == 1) {
		Chat_AddRaw(tmp, "&e/client model: &cYou didn't specify a model name.");
	} else {
		UInt8 modelBuffer[String_BufferSize(STRING_SIZE)];
		String model = String_InitAndClearArray(modelBuffer);
		String_AppendString(&model, &args[1]);
		String_MakeLowercase(&model);
		Entity_SetModel(&LocalPlayer_Instance.Base, &model);
	}
}

void ModelCommand_Make(ChatCommand* cmd) {
	cmd->Name    = "Model";
	cmd->Help[0] = "&a/client model [name]";
	cmd->Help[1] = "&bnames: &echibi, chicken, creeper, human, pig, sheep";
	cmd->Help[2] = "&e       skeleton, spider, zombie, sitting, <numerical block id>";
	cmd->SingleplayerOnly = true;
	cmd->Execute = ModelCommand_Execute;
}


/*########################################################################################################################*
*-------------------------------------------------------CuboidCommand-----------------------------------------------------*
*#########################################################################################################################*/
Int32 cuboid_block = -1;
Vector3I cuboid_mark1, cuboid_mark2;
bool cuboid_persist, cuboid_hooked;

bool CuboidCommand_ParseBlock(STRING_PURE String* args, UInt32 argsCount) {
	if (argsCount == 1) return true;
	if (String_CaselessEqualsConst(&args[1], "yes")) { cuboid_persist = true; return true; }

	Int32 temp = Block_FindID(&args[1]);
	BlockID block = 0;

	if (temp != -1) {
		block = (BlockID)temp;
	} else {
		#if USE16_BIT
		if (!Convert_TryParseUInt16(&args[1], &block)) {
		#else
		if (!Convert_TryParseUInt8(&args[1], &block)) {
		#endif		
			Commands_Log("&eCuboid: &c\"%s\" is not a valid block name or id.", &args[1]); return false;
		}
	}

	if (block >= BLOCK_CPE_COUNT && !Block_IsCustomDefined(block)) {
		Commands_Log("&eCuboid: &cThere is no block with id \"%s\".", &args[1]); return false;
	}

	cuboid_block = block;
	return true;
}

void CuboidCommand_DoCuboid(void) {
	Vector3I min, max;
	Vector3I_Min(&min, &cuboid_mark1, &cuboid_mark2);
	Vector3I_Max(&max, &cuboid_mark1, &cuboid_mark2);
	if (!World_IsValidPos_3I(min) || !World_IsValidPos_3I(max)) return;

	BlockID toPlace = (BlockID)cuboid_block;
	if (cuboid_block == -1) toPlace = Inventory_SelectedBlock;

	for (Int32 y = min.Y; y <= max.Y; y++) {
		for (Int32 z = min.Z; z <= max.Z; z++) {
			for (Int32 x = min.X; x <= max.X; x++) {
				Game_UpdateBlock(x, y, z, toPlace);
			}
		}
	}
}

void CuboidCommand_BlockChanged(void* obj, Vector3I coords, BlockID oldBlock, BlockID block) {
	if (cuboid_mark1.X == Int32_MaxValue) {
		cuboid_mark1 = coords;
		Game_UpdateBlock(coords.X, coords.Y, coords.Z, oldBlock);
		UInt8 msgBuffer[String_BufferSize(STRING_SIZE)];
		String msg = String_InitAndClearArray(msgBuffer);

		String_Format3(&msg, "&eCuboid: &fMark 1 placed at (%i, %i, %i), place mark 2.", &coords.X, &coords.Y, &coords.Z);
		Chat_AddOf(&msg, MSG_TYPE_CLIENTSTATUS_1);
	} else {
		cuboid_mark2 = coords;
		CuboidCommand_DoCuboid();

		if (!cuboid_persist) {
			Event_UnregisterBlock(&UserEvents_BlockChanged, NULL, CuboidCommand_BlockChanged);
			cuboid_hooked = false;
			String empty = String_MakeNull(); Chat_AddOf(&empty, MSG_TYPE_CLIENTSTATUS_1);
		} else {
			cuboid_mark1 = Vector3I_MaxValue();
			String msg = String_FromConst("&eCuboid: &fPlace or delete a block.");
			Chat_AddOf(&msg, MSG_TYPE_CLIENTSTATUS_1);
		}
	}
}

void CuboidCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	if (cuboid_hooked) {
		Event_UnregisterBlock(&UserEvents_BlockChanged, NULL, CuboidCommand_BlockChanged);
		cuboid_hooked = false;
	}

	cuboid_block = -1;
	cuboid_mark1 = Vector3I_MaxValue();
	cuboid_mark2 = Vector3I_MaxValue();
	cuboid_persist = false;

	if (!CuboidCommand_ParseBlock(args, argsCount)) return;
	if (argsCount > 2 && String_CaselessEqualsConst(&args[2], "yes")) {
		cuboid_persist = true;
	}

	String msg = String_FromConst("&eCuboid: &fPlace or delete a block.");
	Chat_AddOf(&msg, MSG_TYPE_CLIENTSTATUS_1);
	Event_RegisterBlock(&UserEvents_BlockChanged, NULL, CuboidCommand_BlockChanged);
	cuboid_hooked = true;
}

void CuboidCommand_Make(ChatCommand* cmd) {
	cmd->Name    = "Cuboid";
	cmd->Help[0] = "&a/client cuboid [block] [persist]";
	cmd->Help[1] = "&eFills the 3D rectangle between two points with [block].";
	cmd->Help[2] = "&eIf no block is given, uses your currently held block.";
	cmd->Help[3] = "&e  If persist is given and is \"yes\", then the command";
	cmd->Help[4] = "&e  will repeatedly cuboid, without needing to be typed in again.";
	cmd->SingleplayerOnly = true;
	cmd->Execute = CuboidCommand_Execute;
}


/*########################################################################################################################*
*------------------------------------------------------TeleportCommand----------------------------------------------------*
*#########################################################################################################################*/
void TeleportCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	if (argsCount != 4) {
		Chat_AddRaw(tmp, "&e/client teleport: &cYou didn't specify X, Y and Z coordinates.");
	} else {
		Real32 x, y, z;
		if (!Convert_TryParseReal32(&args[1], &x) || !Convert_TryParseReal32(&args[2], &y) || !Convert_TryParseReal32(&args[3], &z)) {
			Chat_AddRaw(tmp, "&e/client teleport: &cCoordinates must be decimals");
			return;
		}

		Vector3 v = { x, y, z };
		LocationUpdate update; LocationUpdate_MakePos(&update, v, false);
		Entity* entity = &LocalPlayer_Instance.Base;
		entity->VTABLE->SetLocation(entity, &update, false);
	}
}

void TeleportCommand_Make(ChatCommand* cmd) {
	cmd->Name    = "TP";
	cmd->Help[0] = "&a/client tp [x y z]";
	cmd->Help[1] = "&eMoves you to the given coordinates.";
	cmd->SingleplayerOnly = true;
	cmd->Execute = TeleportCommand_Execute;
}


/*########################################################################################################################*
*-------------------------------------------------------Generic chat------------------------------------------------------*
*#########################################################################################################################*/
void Chat_Send(STRING_PURE String* text) {
	if (text->length == 0) return;
	StringsBuffer_Add(&Chat_InputLog, text);

	if (Commands_IsCommandPrefix(text)) {
		Commands_Execute(text);
	} else {
		ServerConnection_SendChat(text);
	}
}

void Chat_Init(void) {
	Commands_Register(GpuInfoCommand_Make);
	Commands_Register(HelpCommand_Make);
	Commands_Register(RenderTypeCommand_Make);
	Commands_Register(ResolutionCommand_Make);
	Commands_Register(ModelCommand_Make);
	Commands_Register(CuboidCommand_Make);
	Commands_Register(TeleportCommand_Make);

	StringsBuffer_Init(&Chat_Log);
	StringsBuffer_Init(&Chat_InputLog);
}

void Chat_Reset(void) {
	Chat_CloseLog();
	String_Clear(&Chat_LogName);
}

void Chat_Free(void) {
	commands_count = 0;
	if (Chat_LogTimes != Chat_DefaultLogTimes) Platform_MemFree(&Chat_LogTimes);

	StringsBuffer_Free(&Chat_Log);
	StringsBuffer_Free(&Chat_InputLog);
}

IGameComponent Chat_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init  = Chat_Init;
	comp.Reset = Chat_Reset;
	comp.Free  = Chat_Free;
	return comp;
}