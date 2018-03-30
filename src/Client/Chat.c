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

void ChatLine_Make(ChatLine* line, STRING_TRANSIENT String* text) {
	String dst = String_InitAndClearArray(line->Buffer);
	String_AppendString(&dst, text);
	line->Received = Platform_CurrentUTCTime();
}

UInt8 Chat_LogNameBuffer[String_BufferSize(STRING_SIZE)];
String Chat_LogName = String_FromEmptyArray(Chat_LogNameBuffer);
Stream Chat_LogStream;
DateTime ChatLog_LastLogDate;

void Chat_CloseLog(void) {
	if (Chat_LogStream.Data == NULL) return;
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

	UInt32 i;
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
	UInt32 i;
	for (i = 0; i < 20; i++) {
		UInt8 pathBuffer[String_BufferSize(FILENAME_SIZE)];
		String path = String_InitAndClearArray(pathBuffer);
		String_AppendConst(&path, "logs");
		String_Append(&path, Platform_DirectorySeparator);

		String_AppendPaddedInt32(&path, 4, now->Year);
		String_Append(&path, '-');
		String_AppendPaddedInt32(&path, 2, now->Month);
		String_Append(&path, '-');
		String_AppendPaddedInt32(&path, 2, now->Day);

		String_AppendString(&path, &Chat_LogName);
		if (i > 0) {
			String_AppendConst(&path, " _");
			String_AppendInt32(&path, i);
		}
		String_AppendConst(&path, ".log");

		void* file;
		ReturnCode code = Platform_FileOpen(&file, &path, false);
		if (code != 0 && code != ReturnCode_FileShareViolation) {
			ErrorHandler_FailWithCode(code, "Chat - opening log file");
		}

		Stream_FromFile(&Chat_LogStream, file, &path);
		return;
	}

	Chat_LogStream.Data = NULL;
	String failedMsg = String_FromConst("Failed to open chat log file after 20 tries, giving up");
	ErrorHandler_Log(&failedMsg);
}

void Chat_AppendLog(STRING_PURE String* text) {
	if (Chat_LogName.length == 0 || !Game_ChatLogging) return;
	DateTime now = Platform_CurrentLocalTime();

	if (now.Day != ChatLog_LastLogDate.Day || now.Month != ChatLog_LastLogDate.Month || now.Year != ChatLog_LastLogDate.Year) {
		Chat_CloseLog();
		Chat_OpenLog(&now);
	}

	ChatLog_LastLogDate = now;
	if (Chat_LogStream.Data == NULL) return;
	UInt8 logBuffer[String_BufferSize(STRING_SIZE * 2)];
	String str = String_InitAndClearArray(logBuffer);

	/* [HH:mm:ss] text */
	String_Append(&str, '['); String_AppendPaddedInt32(&str, now.Hour,   2);
	String_Append(&str, ':'); String_AppendPaddedInt32(&str, now.Minute, 2);
	String_Append(&str, ':'); String_AppendPaddedInt32(&str, now.Second, 2);
	String_Append(&str, ']'); String_Append(&str, ' ');
	String_AppendColorless(&str, text);

	Stream_WriteLine(&Chat_LogStream, &str);
}

void Chat_Add(STRING_PURE String* text) { Chat_AddOf(text, MSG_TYPE_NORMAL); }
void Chat_AddOf(STRING_PURE String* text, Int32 msgType) {
	if (msgType == MSG_TYPE_NORMAL) {
		StringsBuffer_Add(&Chat_Log, text);
		Chat_AppendLog(text);
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


typedef struct ChatCommand_ {
	String Name;
	String Help[6];
	void (*Execute)(STRING_PURE String* args, UInt32 argsCount);
	bool SingleplayerOnly;
} ChatCommand;
typedef void (*ChatCommandConstructor)(ChatCommand* cmd);

#define COMMANDS_PREFIX "/client"
#define COMMANDS_PREFIX_SPACE "/client "
#define Command_SetName(raw) String name = String_FromConst(raw); cmd->Name = name;
#define Command_SetHelp(Num, raw) String help ## Num = String_FromConst(raw); cmd->Help[Num] = help ## Num;
ChatCommand commands_list[8];
UInt32 commands_count;

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

	ChatCommand command;
	Platform_MemSet(&command, 0, sizeof(ChatCommand));
	constructor(&command);
	commands_list[commands_count++] = command;
}

void Commands_Log(const UInt8* raw1, String* str2, const UInt8* raw3) {
	UInt8 strBuffer[String_BufferSize(STRING_SIZE * 2)];
	String str = String_InitAndClearArray(strBuffer);
	String_AppendConst(&str, raw1);
	String_AppendString(&str, str2);
	String_AppendConst(&str, raw3);
	Chat_Add(&str);
}

ChatCommand* Commands_GetMatch(STRING_PURE String* cmdName) {
	ChatCommand* match = NULL;
	UInt32 i;
	for (i = 0; i < commands_count; i++) {
		ChatCommand* cmd = &commands_list[i];
		if (!String_CaselessStarts(&cmd->Name, cmdName)) continue;

		if (match != NULL) {
			Commands_Log("&e/client: Multiple commands found that start with: \"&f", cmdName, "&e\".");
			return NULL;
		}
		match = cmd;
	}

	if (match == NULL) {
		Commands_Log("&e/client: Unrecognised command: \"&f", cmdName, "&e\".");
		Chat_AddRaw(tmp, "&e/client: Type &a/client &efor a list of commands.");
		return NULL;
	}
	if (match->SingleplayerOnly && !ServerConnection_IsSinglePlayer) {
		Commands_Log("&e/client: \"&f", cmdName, "&e\" can only be used in singleplayer.");
		return NULL;
	}
	return match;
}

void Commands_PrintDefined(void) {
	UInt8 strBuffer[String_BufferSize(STRING_SIZE)];
	String str = String_InitAndClearArray(strBuffer);
	UInt32 i;

	for (i = 0; i < commands_count; i++) {
		ChatCommand* cmd = &commands_list[i];
		String name = cmd->Name;

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


void HelpCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	if (argsCount == 1) {
		Chat_AddRaw(tmp1, "&eList of client commands:");
		Commands_PrintDefined();
		Chat_AddRaw(tmp2, "&eTo see help for a command, type /client help [cmd name]");
	} else {
		ChatCommand* cmd = Commands_GetMatch(&args[1]);
		if (cmd == NULL) return;

		UInt32 i;
		for (i = 0; i < Array_Elems(cmd->Help); i++) {
			String* help = &cmd->Help[i];
			if (help->length == 0) continue;
			Chat_Add(help);
		}
	}
}

void HelpCommand_Make(ChatCommand* cmd) {
	Command_SetName("Help");
	Command_SetHelp(0, "&a/client help [command name]");
	Command_SetHelp(1, "&eDisplays the help for the given command.");
	cmd->Execute = HelpCommand_Execute;
}

void GpuInfoCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	UInt32 i;
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
	Command_SetName("GpuInfo");
	Command_SetHelp(0, "&a/client gpuinfo");
	Command_SetHelp(1, "&eDisplays information about your GPU.");
	cmd->Execute = GpuInfoCommand_Execute;
}

void RenderTypeCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	if (argsCount == 1) {
		Chat_AddRaw(tmp, "&e/client: &cYou didn't specify a new render type.");
	} else if (Game_SetRenderType(&args[1])) {
		Commands_Log("&e/client: &fRender type is now ", &args[1], ".");
	} else {
		Commands_Log("&e/client: &cUnrecognised render type &f\"", &args[1], "\"&c.");
	}
}

void RenderTypeCommand_Make(ChatCommand* cmd) {
	Command_SetName("RenderType");
	Command_SetHelp(0, "&a/client rendertype [normal/legacy/legacyfast]");
	Command_SetHelp(1, "&bnormal: &eDefault renderer, with all environmental effects enabled.");
	Command_SetHelp(2, "&blegacy: &eMay be slightly slower than normal, but produces the same environmental effects.");
	Command_SetHelp(3, "&blegacyfast: &eSacrifices clouds, fog and overhead sky for faster performance.");
	Command_SetHelp(4, "&bnormalfast: &eSacrifices clouds, fog and overhead sky for faster performance.");
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
		Options_SetInt32(OPTION_WINDOW_WIDTH, width);
		Options_SetInt32(OPTION_WINDOW_HEIGHT, height);
	}
}

void ResolutionCommand_Make(ChatCommand* cmd) {
	Command_SetName("Resolution");
	Command_SetHelp(0, "&a/client resolution [width] [height]");
	Command_SetHelp(1, "&ePrecisely sets the size of the rendered window.");
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
	Command_SetName("Model");
	Command_SetHelp(0, "&a/client model [name]");
	Command_SetHelp(1, "&bnames: &echibi, chicken, creeper, human, pig, sheep");
	Command_SetHelp(2, "&e       skeleton, spider, zombie, sitting, <numerical block id>")
	cmd->SingleplayerOnly = true;
	cmd->Execute = ModelCommand_Execute;
}

Int32 cuboid_block = -1;
Vector3I cuboid_mark1, cuboid_mark2;
bool cuboid_persist = false;

bool CuboidCommand_ParseBlock(STRING_PURE String* args, UInt32 argsCount) {
	if (argsCount == 1) return true;
	String yes = String_FromConst("yes");
	if (String_CaselessEquals(&args[1], &yes)) { cuboid_persist = true; return true; }

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
			Commands_Log("&eCuboid: &c\"", &args[1], "\" is not a valid block name or id."); return false;
		}
	}

	if (block >= BLOCK_CPE_COUNT && !Block_IsCustomDefined(block)) {
		Commands_Log("&eCuboid: &cThere is no block with id \"", &args[1], "\"."); return false;
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

		String_AppendConst(&msg, "&eCuboid: &fMark 1 placed at (");
		String_AppendInt32(&msg, coords.X); String_AppendConst(&msg, ", ");
		String_AppendInt32(&msg, coords.Y); String_AppendConst(&msg, ", ");
		String_AppendInt32(&msg, coords.Z);
		String_AppendConst(&msg, "), place mark 2.");
		Chat_AddOf(&msg, MSG_TYPE_CLIENTSTATUS_3);
	} else {
		cuboid_mark2 = coords;
		CuboidCommand_DoCuboid();
		String empty = String_MakeNull(); Chat_AddOf(&empty, MSG_TYPE_CLIENTSTATUS_3);

		if (!cuboid_persist) {
			Event_UnregisterBlock(&UserEvents_BlockChanged, NULL, CuboidCommand_BlockChanged);
		} else {
			cuboid_mark1 = Vector3I_Create1(Int32_MaxValue);
			String msg = String_FromConst("&eCuboid: &fPlace or delete a block.");
			Chat_AddOf(&msg, MSG_TYPE_CLIENTSTATUS_3);
		}
	}
}

void CuboidCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	Event_UnregisterBlock(&UserEvents_BlockChanged, NULL, CuboidCommand_BlockChanged);
	cuboid_block = -1;
	cuboid_mark1 = Vector3I_Create1(Int32_MaxValue);
	cuboid_mark2 = Vector3I_Create1(Int32_MaxValue);
	cuboid_persist = false;

	if (!CuboidCommand_ParseBlock(args, argsCount)) return;
	String yes = String_FromConst("yes");
	if (argsCount > 2 && String_CaselessEquals(&args[2], &yes)) {
		cuboid_persist = true;
	}

	String msg = String_FromConst("&eCuboid: &fPlace or delete a block.");
	Chat_AddOf(&msg, MSG_TYPE_CLIENTSTATUS_3);
	Event_RegisterBlock(&UserEvents_BlockChanged, NULL, CuboidCommand_BlockChanged);
}

void CuboidCommand_Make(ChatCommand* cmd) {
	Command_SetName("Cuboid");
	Command_SetHelp(0, "&a/client cuboid [block] [persist]");
	Command_SetHelp(1, "&eFills the 3D rectangle between two points with [block].");
	Command_SetHelp(2, "&eIf no block is given, uses your currently held block.");
	Command_SetHelp(3, "&e  If persist is given and is \"yes\", then the command");
	Command_SetHelp(4, "&e  will repeatedly cuboid, without needing to be typed in again.");
	cmd->SingleplayerOnly = true;
	cmd->Execute = CuboidCommand_Execute;
}

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
	Command_SetName("TP");
	Command_SetHelp(0, "&a/client tp [x y z]");
	Command_SetHelp(1, "&eMoves you to the given coordinates.");
	cmd->SingleplayerOnly = true;
	cmd->Execute = TeleportCommand_Execute;
}


void Chat_Send(STRING_PURE String* text) {
	if (text->length == 0) return;
	StringsBuffer_Add(&Chat_InputLog, text);

	if (Commands_IsCommandPrefix(text)) {
		Commands_Execute(text);
	} else {
		ServerConnection_SendChat(text);
	}
}

void Commands_Init(void) {
	Commands_Register(GpuInfoCommand_Make);
	Commands_Register(HelpCommand_Make);
	Commands_Register(RenderTypeCommand_Make);
	Commands_Register(ResolutionCommand_Make);
	Commands_Register(ModelCommand_Make);
	Commands_Register(CuboidCommand_Make);
	Commands_Register(TeleportCommand_Make);
}

void Chat_Reset(void) {
	Chat_CloseLog();
	String_Clear(&Chat_LogName);
}

void Commands_Free(void) {
	commands_count = 0;
}

IGameComponent Chat_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = Commands_Init;
	comp.Reset = Chat_Reset;
	comp.Free = Commands_Free;
	return comp;
}