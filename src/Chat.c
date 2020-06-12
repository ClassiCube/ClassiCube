#include "Chat.h"
#include "Stream.h"
#include "Platform.h"
#include "Event.h"
#include "Game.h"
#include "Logger.h"
#include "Server.h"
#include "World.h"
#include "Inventory.h"
#include "Entity.h"
#include "Window.h"
#include "Graphics.h"
#include "Funcs.h"
#include "Block.h"
#include "EnvRenderer.h"
#include "GameStructs.h"
#include "Utils.h"
#include "TexturePack.h"
#include "Options.h"

static char msgs[10][STRING_SIZE];
String Chat_Status[4]       = { String_FromArray(msgs[0]), String_FromArray(msgs[1]), String_FromArray(msgs[2]), String_FromArray(msgs[3]) };
String Chat_BottomRight[3]  = { String_FromArray(msgs[4]), String_FromArray(msgs[5]), String_FromArray(msgs[6]) };
String Chat_ClientStatus[2] = { String_FromArray(msgs[7]), String_FromArray(msgs[8]) };

String Chat_Announcement = String_FromArray(msgs[9]);
double Chat_AnnouncementReceived;
struct StringsBuffer Chat_Log, Chat_InputLog;
cc_bool Chat_Logging;

/*########################################################################################################################*
*-------------------------------------------------------Chat logging------------------------------------------------------*
*#########################################################################################################################*/
#define CHAT_LOGTIMES_DEF_ELEMS 256
static double defaultLogTimes[CHAT_LOGTIMES_DEF_ELEMS];
static int logTimesCapacity = CHAT_LOGTIMES_DEF_ELEMS, logTimesCount;
double* Chat_LogTime = defaultLogTimes;

static void AppendChatLogTime(void) {
	double now = Game.Time;

	if (logTimesCount == logTimesCapacity) {
		Utils_Resize((void**)&Chat_LogTime, &logTimesCapacity,
					sizeof(double), CHAT_LOGTIMES_DEF_ELEMS, 512);
	}
	Chat_LogTime[logTimesCount++] = now;
}

#ifdef CC_BUILD_MINFILES
static void ResetLogFile(void) { }
static void CloseLogFile(void) { }
void Chat_SetLogName(const String* name) { }
void Chat_DisableLogging(void) { }
static void OpenChatLog(struct DateTime* now) { }
static void AppendChatLog(const String* text) { }
#else
static char   logNameBuffer[STRING_SIZE];
static String logName = String_FromArray(logNameBuffer);
static char   logPathBuffer[FILENAME_SIZE];
static String logPath = String_FromArray(logPathBuffer);

static struct Stream logStream;
static int lastLogDay, lastLogMonth, lastLogYear;

/* Resets log name to empty and resets last log date */
static void ResetLogFile(void) {
	logName.length = 0;
	lastLogYear    = -123;
}

/* Closes handle to the chat log file */
static void CloseLogFile(void) {
	cc_result res;
	if (!logStream.Meta.File) return;

	res = logStream.Close(&logStream);
	if (res) { Logger_Warn2(res, "closing", &logPath); }
}

/* Whether the given character is an allowed in a log filename */
static cc_bool AllowedLogNameChar(char c) {
	return
		c == '{' || c == '}' || c == '[' || c == ']' || c == '(' || c == ')' ||
		(c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void Chat_SetLogName(const String* name) {
	char c;
	int i;
	if (logName.length) return;

	for (i = 0; i < name->length; i++) {
		c = name->buffer[i];

		if (AllowedLogNameChar(c)) {
			String_Append(&logName, c);
		} else if (c == '&') {
			i++; /* skip over following colour code */
		}
	}
}

void Chat_DisableLogging(void) {
	Chat_Logging = false;
	lastLogYear  = -321;
	Chat_AddRaw("&cDisabling chat logging");
	CloseLogFile();
}

static void OpenChatLog(struct DateTime* now) {
	cc_result res;
	int i;
	if (!Utils_EnsureDirectory("logs")) { Chat_DisableLogging(); return; }

	/* Ensure multiple instances do not end up overwriting each other's log entries. */
	for (i = 0; i < 20; i++) {
		logPath.length = 0;
		String_Format3(&logPath, "logs/%p4-%p2-%p2 ", &now->year, &now->month, &now->day);

		if (i > 0) {
			String_Format2(&logPath, "%s _%i.log", &logName, &i);
		} else {
			String_Format1(&logPath, "%s.log", &logName);
		}

		res = Stream_AppendFile(&logStream, &logPath);
		if (res && res != ReturnCode_FileShareViolation) {
			Chat_DisableLogging();
			Logger_Warn2(res, "appending to", &logPath);
			return;
		}

		if (res == ReturnCode_FileShareViolation) continue;
		return;
	}

	Chat_DisableLogging();
	Chat_Add1("&cFailed to open a chat log file after %i tries, giving up", &i);	
}

static void AppendChatLog(const String* text) {
	String str; char strBuffer[STRING_SIZE * 2];
	struct DateTime now;
	cc_result res;	

	if (!logName.length || !Chat_Logging) return;
	DateTime_CurrentLocal(&now);

	if (now.day != lastLogDay || now.month != lastLogMonth || now.year != lastLogYear) {
		CloseLogFile();
		OpenChatLog(&now);
	}

	lastLogDay = now.day; lastLogMonth = now.month; lastLogYear = now.year;
	if (!logStream.Meta.File) return;

	/* [HH:mm:ss] text */
	String_InitArray(str, strBuffer);
	String_Format3(&str, "[%p2:%p2:%p2] ", &now.hour, &now.minute, &now.second);
	String_AppendColorless(&str, text);

	res = Stream_WriteLine(&logStream, &str);
	if (!res) return;
	Chat_DisableLogging();
	Logger_Warn2(res, "writing to", &logPath);
}
#endif

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
	String msg; char msgBuffer[STRING_SIZE * 2];
	String_InitArray(msg, msgBuffer);

	String_Format4(&msg, format, a1, a2, a3, a4);
	Chat_AddOf(&msg, MSG_TYPE_NORMAL);
}

void Chat_AddRaw(const char* raw) {
	String str = String_FromReadonly(raw);
	Chat_AddOf(&str, MSG_TYPE_NORMAL);
}
void Chat_Add(const String* text) { Chat_AddOf(text, MSG_TYPE_NORMAL); }

void Chat_AddOf(const String* text, int msgType) {
	if (msgType == MSG_TYPE_NORMAL) {
		StringsBuffer_Add(&Chat_Log, text);
		AppendChatLogTime();
		AppendChatLog(text);
	} else if (msgType >= MSG_TYPE_STATUS_1 && msgType <= MSG_TYPE_STATUS_3) {
		/* Status[0] is for texture pack downloading message */
		String_Copy(&Chat_Status[1 + (msgType - MSG_TYPE_STATUS_1)], text);
	} else if (msgType >= MSG_TYPE_BOTTOMRIGHT_1 && msgType <= MSG_TYPE_BOTTOMRIGHT_3) {	
		String_Copy(&Chat_BottomRight[msgType - MSG_TYPE_BOTTOMRIGHT_1], text);
	} else if (msgType == MSG_TYPE_ANNOUNCEMENT) {
		String_Copy(&Chat_Announcement, text);
		Chat_AnnouncementReceived = Game.Time;
	} else if (msgType >= MSG_TYPE_CLIENTSTATUS_1 && msgType <= MSG_TYPE_CLIENTSTATUS_2) {
		String_Copy(&Chat_ClientStatus[msgType - MSG_TYPE_CLIENTSTATUS_1], text);
	}

	Event_RaiseChat(&ChatEvents.ChatReceived, text, msgType);
}


/*########################################################################################################################*
*---------------------------------------------------------Commands--------------------------------------------------------*
*#########################################################################################################################*/
#define COMMANDS_PREFIX "/client"
#define COMMANDS_PREFIX_SPACE "/client "
static struct ChatCommand* cmds_head;
static struct ChatCommand* cmds_tail;

static cc_bool Commands_IsCommandPrefix(const String* str) {
	static const String prefixSpace = String_FromConst(COMMANDS_PREFIX_SPACE);
	static const String prefix      = String_FromConst(COMMANDS_PREFIX);

	if (!str->length) return false;
	if (Server.IsSinglePlayer && str->buffer[0] == '/') return true;
	
	return String_CaselessStarts(str, &prefixSpace)
		|| String_CaselessEquals(str, &prefix);
}

void Commands_Register(struct ChatCommand* cmd) {
	LinkedList_Add(cmd, cmds_head, cmds_tail);
}

static struct ChatCommand* Commands_FindMatch(const String* cmdName) {
	struct ChatCommand* match = NULL;
	struct ChatCommand* cmd;
	String name;

	for (cmd = cmds_head; cmd; cmd = cmd->next) {
		name = String_FromReadonly(cmd->Name);
		if (String_CaselessEquals(&name, cmdName)) return cmd;
	}

	for (cmd = cmds_head; cmd; cmd = cmd->next) {
		name = String_FromReadonly(cmd->Name);
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
	}
	return match;
}

static void Commands_PrintDefault(void) {
	String str; char strBuffer[STRING_SIZE];
	String name;
	struct ChatCommand* cmd;

	Chat_AddRaw("&eList of client commands:");
	String_InitArray(str, strBuffer);

	for (cmd = cmds_head; cmd; cmd = cmd->next) {
		name = String_FromReadonly(cmd->Name);

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
	static const String prefixSpace = String_FromConst(COMMANDS_PREFIX_SPACE);
	static const String prefix      = String_FromConst(COMMANDS_PREFIX);
	String text = *input;

	int offset, count;
	String args[10];
	struct ChatCommand* cmd;

	if (String_CaselessStarts(&text, &prefixSpace)) { /* /client command args */
		offset = prefixSpace.length;
	} else if (String_CaselessStarts(&text, &prefix)) { /* /clientcommand args */
		offset = prefix.length;
	} else { /* /command args */
		offset = 1;
	}

	text = String_UNSAFE_SubstringAt(&text, offset);
	/* Check for only / or /client */
	if (!text.length) { Commands_PrintDefault(); return; }

	count = String_UNSAFE_Split(&text, ' ', args, Array_Elems(args));
	cmd   = Commands_FindMatch(&args[0]);
	if (!cmd) return;

	if (cmd->SingleplayerOnly && !Server.IsSinglePlayer) {
		Chat_Add1("&e/client: \"&f%s&e\" can only be used in singleplayer.", &args[0]);
		return;
	}
	cmd->Execute(&args[1], count - 1);
}


/*########################################################################################################################*
*------------------------------------------------------Simple commands----------------------------------------------------*
*#########################################################################################################################*/
static void HelpCommand_Execute(const String* args, int argsCount) {
	struct ChatCommand* cmd;
	int i;

	if (!argsCount) { Commands_PrintDefault(); return; }
	cmd = Commands_FindMatch(&args[0]);
	if (!cmd) return;

	for (i = 0; i < Array_Elems(cmd->Help); i++) {
		if (!cmd->Help[i]) continue;
		Chat_AddRaw(cmd->Help[i]);
	}
}

static struct ChatCommand HelpCommand = {
	"Help", HelpCommand_Execute, false,
	{
		"&a/client help [command name]",
		"&eDisplays the help for the given command.",
	}
};

static void GpuInfoCommand_Execute(const String* args, int argsCount) {
	char linesBuffer[GFX_APIINFO_LINES][STRING_SIZE];
	String lines[GFX_APIINFO_LINES]; 
	int i;

	for (i = 0; i < GFX_APIINFO_LINES; i++) {
		String_InitArray(lines[i], linesBuffer[i]);
	}
	Gfx_GetApiInfo(lines);
	
	for (i = 0; i < GFX_APIINFO_LINES; i++) {
		if (!lines[i].length) continue;
		Chat_Add1("&a%s", &lines[i]);
	}
}

static struct ChatCommand GpuInfoCommand = {
	"GpuInfo", GpuInfoCommand_Execute, false,
	{
		"&a/client gpuinfo",
		"&eDisplays information about your GPU.",
	}
};

static void RenderTypeCommand_Execute(const String* args, int argsCount) {
	int flags;
	if (!argsCount) {
		Chat_AddRaw("&e/client: &cYou didn't specify a new render type."); return;
	}

	flags = EnvRenderer_CalcFlags(&args[0]);
	if (flags >= 0) {
		EnvRenderer_SetMode(flags);
		Options_Set(OPT_RENDER_TYPE, &args[0]);
		Chat_Add1("&e/client: &fRender type is now %s.", &args[0]);
	} else {
		Chat_Add1("&e/client: &cUnrecognised render type &f\"%s\"&c.", &args[0]);
	}
}

static struct ChatCommand RenderTypeCommand = {
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
	if (argsCount < 2) {
		Chat_Add4("&e/client: &fCurrent resolution is %i@%f2 x %i@%f2", 
				&WindowInfo.Width, &DisplayInfo.DpiX, &WindowInfo.Height, &DisplayInfo.DpiY);
	} else if (!Convert_ParseInt(&args[0], &width) || !Convert_ParseInt(&args[1], &height)) {
		Chat_AddRaw("&e/client: &cWidth and height must be integers.");
	} else if (width <= 0 || height <= 0) {
		Chat_AddRaw("&e/client: &cWidth and height must be above 0.");
	} else {
		Window_SetSize(width, height);
		/* Window_Create uses these, but scales by DPI. Hence DPI unscale them here. */
		Options_SetInt(OPT_WINDOW_WIDTH,  (int)(width  / DisplayInfo.DpiX));
		Options_SetInt(OPT_WINDOW_HEIGHT, (int)(height / DisplayInfo.DpiY));
	}
}

static struct ChatCommand ResolutionCommand = {
	"Resolution", ResolutionCommand_Execute, false,
	{
		"&a/client resolution [width] [height]",
		"&ePrecisely sets the size of the rendered window.",
	}
};

static void ModelCommand_Execute(const String* args, int argsCount) {
	if (argsCount) {
		Entity_SetModel(&LocalPlayer_Instance.Base, &args[0]);
	} else {
		Chat_AddRaw("&e/client model: &cYou didn't specify a model name.");
	}
}

static struct ChatCommand ModelCommand = {
	"Model", ModelCommand_Execute, true,
	{
		"&a/client model [name]",
		"&bnames: &echibi, chicken, creeper, human, pig, sheep",
		"&e       skeleton, spider, zombie, sit, <numerical block id>",
	}
};

static void ClearDeniedCommand_Execute(const String* args, int argsCount) {
	int count = TextureCache_ClearDenied();
	Chat_Add1("Removed &e%i &fdenied texture pack URLs.", &count);
}

static struct ChatCommand ClearDeniedCommand = {
	"ClearDenied", ClearDeniedCommand_Execute, false,
	{
		"&a/client cleardenied",
		"&eClears the list of texture pack URLs you have denied",
	}
};


/*########################################################################################################################*
*-------------------------------------------------------CuboidCommand-----------------------------------------------------*
*#########################################################################################################################*/
static int cuboid_block = -1;
static IVec3 cuboid_mark1, cuboid_mark2;
static cc_bool cuboid_persist, cuboid_hooked, cuboid_hasMark1;
static const String cuboid_msg = String_FromConst("&eCuboid: &fPlace or delete a block.");

static cc_bool CuboidCommand_ParseBlock(const String* args, int argsCount) {
	int block;
	if (!argsCount) return true;
	if (String_CaselessEqualsConst(&args[0], "yes")) { cuboid_persist = true; return true; }

	block = Block_Parse(&args[0]);
	if (block == -1) {
		Chat_Add1("&eCuboid: &c\"%s\" is not a valid block name or id.", &args[0]); return false;
	}

	if (block >= BLOCK_CPE_COUNT && !Block_IsCustomDefined(block)) {
		Chat_Add1("&eCuboid: &cThere is no block with id \"%s\".", &args[0]); return false;
	}

	cuboid_block = block;
	return true;
}

static void CuboidCommand_DoCuboid(void) {
	IVec3 min, max;
	BlockID toPlace;
	int x, y, z;

	IVec3_Min(&min, &cuboid_mark1, &cuboid_mark2);
	IVec3_Max(&max, &cuboid_mark1, &cuboid_mark2);
	if (!World_Contains(min.X, min.Y, min.Z)) return;
	if (!World_Contains(max.X, max.Y, max.Z)) return;

	toPlace = (BlockID)cuboid_block;
	if (cuboid_block == -1) toPlace = Inventory_SelectedBlock;

	for (y = min.Y; y <= max.Y; y++) {
		for (z = min.Z; z <= max.Z; z++) {
			for (x = min.X; x <= max.X; x++) {
				Game_ChangeBlock(x, y, z, toPlace);
			}
		}
	}
}

static void CuboidCommand_BlockChanged(void* obj, IVec3 coords, BlockID old, BlockID now) {
	String msg; char msgBuffer[STRING_SIZE];
	String_InitArray(msg, msgBuffer);

	if (!cuboid_hasMark1) {
		cuboid_mark1    = coords;
		cuboid_hasMark1 = true;
		Game_UpdateBlock(coords.X, coords.Y, coords.Z, old);	

		String_Format3(&msg, "&eCuboid: &fMark 1 placed at (%i, %i, %i), place mark 2.", &coords.X, &coords.Y, &coords.Z);
		Chat_AddOf(&msg, MSG_TYPE_CLIENTSTATUS_1);
	} else {
		cuboid_mark2 = coords;
		CuboidCommand_DoCuboid();

		if (!cuboid_persist) {
			Event_UnregisterBlock(&UserEvents.BlockChanged, NULL, CuboidCommand_BlockChanged);
			cuboid_hooked = false;
			Chat_AddOf(&String_Empty, MSG_TYPE_CLIENTSTATUS_1);
		} else {
			cuboid_hasMark1 = false;
			Chat_AddOf(&cuboid_msg, MSG_TYPE_CLIENTSTATUS_1);
		}
	}
}

static void CuboidCommand_Execute(const String* args, int argsCount) {
	if (cuboid_hooked) {
		Event_UnregisterBlock(&UserEvents.BlockChanged, NULL, CuboidCommand_BlockChanged);
		cuboid_hooked = false;
	}

	cuboid_block    = -1;
	cuboid_hasMark1 = false;
	cuboid_persist  = false;

	if (!CuboidCommand_ParseBlock(args, argsCount)) return;
	if (argsCount > 1 && String_CaselessEqualsConst(&args[0], "yes")) {
		cuboid_persist = true;
	}

	Chat_AddOf(&cuboid_msg, MSG_TYPE_CLIENTSTATUS_1);
	Event_RegisterBlock(&UserEvents.BlockChanged, NULL, CuboidCommand_BlockChanged);
	cuboid_hooked = true;
}

static struct ChatCommand CuboidCommand = {
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
	struct LocationUpdate update;
	struct Entity* e = &LocalPlayer_Instance.Base;
	Vec3 v;

	if (argsCount != 3) {
		Chat_AddRaw("&e/client teleport: &cYou didn't specify X, Y and Z coordinates.");
		return;
	}
	if (!Convert_ParseFloat(&args[0], &v.X) || !Convert_ParseFloat(&args[1], &v.Y) || !Convert_ParseFloat(&args[2], &v.Z)) {
		Chat_AddRaw("&e/client teleport: &cCoordinates must be decimals");
		return;
	}

	LocationUpdate_MakePos(&update, v, false);
	e->VTABLE->SetLocation(e, &update, false);
}

static struct ChatCommand TeleportCommand = {
	"TP", TeleportCommand_Execute, true,
	{
		"&a/client tp [x y z]",
		"&eMoves you to the given coordinates.",
	}
};


/*########################################################################################################################*
*-------------------------------------------------------Generic chat------------------------------------------------------*
*#########################################################################################################################*/
void Chat_Send(const String* text, cc_bool logUsage) {
	if (!text->length) return;
	Event_RaiseChat(&ChatEvents.ChatSending, text, 0);
	if (logUsage) StringsBuffer_Add(&Chat_InputLog, text);

	if (Commands_IsCommandPrefix(text)) {
		Commands_Execute(text);
	} else {
		Server.SendChat(text);
	}
}

static void Chat_Init(void) {
	Commands_Register(&GpuInfoCommand);
	Commands_Register(&HelpCommand);
	Commands_Register(&RenderTypeCommand);
	Commands_Register(&ResolutionCommand);
	Commands_Register(&ModelCommand);
	Commands_Register(&CuboidCommand);
	Commands_Register(&TeleportCommand);
	Commands_Register(&ClearDeniedCommand);

#ifndef CC_BUILD_MINFILES
	Chat_Logging = Options_GetBool(OPT_CHAT_LOGGING, true);
#endif
}

static void Chat_Reset(void) {
	CloseLogFile();
	ResetLogFile();

	/* reset CPE messages */
	Chat_AddOf(&String_Empty, MSG_TYPE_ANNOUNCEMENT);
	Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_1);
	Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_2);
	Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_3);
	Chat_AddOf(&String_Empty, MSG_TYPE_BOTTOMRIGHT_1);
	Chat_AddOf(&String_Empty, MSG_TYPE_BOTTOMRIGHT_2);
	Chat_AddOf(&String_Empty, MSG_TYPE_BOTTOMRIGHT_3);
}

static void Chat_Free(void) {
	CloseLogFile();
	cmds_head = NULL;

	if (Chat_LogTime != defaultLogTimes) Mem_Free(Chat_LogTime);
	Chat_LogTime  = defaultLogTimes;
	logTimesCount = 0;

	StringsBuffer_Clear(&Chat_Log);
	StringsBuffer_Clear(&Chat_InputLog);
}

struct IGameComponent Chat_Component = {
	Chat_Init, /* Init  */
	Chat_Free, /* Free  */
	Chat_Reset /* Reset */
};
