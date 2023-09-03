#include "Chat.h"
#include "String.h"
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
#include "Utils.h"
#include "TexturePack.h"
#include "Options.h"
#include "Drawer2D.h"
 
static char status[5][STRING_SIZE];
static char bottom[3][STRING_SIZE];
static char client[2][STRING_SIZE];
static char announcement[STRING_SIZE];
static char bigAnnouncement[STRING_SIZE];
static char smallAnnouncement[STRING_SIZE];

cc_string Chat_Status[5]       = { String_FromArray(status[0]), String_FromArray(status[1]), String_FromArray(status[2]),
                                                                String_FromArray(status[3]), String_FromArray(status[4]) };
cc_string Chat_BottomRight[3]  = { String_FromArray(bottom[0]), String_FromArray(bottom[1]), String_FromArray(bottom[2]) };
cc_string Chat_ClientStatus[2] = { String_FromArray(client[0]), String_FromArray(client[8]) };

cc_string Chat_Announcement = String_FromArray(announcement);
cc_string Chat_BigAnnouncement = String_FromArray(bigAnnouncement);
cc_string Chat_SmallAnnouncement = String_FromArray(smallAnnouncement);

double Chat_AnnouncementReceived;
double Chat_BigAnnouncementReceived;
double Chat_SmallAnnouncementReceived;

struct StringsBuffer Chat_Log, Chat_InputLog;
cc_bool Chat_Logging;

/*########################################################################################################################*
*-------------------------------------------------------Chat logging------------------------------------------------------*
*#########################################################################################################################*/
double Chat_RecentLogTimes[CHATLOG_TIME_MASK + 1];

static void ClearChatLogs(void) {
	Mem_Set(Chat_RecentLogTimes, 0, sizeof(Chat_RecentLogTimes));
	StringsBuffer_Clear(&Chat_Log);
}

static char      logNameBuffer[STRING_SIZE];
static cc_string logName = String_FromArray(logNameBuffer);
static char      logPathBuffer[FILENAME_SIZE];
static cc_string logPath = String_FromArray(logPathBuffer);

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
	if (res) { Logger_SysWarn2(res, "closing", &logPath); }
}

/* Whether the given character is an allowed in a log filename */
static cc_bool AllowedLogNameChar(char c) {
	return
		c == '{' || c == '}' || c == '[' || c == ']' || c == '(' || c == ')' ||
		(c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void Chat_SetLogName(const cc_string* name) {
	char c;
	int i;
	if (logName.length) return;

	for (i = 0; i < name->length; i++) {
		c = name->buffer[i];

		if (AllowedLogNameChar(c)) {
			String_Append(&logName, c);
		} else if (c == '&') {
			i++; /* skip over following color code */
		}
	}
}

void Chat_DisableLogging(void) {
	Chat_Logging = false;
	lastLogYear  = -321;
	Chat_AddRaw("&cDisabling chat logging");
	CloseLogFile();
}

static cc_bool CreateLogsDirectory(void) {
	static const cc_string dir = String_FromConst("logs");
	cc_result res;
	/* Utils_EnsureDirectory cannot be used here because it causes a stack overflow  */
	/* when running the game and an error occurs when trying to create the directory */
	/* This happens because when running the game, Logger_WarnFunc is changed to log */
	/* a message in chat instead of showing a dialog box, which causes the following */
	/* functions to be called in a recursive loop: */
	/*                                                                                         */
	/* Utils_EnsureDirectory --> Logger_SysWarn2 --> Chat_Add --> AppendChatLog -> OpenChatLog */
	/*  --> Utils_EnsureDirectory --> Logger_SysWarn2 --> Chat_Add --> AppendChatLog -> OpenChatLog */
	/*       --> Utils_EnsureDirectory --> Logger_SysWarn2 --> Chat_Add --> AppendChatLog -> OpenChatLog */
	/*            --> Utils_EnsureDirectory --> Logger_SysWarn2 --> Chat_Add --> AppendChatLog ... */
	/* and so on, until eventually the stack overflows */
	res = Directory_Create(&dir);
	if (!res || res == ReturnCode_DirectoryExists) return true;

	Chat_DisableLogging();
	Logger_SysWarn2(res, "creating directory", &dir); 
	return false;
}

static void OpenChatLog(struct DateTime* now) {
	cc_result res;
	int i;
	if (!CreateLogsDirectory()) return;

	/* Ensure multiple instances do not end up overwriting each other's log entries. */
	for (i = 0; i < 20; i++) {
		logPath.length = 0;
		String_Format3(&logPath, "logs/%p4-%p2-%p2 ", &now->year, &now->month, &now->day);

		if (i > 0) {
			String_Format2(&logPath, "%s _%i.txt", &logName, &i);
		} else {
			String_Format1(&logPath, "%s.txt", &logName);
		}

		res = Stream_AppendFile(&logStream, &logPath);
		if (res && res != ReturnCode_FileShareViolation) {
			Chat_DisableLogging();
			Logger_SysWarn2(res, "appending to", &logPath);
			return;
		}

		if (res == ReturnCode_FileShareViolation) continue;
		return;
	}

	Chat_DisableLogging();
	Chat_Add1("&cFailed to open a chat log file after %i tries, giving up", &i);	
}

static void AppendChatLog(const cc_string* text) {
	cc_string str; char strBuffer[DRAWER2D_MAX_TEXT_LENGTH];
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
	Drawer2D_WithoutColors(&str, text);

	res = Stream_WriteLine(&logStream, &str);
	if (!res) return;
	Chat_DisableLogging();
	Logger_SysWarn2(res, "writing to", &logPath);
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
	cc_string msg; char msgBuffer[STRING_SIZE * 2];
	String_InitArray(msg, msgBuffer);

	String_Format4(&msg, format, a1, a2, a3, a4);
	Chat_AddOf(&msg, MSG_TYPE_NORMAL);
}

void Chat_AddRaw(const char* raw) {
	cc_string str = String_FromReadonly(raw);
	Chat_AddOf(&str, MSG_TYPE_NORMAL);
}
void Chat_Add(const cc_string* text) { Chat_AddOf(text, MSG_TYPE_NORMAL); }

void Chat_AddOf(const cc_string* text, int msgType) {
	cc_string str;
	if (msgType == MSG_TYPE_NORMAL) {
		/* Check for chat overflow (see issue #837) */
		/* This happens because Offset/Length are packed into a single 32 bit value, */
		/*  with 9 bits used for length. Hence if offset exceeds 2^23 (8388608), it */
		/*  overflows and earlier chat messages start wrongly appearing instead */
		if (Chat_Log.totalLength > 8388000) {
			ClearChatLogs();
			Chat_AddRaw("&cChat log cleared as it hit 8.3 million character limit");
		}

		/* StringsBuffer_Add will abort game if try to add string > 511 characters */
		str        = *text; 
		str.length = min(str.length, DRAWER2D_MAX_TEXT_LENGTH);

		Chat_GetLogTime(Chat_Log.count) = Game.Time;
		AppendChatLog(&str);
		StringsBuffer_Add(&Chat_Log, &str);
	} else if (msgType >= MSG_TYPE_STATUS_1 && msgType <= MSG_TYPE_STATUS_3) {
		/* Status[0] is for texture pack downloading message */
		/* Status[1] is for reduced performance mode message */
		String_Copy(&Chat_Status[2 + (msgType - MSG_TYPE_STATUS_1)], text);
	} else if (msgType >= MSG_TYPE_BOTTOMRIGHT_1 && msgType <= MSG_TYPE_BOTTOMRIGHT_3) {	
		String_Copy(&Chat_BottomRight[msgType - MSG_TYPE_BOTTOMRIGHT_1], text);
	} else if (msgType == MSG_TYPE_ANNOUNCEMENT) {
		String_Copy(&Chat_Announcement, text);
		Chat_AnnouncementReceived = Game.Time;
	} else if (msgType == MSG_TYPE_BIGANNOUNCEMENT) {
		String_Copy(&Chat_BigAnnouncement, text);
		Chat_BigAnnouncementReceived = Game.Time;
	} else if (msgType == MSG_TYPE_SMALLANNOUNCEMENT) {
		String_Copy(&Chat_SmallAnnouncement, text);
		Chat_SmallAnnouncementReceived = Game.Time;
	} else if (msgType >= MSG_TYPE_CLIENTSTATUS_1 && msgType <= MSG_TYPE_CLIENTSTATUS_2) {
		String_Copy(&Chat_ClientStatus[msgType - MSG_TYPE_CLIENTSTATUS_1], text);
	} else if (msgType >= MSG_TYPE_EXTRASTATUS_1 && msgType <= MSG_TYPE_EXTRASTATUS_2) {
		String_Copy(&Chat_Status[msgType - MSG_TYPE_EXTRASTATUS_1], text);
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

static cc_bool Commands_IsCommandPrefix(const cc_string* str) {
	static const cc_string prefixSpace = String_FromConst(COMMANDS_PREFIX_SPACE);
	static const cc_string prefix      = String_FromConst(COMMANDS_PREFIX);

	if (!str->length) return false;
	if (Server.IsSinglePlayer && str->buffer[0] == '/') return true;
	
	return String_CaselessStarts(str, &prefixSpace)
		|| String_CaselessEquals(str, &prefix);
}

void Commands_Register(struct ChatCommand* cmd) {
	LinkedList_Append(cmd, cmds_head, cmds_tail);
}

static struct ChatCommand* Commands_FindMatch(const cc_string* cmdName) {
	struct ChatCommand* match = NULL;
	struct ChatCommand* cmd;
	cc_string name;

	for (cmd = cmds_head; cmd; cmd = cmd->next) {
		name = String_FromReadonly(cmd->name);
		if (String_CaselessEquals(&name, cmdName)) return cmd;
	}

	for (cmd = cmds_head; cmd; cmd = cmd->next) {
		name = String_FromReadonly(cmd->name);
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
	cc_string str; char strBuffer[STRING_SIZE];
	struct ChatCommand* cmd;
	cc_string name;

	Chat_AddRaw("&eList of client commands:");
	String_InitArray(str, strBuffer);

	for (cmd = cmds_head; cmd; cmd = cmd->next) {
		name = String_FromReadonly(cmd->name);

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

static void Commands_Execute(const cc_string* input) {
	static const cc_string prefixSpace = String_FromConst(COMMANDS_PREFIX_SPACE);
	static const cc_string prefix      = String_FromConst(COMMANDS_PREFIX);
	cc_string text = *input;

	struct ChatCommand* cmd;
	int offset, count;
	cc_string name, value;
	cc_string args[50];

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

	String_UNSAFE_Separate(&text, ' ', &name, &value);
	cmd = Commands_FindMatch(&name);
	if (!cmd) return;

	if ((cmd->flags & COMMAND_FLAG_SINGLEPLAYER_ONLY) && !Server.IsSinglePlayer) {
		Chat_Add1("&e/client: \"&f%s&e\" can only be used in singleplayer.", &name);
		return;
	}

	if (cmd->flags & COMMAND_FLAG_UNSPLIT_ARGS) {
		/* argsCount = 0 if value.length is 0, 1 otherwise */
		cmd->Execute(&value, value.length != 0);
	} else {
		count = String_UNSAFE_Split(&value, ' ', args, Array_Elems(args));
		cmd->Execute(args,   value.length ? count : 0);
	}
}


/*########################################################################################################################*
*------------------------------------------------------Simple commands----------------------------------------------------*
*#########################################################################################################################*/
static void HelpCommand_Execute(const cc_string* args, int argsCount) {
	struct ChatCommand* cmd;
	int i;

	if (!argsCount) { Commands_PrintDefault(); return; }
	cmd = Commands_FindMatch(args);
	if (!cmd) return;

	for (i = 0; i < Array_Elems(cmd->help); i++) {
		if (!cmd->help[i]) continue;
		Chat_AddRaw(cmd->help[i]);
	}
}

static struct ChatCommand HelpCommand = {
	"Help", HelpCommand_Execute,
	COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client help [command name]",
		"&eDisplays the help for the given command.",
	}
};

static void GpuInfoCommand_Execute(const cc_string* args, int argsCount) {
	char buffer[7 * STRING_SIZE];
	cc_string str, line;
	String_InitArray(str, buffer);
	Gfx_GetApiInfo(&str);
	
	while (str.length) {
		String_UNSAFE_SplitBy(&str, '\n', &line);
		if (line.length) Chat_Add1("&a%s", &line);
	}
}

static struct ChatCommand GpuInfoCommand = {
	"GpuInfo", GpuInfoCommand_Execute,
	COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client gpuinfo",
		"&eDisplays information about your GPU.",
	}
};

static void RenderTypeCommand_Execute(const cc_string* args, int argsCount) {
	int flags;
	if (!argsCount) {
		Chat_AddRaw("&e/client: &cYou didn't specify a new render type."); return;
	}

	flags = EnvRenderer_CalcFlags(args);
	if (flags >= 0) {
		EnvRenderer_SetMode(flags);
		Options_Set(OPT_RENDER_TYPE, args);
		Chat_Add1("&e/client: &fRender type is now %s.", args);
	} else {
		Chat_Add1("&e/client: &cUnrecognised render type &f\"%s\"&c.", args);
	}
}

static struct ChatCommand RenderTypeCommand = {
	"RenderType", RenderTypeCommand_Execute,
	COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client rendertype [normal/legacy/fast]",
		"&bnormal: &eDefault render mode, with all environmental effects enabled",
		"&blegacy: &eSame as normal mode, &cbut is usually slightly slower",
		"   &eIf you have issues with clouds and map edges disappearing randomly, use this mode",
		"&bfast: &eSacrifices clouds, fog and overhead sky for faster performance",
	}
};

static void ResolutionCommand_Execute(const cc_string* args, int argsCount) {
	int width, height;
	if (argsCount < 2) {
		Chat_Add4("&e/client: &fCurrent resolution is %i@%f2 x %i@%f2", 
				&WindowInfo.Width, &DisplayInfo.ScaleX, &WindowInfo.Height, &DisplayInfo.ScaleY);
	} else if (!Convert_ParseInt(&args[0], &width) || !Convert_ParseInt(&args[1], &height)) {
		Chat_AddRaw("&e/client: &cWidth and height must be integers.");
	} else if (width <= 0 || height <= 0) {
		Chat_AddRaw("&e/client: &cWidth and height must be above 0.");
	} else {
		Window_SetSize(width, height);
		/* Window_Create uses these, but scales by DPI. Hence DPI unscale them here. */
		Options_SetInt(OPT_WINDOW_WIDTH,  (int)(width  / DisplayInfo.ScaleX));
		Options_SetInt(OPT_WINDOW_HEIGHT, (int)(height / DisplayInfo.ScaleY));
	}
}

static struct ChatCommand ResolutionCommand = {
	"Resolution", ResolutionCommand_Execute,
	0,
	{
		"&a/client resolution [width] [height]",
		"&ePrecisely sets the size of the rendered window.",
	}
};

static void ModelCommand_Execute(const cc_string* args, int argsCount) {
	if (argsCount) {
		Entity_SetModel(&LocalPlayer_Instance.Base, args);
	} else {
		Chat_AddRaw("&e/client model: &cYou didn't specify a model name.");
	}
}

static struct ChatCommand ModelCommand = {
	"Model", ModelCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY | COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client model [name]",
		"&bnames: &echibi, chicken, creeper, human, pig, sheep",
		"&e       skeleton, spider, zombie, sit, <numerical block id>",
	}
};

static void ClearDeniedCommand_Execute(const cc_string* args, int argsCount) {
	int count = TextureCache_ClearDenied();
	Chat_Add1("Removed &e%i &fdenied texture pack URLs.", &count);
}

static struct ChatCommand ClearDeniedCommand = {
	"ClearDenied", ClearDeniedCommand_Execute,
	COMMAND_FLAG_UNSPLIT_ARGS,
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
static const cc_string cuboid_msg = String_FromConst("&eCuboid: &fPlace or delete a block.");
static const cc_string yes_string = String_FromConst("yes");

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
	cc_string msg; char msgBuffer[STRING_SIZE];
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
			Event_Unregister_(&UserEvents.BlockChanged, NULL, CuboidCommand_BlockChanged);
			cuboid_hooked = false;
			Chat_AddOf(&String_Empty, MSG_TYPE_CLIENTSTATUS_1);
		} else {
			cuboid_hasMark1 = false;
			Chat_AddOf(&cuboid_msg, MSG_TYPE_CLIENTSTATUS_1);
		}
	}
}

static cc_bool CuboidCommand_ParseArgs(const cc_string* args) {
	cc_string value = *args;
	int block;

	/* Check for /cuboid [block] yes */
	if (String_CaselessEnds(&value, &yes_string)) {
		cuboid_persist = true; 
		value.length  -= 3;
		String_UNSAFE_TrimEnd(&value);

		/* special case "/cuboid yes" */
		if (!value.length) return true;
	}

	block = Block_Parse(&value);
	if (block == -1) {
		Chat_Add1("&eCuboid: &c\"%s\" is not a valid block name or id.", &value); return false;
	}

	if (block > Game_Version.MaxCoreBlock && !Block_IsCustomDefined(block)) {
		Chat_Add1("&eCuboid: &cThere is no block with id \"%s\".", &value); return false;
	}

	cuboid_block = block;
	return true;
}

static void CuboidCommand_Execute(const cc_string* args, int argsCount) {
	if (cuboid_hooked) {
		Event_Unregister_(&UserEvents.BlockChanged, NULL, CuboidCommand_BlockChanged);
		cuboid_hooked = false;
	}

	cuboid_block    = -1;
	cuboid_hasMark1 = false;
	cuboid_persist  = false;
	if (argsCount && !CuboidCommand_ParseArgs(args)) return;

	Chat_AddOf(&cuboid_msg, MSG_TYPE_CLIENTSTATUS_1);
	Event_Register_(&UserEvents.BlockChanged, NULL, CuboidCommand_BlockChanged);
	cuboid_hooked = true;
}

static struct ChatCommand CuboidCommand = {
	"Cuboid", CuboidCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY | COMMAND_FLAG_UNSPLIT_ARGS,
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
static void TeleportCommand_Execute(const cc_string* args, int argsCount) {
	struct Entity* e = &LocalPlayer_Instance.Base;
	struct LocationUpdate update;
	Vec3 v;

	if (argsCount != 3) {
		Chat_AddRaw("&e/client teleport: &cYou didn't specify X, Y and Z coordinates.");
		return;
	}
	if (!Convert_ParseFloat(&args[0], &v.X) || !Convert_ParseFloat(&args[1], &v.Y) || !Convert_ParseFloat(&args[2], &v.Z)) {
		Chat_AddRaw("&e/client teleport: &cCoordinates must be decimals");
		return;
	}

	update.flags = LU_HAS_POS;
	update.pos   = v;
	e->VTABLE->SetLocation(e, &update);
}

static struct ChatCommand TeleportCommand = {
	"TP", TeleportCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY,
	{
		"&a/client tp [x y z]",
		"&eMoves you to the given coordinates.",
	}
};


/*########################################################################################################################*
*------------------------------------------------------BlockEditCommand----------------------------------------------------*
*#########################################################################################################################*/
static cc_bool BlockEditCommand_GetInt(const cc_string* str, const char* name, int* value, int min, int max) {
	int maxTexs = ATLAS1D_MAX_ATLASES;

	if (!Convert_ParseInt(str, value)) {
		Chat_Add1("&eBlockEdit: &e%c must be an integer", name);
		return false;
	}

	if (*value < min || *value > max) {
		Chat_Add3("&eBlockEdit: &e%c must be between %i and %i", name, &min, &max);
		return false;
	}
	return true;
}
static cc_bool BlockEditCommand_GetTexture(const cc_string* str, int* tex) {
	return BlockEditCommand_GetInt(str, "Texture", tex, 0, ATLAS1D_MAX_ATLASES - 1);
}

static void BlockEditCommand_Execute(const cc_string* args, int argsCount__) {
	cc_string parts[3];
	cc_string* prop;
	cc_string* value;
	int argsCount, block, v;

	if (String_CaselessEqualsConst(args, "properties")) {
		Chat_AddRaw("&eEditable block properties:");
		Chat_AddRaw("&a  name &e- Sets the name of the block");
		Chat_AddRaw("&a  all &e- Sets textures on all six sides of the block");
		Chat_AddRaw("&a  sides &e- Sets textures on four sides of the block");
		Chat_AddRaw("&a  left/right/front/back/top/bottom &e- Sets one texture");
		Chat_AddRaw("&a  collide &e- Sets collision mode of the block");
		return;
	}

	argsCount = String_UNSAFE_Split(args, ' ', parts, 3);
	if (argsCount < 3) {
		Chat_AddRaw("&eBlockEdit: &eThree arguments required &e(See &a/client help blockedit&e)");
		return;
	}

	block = Block_Parse(&parts[0]);
	if (block == -1) {
		Chat_Add1("&eBlockEdit: &c\"%s\" is not a valid block name or ID", &parts[0]);
		return;
	}

	/* TODO: Redo as an array */
	prop  = &parts[1];
	value = &parts[2];
	if (String_CaselessEqualsConst(prop, "name")) {
		Block_SetName(block, value);
	} else if (String_CaselessEqualsConst(prop, "all")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_SetSide(v, block);
		Block_Tex(block, FACE_YMAX) = v;
		Block_Tex(block, FACE_YMIN) = v;
	} else if (String_CaselessEqualsConst(prop, "sides")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_SetSide(v, block);
	} else if (String_CaselessEqualsConst(prop, "left")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_Tex(block, FACE_XMIN) = v;
	} else if (String_CaselessEqualsConst(prop, "right")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_Tex(block, FACE_XMAX) = v;
	} else if (String_CaselessEqualsConst(prop, "bottom")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_Tex(block, FACE_YMIN) = v;
	}  else if (String_CaselessEqualsConst(prop, "top")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_Tex(block, FACE_YMAX) = v;
	} else if (String_CaselessEqualsConst(prop, "front")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_Tex(block, FACE_ZMIN) = v;
	} else if (String_CaselessEqualsConst(prop, "back")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_Tex(block, FACE_ZMAX) = v;
	} else if (String_CaselessEqualsConst(prop, "collide")) {
		if (!BlockEditCommand_GetInt(value, "Collide mode", &v, 0, COLLIDE_CLIMB)) return;

		Blocks.Collide[block] = v;
	} else {
		Chat_Add1("&eBlockEdit: &eUnknown property %s &e(See &a/client help blockedit&e)", prop);
		return;
	}

	Block_DefineCustom(block, false);
}

static struct ChatCommand BlockEditCommand = {
	"BlockEdit", BlockEditCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY | COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client blockedit [block] [property] [value]",
		"&eEdits the given property of the given block",
		"&a/client blockedit properties",
		"&eLists the editable block properties",
	}
};


/*########################################################################################################################*
*-------------------------------------------------------Generic chat------------------------------------------------------*
*#########################################################################################################################*/
static void LogInputUsage(const cc_string* text) {
	/* Simplify navigating through input history by not logging duplicate entries */
	if (Chat_InputLog.count) {
		int lastIndex  = Chat_InputLog.count - 1;
		cc_string last = StringsBuffer_UNSAFE_Get(&Chat_InputLog, lastIndex);

		if (String_Equals(text, &last)) return;
	}
	StringsBuffer_Add(&Chat_InputLog, text);
}

void Chat_Send(const cc_string* text, cc_bool logUsage) {
	if (!text->length) return;
	Event_RaiseChat(&ChatEvents.ChatSending, text, 0);
	if (logUsage) LogInputUsage(text);

	if (Commands_IsCommandPrefix(text)) {
		Commands_Execute(text);
	} else {
		Server.SendChat(text);
	}
}

static void OnInit(void) {
	Commands_Register(&GpuInfoCommand);
	Commands_Register(&HelpCommand);
	Commands_Register(&RenderTypeCommand);
	Commands_Register(&ResolutionCommand);
	Commands_Register(&ModelCommand);
	Commands_Register(&CuboidCommand);
	Commands_Register(&TeleportCommand);
	Commands_Register(&ClearDeniedCommand);
	Commands_Register(&BlockEditCommand);

#if defined CC_BUILD_MOBILE || defined CC_BUILD_WEB
	/* Better to not log chat by default on mobile/web, */
	/* since it's not easily visible to end users */
	Chat_Logging = Options_GetBool(OPT_CHAT_LOGGING, false);
#else
	Chat_Logging = Options_GetBool(OPT_CHAT_LOGGING, true);
#endif
}

static void ClearCPEMessages(void) {
	Chat_AddOf(&String_Empty, MSG_TYPE_ANNOUNCEMENT);
	Chat_AddOf(&String_Empty, MSG_TYPE_BIGANNOUNCEMENT);
	Chat_AddOf(&String_Empty, MSG_TYPE_SMALLANNOUNCEMENT);
	Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_1);
	Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_2);
	Chat_AddOf(&String_Empty, MSG_TYPE_STATUS_3);
	Chat_AddOf(&String_Empty, MSG_TYPE_BOTTOMRIGHT_1);
	Chat_AddOf(&String_Empty, MSG_TYPE_BOTTOMRIGHT_2);
	Chat_AddOf(&String_Empty, MSG_TYPE_BOTTOMRIGHT_3);
}

static void OnReset(void) {
	CloseLogFile();
	ResetLogFile();
	ClearCPEMessages();
}

static void OnFree(void) {
	CloseLogFile();
	ClearCPEMessages();
	cmds_head = NULL;

	ClearChatLogs();
	StringsBuffer_Clear(&Chat_InputLog);
}

struct IGameComponent Chat_Component = {
	OnInit, /* Init  */
	OnFree, /* Free  */
	OnReset /* Reset */
};
