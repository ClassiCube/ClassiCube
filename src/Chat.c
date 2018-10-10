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
#include "GraphicsCommon.h"
#include "Funcs.h"
#include "Block.h"
#include "EnvRenderer.h"
#include "GameStructs.h"

#define CHAT_LOGTIMES_DEF_ELEMS 256
TimeMS Chat_DefaultLogTimes[CHAT_LOGTIMES_DEF_ELEMS];
TimeMS* Chat_LogTimes = Chat_DefaultLogTimes;
int Chat_LogTimesMax = CHAT_LOGTIMES_DEF_ELEMS, Chat_LogTimesCount;

TimeMS Chat_GetLogTime(int i) {
	if (i < 0 || i >= Chat_LogTimesCount) ErrorHandler_Fail("Tried to get time past LogTime end");
	return Chat_LogTimes[i];
}

static void Chat_AppendLogTime(void) {	
	if (Chat_LogTimesCount == Chat_LogTimesMax) {
		Chat_LogTimes = Utils_Resize(Chat_LogTimes, &Chat_LogTimesMax,
									sizeof(TimeMS), CHAT_LOGTIMES_DEF_ELEMS, 512);
	}

	TimeMS now = DateTime_CurrentUTC_MS();
	Chat_LogTimes[Chat_LogTimesCount++] = now;
}

static void ChatLine_Make(struct ChatLine* line, const String* text) {
	String dst = String_ClearedArray(line->Buffer);
	String_AppendString(&dst, text);
	line->Received = DateTime_CurrentUTC_MS();
}

char   Chat_LogNameBuffer[STRING_SIZE];
String Chat_LogName = String_FromArray(Chat_LogNameBuffer);
char   Chat_LogPathBuffer[FILENAME_SIZE];
String Chat_LogPath = String_FromArray(Chat_LogPathBuffer);

struct Stream Chat_LogStream;
DateTime ChatLog_LastLogDate;

static void Chat_CloseLog(void) {
	if (!Chat_LogStream.Meta.File) return;
	ReturnCode res = Chat_LogStream.Close(&Chat_LogStream);
	if (res) { Chat_LogError2(res, "closing", &Chat_LogPath); }
}

static bool Chat_AllowedLogChar(char c) {
	return
		c == '{' || c == '}' || c == '[' || c == ']' || c == '(' || c == ')' ||
		(c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void Chat_SetLogName(const String* name) {
	if (Chat_LogName.length) return;
	Chat_LogName.length = 0;

	char noColsBuffer[STRING_SIZE];
	String noColsName = String_FromArray(noColsBuffer);
	String_AppendColorless(&noColsName, name);

	int i;
	for (i = 0; i < noColsName.length; i++) {
		if (Chat_AllowedLogChar(noColsName.buffer[i])) {
			String_Append(&Chat_LogName, noColsName.buffer[i]);
		}
	}
}

static void Chat_DisableLogging(void) {
	Game_ChatLogging = false;
	Chat_AddRaw("&cDisabling chat logging");
}

static void Chat_OpenLog(DateTime* now) {
	ReturnCode res;
	if (!Utils_EnsureDirectory("logs")) { Chat_DisableLogging(); return; }

	/* Ensure multiple instances do not end up overwriting each other's log entries. */
	int i, year = now->Year, month = now->Month, day = now->Day;
	for (i = 0; i < 20; i++) {
		String* path = &Chat_LogPath;
		path->length = 0;
		String_Format4(path, "logs%r%p4-%p2-%p2 ", &Directory_Separator, &year, &month, &day);

		if (i > 0) {
			String_Format2(path, "%s _%i.log", &Chat_LogName, &i);
		} else {
			String_Format1(path, "%s.log", &Chat_LogName);
		}

		void* file; res = File_Append(&file, path);
		if (res && res != ReturnCode_FileShareViolation) {
			Chat_DisableLogging();
			Chat_LogError2(res, "appending to", path); return;
		}

		if (res == ReturnCode_FileShareViolation) continue;
		Stream_FromFile(&Chat_LogStream, file);
		return;
	}

	Chat_LogStream.Meta.File = NULL;
	Chat_DisableLogging();
	Chat_Add1("&cFailed to open a chat log file after %i tries, giving up", &i);	
}

static void Chat_AppendLog(const String* text) {
	if (!Chat_LogName.length || !Game_ChatLogging) return;
	DateTime now; DateTime_CurrentLocal(&now);

	if (now.Day != ChatLog_LastLogDate.Day || now.Month != ChatLog_LastLogDate.Month || now.Year != ChatLog_LastLogDate.Year) {
		Chat_CloseLog();
		Chat_OpenLog(&now);
	}

	ChatLog_LastLogDate = now;
	if (!Chat_LogStream.Meta.File) return;
	char logBuffer[STRING_SIZE * 2];
	String str = String_FromArray(logBuffer);

	/* [HH:mm:ss] text */
	int hour = now.Hour, minute = now.Minute, second = now.Second;
	String_Format3(&str, "[%p2:%p2:%p2] ", &hour, &minute, &second);
	String_AppendColorless(&str, text);

	ReturnCode res = Stream_WriteLine(&Chat_LogStream, &str);
	if (!res) return;
	Chat_DisableLogging();
	Chat_LogError2(res, "writing to", &Chat_LogPath);
}

void Chat_LogError(ReturnCode result, const char* place) {
	Chat_Add4("&cError %h when %c", &result, place, NULL, NULL);
}
void Chat_LogError2(ReturnCode result, const char* place, const String* path) {
	Chat_Add4("&cError %h when %c '%s'", &result, place, path, NULL);
}
void Chat_Add1(const char* format, const void* a1) {
	Chat_Add4(format, a1, NULL, NULL, NULL);
}
void Chat_Add2(const char* format, const void* a1, const void* a2) {
	Chat_Add4(format, a1, a2, NULL, NULL);
}
void Chat_Add3(const char* format, const void* a1, const void* a2, const void* a3) {
	Chat_Add4(format, a1, a2, a3, NULL);
}
void Chat_Add4(const char* format, const void* a1, const void* a2, const void* a3, const void* a4) {
	char msgBuffer[STRING_SIZE * 2];
	String msg = String_FromArray(msgBuffer);
	String_Format4(&msg, format, a1, a2, a3, a4);
	Chat_AddOf(&msg, MSG_TYPE_NORMAL);
}

void Chat_AddRaw(const char* raw) {
	String str = String_FromReadonly(raw);
	Chat_AddOf(&str, MSG_TYPE_NORMAL);
}
void Chat_Add(const String* text) { Chat_AddOf(text, MSG_TYPE_NORMAL); }

void Chat_AddOf(const String* text, int msgType) {
	Event_RaiseChat(&ChatEvents_ChatReceived, text, msgType);

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
}


/*########################################################################################################################*
*---------------------------------------------------------Commands--------------------------------------------------------*
*#########################################################################################################################*/
struct ChatCommand {
	const char* Name;
	void (*Execute)(const String* args, int argsCount);
	bool SingleplayerOnly;
	const char* Help[5];
};

#define COMMANDS_PREFIX "/client"
#define COMMANDS_PREFIX_SPACE "/client "
struct ChatCommand commands_list[8];
int commands_count;

static bool Commands_IsCommandPrefix(const String* input) {
	if (!input->length) return false;
	if (ServerConnection_IsSinglePlayer && input->buffer[0] == '/')
		return true;

	String prefixSpace = String_FromConst(COMMANDS_PREFIX_SPACE);
	String prefix      = String_FromConst(COMMANDS_PREFIX);
	return String_CaselessStarts(input, &prefixSpace)
		|| String_CaselessEquals(input, &prefix);
}

NOINLINE_ static void Commands_Register(struct ChatCommand* cmd) {
	if (commands_count == Array_Elems(commands_list)) {
		ErrorHandler_Fail("Commands_Register - hit max client commands");
	}
	commands_list[commands_count++] = *cmd;
}

static struct ChatCommand* Commands_GetMatch(const String* cmdName) {
	struct ChatCommand* match = NULL;
	int i;
	for (i = 0; i < commands_count; i++) {
		struct ChatCommand* cmd = &commands_list[i];
		String name = String_FromReadonly(cmd->Name);
		if (!String_CaselessStarts(&name, cmdName)) continue;

		if (match) {
			Chat_Add1("&e/client: Multiple commands found that start with: \"&f%s&e\".", cmdName);
			return NULL;
		}
		match = cmd;
	}

	if (!match) {
		Chat_Add1("&e/client: Unrecognised command: \"&f%s&e\".", cmdName);
		Chat_AddRaw("&e/client: Type &a/client &efor a list of commands.");
		return NULL;
	}
	if (match->SingleplayerOnly && !ServerConnection_IsSinglePlayer) {
		Chat_Add1("&e/client: \"&f%s&e\" can only be used in singleplayer.", cmdName);
		return NULL;
	}
	return match;
}

static void Commands_PrintDefault(void) {
	Chat_AddRaw("&eList of client commands:");
	char strBuffer[STRING_SIZE];
	String str = String_FromArray(strBuffer);
	int i;

	for (i = 0; i < commands_count; i++) {
		struct ChatCommand* cmd = &commands_list[i];
		String name = String_FromReadonly(cmd->Name);

		if ((str.length + name.length + 2) > str.capacity) {
			Chat_Add(&str);
			str.length = 0;
		}
		String_AppendString(&str, &name);
		String_AppendConst(&str, ", ");
	}

	if (str.length) { Chat_Add(&str); }
	Chat_AddRaw("&eTo see help for a command, type /client help [cmd name]");
}

static void Commands_Execute(const String* input) {
	String text        = *input;
	String prefixSpace = String_FromConst(COMMANDS_PREFIX_SPACE);
	String prefix      = String_FromConst(COMMANDS_PREFIX);

	int offset;
	if (String_CaselessStarts(&text, &prefixSpace)) { /* /clientcommand args */
		offset = prefixSpace.length;
	} else if (String_CaselessStarts(&text, &prefix)) { /* /client command args */
		offset = prefix.length;
	} else { /* /command args */
		offset = 1;
	}
	text = String_UNSAFE_Substring(&text, offset, text.length - offset);

	if (!text.length) { /* only / or /client */
		Commands_PrintDefault(); return;
	}

	String args[10];
	int argsCount = Array_Elems(args);
	String_UNSAFE_Split(&text, ' ', args, &argsCount);

	struct ChatCommand* cmd = Commands_GetMatch(&args[0]);
	if (!cmd) return;
	cmd->Execute(args, argsCount);
}


/*########################################################################################################################*
*------------------------------------------------------Simple commands----------------------------------------------------*
*#########################################################################################################################*/
static void HelpCommand_Execute(const String* args, int argsCount) {
	if (argsCount == 1) { 
		Commands_PrintDefault(); return; 
	}

	struct ChatCommand* cmd = Commands_GetMatch(&args[1]);
	if (!cmd) return;

	int i;
	for (i = 0; i < Array_Elems(cmd->Help); i++) {
		if (!cmd->Help[i]) continue;
		Chat_AddRaw(cmd->Help[i]);
	}
}

struct ChatCommand HelpCommand_Instance = {
	"Help", HelpCommand_Execute, false,
	{
		"&a/client help [command name]",
		"&eDisplays the help for the given command.",
	}
};

static void GpuInfoCommand_Execute(const String* args, int argsCount) {
	Gfx_UpdateApiInfo();
	int i;
	for (i = 0; i < Array_Elems(Gfx_ApiInfo); i++) {
		if (!Gfx_ApiInfo[i].length) continue;
		Chat_Add1("&a%s", &Gfx_ApiInfo[i]);
	}
}

struct ChatCommand GpuInfoCommand_Instance = {
	"GpuInfo", GpuInfoCommand_Execute, false,
	{
		"&a/client gpuinfo",
		"&eDisplays information about your GPU.",
	}
};

static void RenderTypeCommand_Execute(const String* args, int argsCount) {
	if (argsCount == 1) {
		Chat_AddRaw("&e/client: &cYou didn't specify a new render type."); return;
	}

	int flags = Game_CalcRenderType(&args[1]);
	if (flags >= 0) {
		EnvRenderer_UseLegacyMode( flags & 1);
		EnvRenderer_UseMinimalMode(flags & 2);

		Options_Set(OPT_RENDER_TYPE, &args[1]);
		Chat_Add1("&e/client: &fRender type is now %s.", &args[1]);
	} else {
		Chat_Add1("&e/client: &cUnrecognised render type &f\"%s\"&c.", &args[1]);
	}
}

struct ChatCommand RenderTypeCommand_Instance = {
	"RenderType", RenderTypeCommand_Execute, false,
	{
		"&a/client rendertype [normal/legacy/legacyfast]",
		"&bnormal: &eDefault renderer, with all environmental effects enabled.",
		"&blegacy: &eMay be slightly slower than normal, but produces the same environmental effects.",
		"&blegacyfast: &eSacrifices clouds, fog and overhead sky for faster performance.",
		"&bnormalfast: &eSacrifices clouds, fog and overhead sky for faster performance.",
	}
};

static void ResolutionCommand_Execute(const String* args, int argsCount) {
	int width, height;
	if (argsCount < 3) {
		Chat_AddRaw("&e/client: &cYou didn't specify width and height");
	} else if (!Convert_TryParseInt(&args[1], &width) || !Convert_TryParseInt(&args[2], &height)) {
		Chat_AddRaw("&e/client: &cWidth and height must be integers.");
	} else if (width <= 0 || height <= 0) {
		Chat_AddRaw("&e/client: &cWidth and height must be above 0.");
	} else {
		Window_SetClientSize(width, height);
		Options_SetInt(OPT_WINDOW_WIDTH, width);
		Options_SetInt(OPT_WINDOW_HEIGHT, height);
	}
}

struct ChatCommand ResolutionCommand_Instance = {
	"Resolution", ResolutionCommand_Execute, false,
	{
		"&a/client resolution [width] [height]",
		"&ePrecisely sets the size of the rendered window.",
	}
};

static void ModelCommand_Execute(const String* args, int argsCount) {
	if (argsCount == 1) {
		Chat_AddRaw("&e/client model: &cYou didn't specify a model name.");
	} else {
		Entity_SetModel(&LocalPlayer_Instance.Base, &args[1]);
	}
}

struct ChatCommand ModelCommand_Instance = {
	"Model", ModelCommand_Execute, true,
	{
		"&a/client model [name]",
		"&bnames: &echibi, chicken, creeper, human, pig, sheep",
		"&e       skeleton, spider, zombie, sitting, <numerical block id>",
	}
};


/*########################################################################################################################*
*-------------------------------------------------------CuboidCommand-----------------------------------------------------*
*#########################################################################################################################*/
int cuboid_block = -1;
Vector3I cuboid_mark1, cuboid_mark2;
bool cuboid_persist, cuboid_hooked;

static bool CuboidCommand_ParseBlock(const String* args, int argsCount) {
	if (argsCount == 1) return true;
	if (String_CaselessEqualsConst(&args[1], "yes")) { cuboid_persist = true; return true; }

	int raw = Block_Parse(&args[1]);
	if (raw == -1) {
		Chat_Add1("&eCuboid: &c\"%s\" is not a valid block name or id.", &args[1]); return false;
	}

	BlockID block = (BlockID)raw;
	if (block >= BLOCK_CPE_COUNT && !Block_IsCustomDefined(block)) {
		Chat_Add1("&eCuboid: &cThere is no block with id \"%s\".", &args[1]); return false;
	}

	cuboid_block = block;
	return true;
}

static void CuboidCommand_DoCuboid(void) {
	Vector3I min, max;
	Vector3I_Min(&min, &cuboid_mark1, &cuboid_mark2);
	Vector3I_Max(&max, &cuboid_mark1, &cuboid_mark2);
	if (!World_IsValidPos_3I(min) || !World_IsValidPos_3I(max)) return;

	BlockID toPlace = (BlockID)cuboid_block;
	if (cuboid_block == -1) toPlace = Inventory_SelectedBlock;

	int x, y, z;
	for (y = min.Y; y <= max.Y; y++) {
		for (z = min.Z; z <= max.Z; z++) {
			for (x = min.X; x <= max.X; x++) {
				Game_UpdateBlock(x, y, z, toPlace);
			}
		}
	}
}

static void CuboidCommand_BlockChanged(void* obj, Vector3I coords, BlockID old, BlockID now) {
	if (cuboid_mark1.X == Int32_MaxValue) {
		cuboid_mark1 = coords;
		Game_UpdateBlock(coords.X, coords.Y, coords.Z, old);
		char msgBuffer[STRING_SIZE];
		String msg = String_FromArray(msgBuffer);

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

static void CuboidCommand_Execute(const String* args, int argsCount) {
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

struct ChatCommand CuboidCommand_Instance = {
	"Cuboid", CuboidCommand_Execute, true, 
	{
		"&a/client cuboid [block] [persist]",
		"&eFills the 3D rectangle between two points with [block].",
		"&eIf no block is given, uses your currently held block.",
		"&e  If persist is given and is \"yes\", then the command",
		"&e  will repeatedly cuboid, without needing to be typed in again.",
	}
};


/*########################################################################################################################*
*------------------------------------------------------TeleportCommand----------------------------------------------------*
*#########################################################################################################################*/
static void TeleportCommand_Execute(const String* args, int argsCount) {
	if (argsCount != 4) {
		Chat_AddRaw("&e/client teleport: &cYou didn't specify X, Y and Z coordinates.");
	} else {
		float x, y, z;
		if (!Convert_TryParseFloat(&args[1], &x) || !Convert_TryParseFloat(&args[2], &y) || !Convert_TryParseFloat(&args[3], &z)) {
			Chat_AddRaw("&e/client teleport: &cCoordinates must be decimals");
			return;
		}

		Vector3 v = { x, y, z };
		struct LocationUpdate update; LocationUpdate_MakePos(&update, v, false);
		struct Entity* entity = &LocalPlayer_Instance.Base;
		entity->VTABLE->SetLocation(entity, &update, false);
	}
}

struct ChatCommand TeleportCommand_Instance = {
	"TP", TeleportCommand_Execute, true,
	{
		"&a/client tp [x y z]",
		"&eMoves you to the given coordinates.",
	}
};


/*########################################################################################################################*
*-------------------------------------------------------Generic chat------------------------------------------------------*
*#########################################################################################################################*/
void Chat_Send(const String* text, bool logUsage) {
	if (!text->length) return;
	Event_RaiseChat(&ChatEvents_ChatSending, text, 0);
	if (logUsage) StringsBuffer_Add(&Chat_InputLog, text);

	if (Commands_IsCommandPrefix(text)) {
		Commands_Execute(text);
	} else {
		ServerConnection_SendChat(text);
	}
}

static void Chat_Init(void) {
	Commands_Register(&GpuInfoCommand_Instance);
	Commands_Register(&HelpCommand_Instance);
	Commands_Register(&RenderTypeCommand_Instance);
	Commands_Register(&ResolutionCommand_Instance);
	Commands_Register(&ModelCommand_Instance);
	Commands_Register(&CuboidCommand_Instance);
	Commands_Register(&TeleportCommand_Instance);
}

static void Chat_Reset(void) {
	Chat_CloseLog();
	Chat_LogName.length = 0;
	String empty = String_MakeNull();
	/* reset CPE messages */
	Chat_AddOf(&empty, MSG_TYPE_ANNOUNCEMENT);
	Chat_AddOf(&empty, MSG_TYPE_STATUS_1);
	Chat_AddOf(&empty, MSG_TYPE_STATUS_2);
	Chat_AddOf(&empty, MSG_TYPE_STATUS_3);
	Chat_AddOf(&empty, MSG_TYPE_BOTTOMRIGHT_1);
	Chat_AddOf(&empty, MSG_TYPE_BOTTOMRIGHT_2);
	Chat_AddOf(&empty, MSG_TYPE_BOTTOMRIGHT_3);
}

static void Chat_Free(void) {
	Chat_CloseLog();
	commands_count = 0;

	if (Chat_LogTimes != Chat_DefaultLogTimes) Mem_Free(Chat_LogTimes);
	Chat_LogTimes      = Chat_DefaultLogTimes;
	Chat_LogTimesCount = 0;

	StringsBuffer_Clear(&Chat_Log);
	StringsBuffer_Clear(&Chat_InputLog);
}

void Chat_MakeComponent(struct IGameComponent* comp) {
	comp->Init  = Chat_Init;
	comp->Reset = Chat_Reset;
	comp->Free  = Chat_Free;
}
