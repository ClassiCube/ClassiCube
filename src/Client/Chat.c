#include "Chat.h"
#include "Stream.h"
#include "Platform.h"
#include "Event.h"
#include "Game.h"
#include "ErrorHandler.h"
#include "ServerConnection.h"

void ChatLine_Make(ChatLine* line, STRING_TRANSIENT String* text) {
	String dst = String_InitAndClearArray(line->Buffer);
	String_AppendString(&dst, text);
	line->Received = Platform_CurrentUTCTime();
}

UInt8 Chat_LogNameBuffer[String_BufferSize(STRING_SIZE)];
String Chat_LogName = String_FromEmptyArray(Chat_LogNameBuffer);
Stream Chat_LogStream;
DateTime ChatLog_LastLogDate;

void Chat_Send(STRING_PURE String* text) {
	if (text->length == 0) return;
	StringsBuffer_Add(&InputLog, text);

	if (Commands_IsCommandPrefix(text)) {
		Commands_Execute(text);
	} else {
		ServerConnection_SendChat(text);
	}
}

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

void Chat_Add(STRING_PURE String* text) { Chat_AddOf(text, MESSAGE_TYPE_NORMAL); }
void Chat_AddOf(STRING_PURE String* text, MessageType type) {
	if (type == MESSAGE_TYPE_NORMAL) {
		StringsBuffer_Add(&ChatLog, text);
		Chat_AppendLog(text);
	} else if (type >= MESSAGE_TYPE_STATUS_1 && type <= MESSAGE_TYPE_STATUS_3) {
		ChatLine_Make(&Chat_Status[type - MESSAGE_TYPE_STATUS_1], text);
	} else if (type >= MESSAGE_TYPE_BOTTOMRIGHT_1 && type <= MESSAGE_TYPE_BOTTOMRIGHT_3) {
		ChatLine_Make(&Chat_BottomRight[type - MESSAGE_TYPE_BOTTOMRIGHT_1], text);
	} else if (type == MESSAGE_TYPE_ANNOUNCEMENT) {
		ChatLine_Make(&Chat_Announcement, text);
	} else if (type >= MESSAGE_TYPE_CLIENTSTATUS_1 && type <= MESSAGE_TYPE_CLIENTSTATUS_3) {
		ChatLine_Make(&Chat_ClientStatus[type - MESSAGE_TYPE_CLIENTSTATUS_1], text);
	}
	Event_RaiseChat(&ChatEvents_ChatReceived, text, type);
}

void Chat_Reset(void) {
	Chat_CloseLog();
	String_Clear(&Chat_LogName);
}

IGameComponent Chat_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Reset = Chat_Reset;
	return comp;
}

typedef struct ChatCommand_ {
	String Name;
	String Help[6];
	void (*Execute)(STRING_PURE String* args, UInt32 argsCount);
	bool SingleplayerOnly;
} ChatCommand;
typedef void (*ChatCommandConstructor)(ChatCommand* cmd);

#define COMMANDS_PREFIX "/client"
#define COMMANDS_PREFIX_STARTS "/client "
bool Commands_IsCommandPrefix(STRING_PURE String* input) {
	if (input->length == 0) return false;
	if (ServerConnection_IsSinglePlayer && input->buffer[0] == '/')
		return true;

	String starts = String_FromConst(COMMANDS_PREFIX_STARTS);
	String prefix = String_FromConst(COMMANDS_PREFIX);
	return String_CaselessStarts(input, &starts)
		|| String_CaselessEquals(input, &prefix);
}


void HelpCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	if (argsCount == 1) {
		game.Chat.Add("&eList of client commands:");
		Commands_PrintDefined();
		game.Chat.Add("&eTo see help for a command, type /client help [cmd name]");
	} else {
		ChatCommand* cmd = Commands_GetMatch(&args[1]);
		if (cmd == NULL) return;

		UInt32 i;
		for (i = 0; i < Array_NumElements(cmd->Help); i++) {
			String* help = &cmd->Help[i];
			if (help->length == 0) continue;
			Chat_Add(help);
		}
	}
}

void HelpCommand_Make(ChatCommand* cmd) {
	String name  = String_FromConst("Help");                                       cmd->Name    = name;
	String help0 = String_FromConst("&a/client help [command name]");              cmd->Help[0] = help0;
	String help1 = String_FromConst("&eDisplays the help for the given command."); cmd->Help[1] = help1;
	cmd->Execute = HelpCommand_Execute;
}

void GpuInfoCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	string[] lines = game.Graphics.ApiInfo;
	for (int i = 0; i < lines.Length; i++) {
		game.Chat.Add("&a" + lines[i]);
	}
}

void GpuInfoCommand_Make(ChatCommand* cmd) {
	String name  = String_FromConst("GpuInfo");                                cmd->Name    = name;
	String help0 = String_FromConst("&a/client gpuinfo");                      cmd->Help[0] = help0;
	String help1 = String_FromConst("&eDisplays information about your GPU."); cmd->Help[1] = help1;
}

void RenderTypeCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	if (argsCount == 1) {
		game.Chat.Add("&e/client: &cYou didn't specify a new render type.");
	} else if (game.SetRenderType(args[1])) {
		game.Chat.Add("&e/client: &fRender type is now " + args[1] + ".");
	} else {
		game.Chat.Add("&e/client: &cUnrecognised render type &f\"" + args[1] + "\"&c.");
	}
}

void RenderTypeCommand_Make(ChatCommand* cmd) {
	String name  = String_FromConst("RenderType");                                                                                   cmd->Name    = name;
	String help0 = String_FromConst("&a/client rendertype [normal/legacy/legacyfast]");                                              cmd->Help[0] = help0;
	String help1 = String_FromConst("&bnormal: &eDefault renderer, with all environmental effects enabled.");                        cmd->Help[1] = help1;
	String help2 = String_FromConst("&blegacy: &eMay be slightly slower than normal, but produces the same environmental effects."); cmd->Help[2] = help2;
	String help3 = String_FromConst("&blegacyfast: &eSacrifices clouds, fog and overhead sky for faster performance.");              cmd->Help[3] = help3;
	String help4 = String_FromConst("&bnormalfast: &eSacrifices clouds, fog and overhead sky for faster performance.");              cmd->Help[4] = help4;
}

void ResolutionCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	Int32 width, height;
	if (argsCount < 3) {
		game.Chat.Add("&e/client: &cYou didn't specify width and height");
	} else if (!Convert_TryParseInt32(&args[1], &width) || !Convert_TryParseInt32(&args[2], &height)) {
		game.Chat.Add("&e/client: &cWidth and height must be integers.");
	} else if (width <= 0 || height <= 0) {
		game.Chat.Add("&e/client: &cWidth and height must be above 0.");
	} else {
		game.window.ClientSize = new Size(width, height);
		Options_SetInt32(OPTION_WINDOW_WIDTH,  width);
		Options_SetInt32(OPTION_WINDOW_HEIGHT, height);
	}
}

void ResolutionCommand_Make(ChatCommand* cmd) {
	String name  = String_FromConst("Resolution");                                        cmd->Name    = name;
	String help0 = String_FromConst("&a/client resolution [width] [height]");             cmd->Help[0] = help0;
	String help1 = String_FromConst("&ePrecisely sets the size of the rendered window."); cmd->Help[1] = help1;
}

void ModelCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
	if (argsCount == 1) {
		game.Chat.Add("&e/client model: &cYou didn't specify a model name.");
	} else {
		game.LocalPlayer.SetModel(Utils.ToLower(args[1]));
	}
}

void ModelCommand_Make(ChatCommand* cmd) {
	String name  = String_FromConst("Model");                                                            cmd->Name    = name;
	String help0 = String_FromConst("&a/client model [name]");                                           cmd->Help[0] = help0;
	String help1 = String_FromConst("&bnames: &echibi, chicken, creeper, human, pig, sheep");            cmd->Help[1] = help1;
	String help2 = String_FromConst("&e       skeleton, spider, zombie, sitting, <numerical block id>"); cmd->Help[2] = help2;
	cmd->SingleplayerOnly = true;
}

void CuboidCommand_Make(ChatCommand* cmd) {
	String name = String_FromConst("Cuboid");
	Help = new string[]{
		"&a/client cuboid [block] [persist]",
		"&eFills the 3D rectangle between two points with [block].",
		"&eIf no block is given, uses your currently held block.",
		"&e  If persist is given and is \"yes\", then the command",
		"&e  will repeatedly cuboid, without needing to be typed in again.",
	};
	cmd->SingleplayerOnly = true;
}
	int block = -1;
	Vector3I mark1, mark2;
	bool persist = false;

	void CuboidCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
		game.UserEvents.BlockChanged -= BlockChanged;
		block = -1;
		mark1 = new Vector3I(int.MaxValue);
		mark2 = new Vector3I(int.MaxValue);
		persist = false;

		if (!ParseBlock(args)) return;
		if (args.Length > 2 && Utils.CaselessEquals(args[2], "yes"))
			persist = true;

		game.Chat.Add("&eCuboid: &fPlace or delete a block.", MessageType.ClientStatus3);
		game.UserEvents.BlockChanged += BlockChanged;
	}

	bool ParseBlock(string[] args) {
		if (args.Length == 1) return true;
		if (Utils.CaselessEquals(args[1], "yes")) { persist = true; return true; }

		int temp = -1;
		BlockID block = 0;
		if ((temp = BlockInfo.FindID(args[1])) != -1) {
			block = (BlockID)temp;
		}
		else if (!BlockID.TryParse(args[1], out block)) {
			game.Chat.Add("&eCuboid: &c\"" + args[1] + "\" is not a valid block name or id."); return false;
		}

		if (block >= Block.CpeCount && BlockInfo.Name[block] == "Invalid") {
			game.Chat.Add("&eCuboid: &cThere is no block with id \"" + args[1] + "\"."); return false;
		}
		this.block = block;
		return true;
	}

	void BlockChanged(object sender, BlockChangedEventArgs e) {
		if (mark1.X == int.MaxValue) {
			mark1 = e.Coords;
			game.UpdateBlock(mark1.X, mark1.Y, mark1.Z, e.OldBlock);
			game.Chat.Add("&eCuboid: &fMark 1 placed at (" + e.Coords + "), place mark 2.",
				MessageType.ClientStatus3);
		}
		else {
			mark2 = e.Coords;
			DoCuboid();
			game.Chat.Add(null, MessageType.ClientStatus3);

			if (!persist) {
				game.UserEvents.BlockChanged -= BlockChanged;
			}
			else {
				mark1 = new Vector3I(int.MaxValue);
				game.Chat.Add("&eCuboid: &fPlace or delete a block.", MessageType.ClientStatus3);
			}
		}
	}

	void DoCuboid() {
		Vector3I min = Vector3I.Min(mark1, mark2);
		Vector3I max = Vector3I.Max(mark1, mark2);
		if (!game.World.IsValidPos(min) || !game.World.IsValidPos(max)) return;

		BlockID toPlace = (BlockID)block;
		if (block == -1) toPlace = game.Inventory.Selected;

		for (int y = min.Y; y <= max.Y; y++)
			for (int z = min.Z; z <= max.Z; z++)
				for (int x = min.X; x <= max.X; x++)
				{
					game.UpdateBlock(x, y, z, toPlace);
				}
	}
}

void TeleportCommand_Make(ChatCommand* cmd) {
	String name  = String_FromConst("TP");                                    cmd->Name    = name;
	String help0 = String_FromConst("&a/client tp [x y z]");                  cmd->Help[0] = help0;
	String help1 = String_FromConst("&eMoves you to the given coordinates."); cmd->Help[1] = help1;
	cmd->SingleplayerOnly = true;
}

	void TeleportCommand_Execute(STRING_PURE String* args, UInt32 argsCount) {
		if (args.Length != 4) {
			game.Chat.Add("&e/client teleport: &cYou didn't specify X, Y and Z coordinates.");
		} else {
			float x = 0, y = 0, z = 0;
			if (!Utils.TryParseDecimal(args[1], out x) ||
				!Utils.TryParseDecimal(args[2], out y) ||
				!Utils.TryParseDecimal(args[3], out z)) {
				game.Chat.Add("&e/client teleport: &cCoordinates must be decimals");
				return;
			}

			Vector3 v = new Vector3(x, y, z);
			LocationUpdate update = LocationUpdate.MakePos(v, false);
			game.LocalPlayer.SetLocation(update, false);
		}
	}
}
}

ChatCommand commands_List[10];
UInt32 commands_Count;
void Init() {
	Register(new GpuInfoCommand());
	Register(new HelpCommand());
	Register(new RenderTypeCommand());
	Register(new ResolutionCommand());
	Register(new ModelCommand());
	Register(new CuboidCommand());
	Register(new TeleportCommand());
}

void Register(Command command) {
	RegisteredCommands.Add(command);
}

ChatCommand* Commands_GetMatch(STRING_PURE String* cmdName) {
	ChatCommand* match = NULL;
	UInt32 i;
	for (i = 0; i < commands_Count; i++) {
		ChatCommand* cmd = &commands_List[i];
		if (!String_CaselessStarts(&cmd->Name, cmdName)) continue;

		if (match != NULL) {
			game.Chat.Add("&e/client: Multiple commands found that start with: \"&f" + cmdName + "&e\".");
			return NULL;
		}
		match = cmd;
	}

	if (match == NULL) {
		game.Chat.Add("&e/client: Unrecognised command: \"&f" + cmdName + "&e\".");
		game.Chat.Add("&e/client: Type &a/client &efor a list of commands.");
		return NULL;
	}
	if (match->SingleplayerOnly && !ServerConnection_IsSinglePlayer) {
		game.Chat.Add("&e/client: \"&f" + cmdName + "&e\" can only be used in singleplayer.");
		return NULL;
	}
	return match;
}

void Commands_Execute(STRING_PURE String* input) {
	String text = *input;
	String prefix = String_FromConst(COMMANDS_PREFIX);

	if (String_CaselessStarts(&text, &prefix)) { /* /client command args */
		text = String_UNSAFE_SubstringAt(&text, prefix.length);
		text = text.TrimStart(splitChar);
	} else { /* /command args */
		text = String_UNSAFE_SubstringAt(&text, 1);
	}

	if (text.length == 0) { /* only / or /client */
		String m1 = String_FromConst("&eList of client commands:"); Chat_Add(&m1);
		Commands_PrintDefined();
		String m2 = String_FromConst("&eTo see help for a command, type &a/client help [cmd name]"); Chat_Add(&m2);
		return;
	}

	String args[10];
	UInt32 argsCount = Array_NumElements(args);
	String_UNSAFE_Split(&text, ' ', args, &argsCount);

	ChatCommand* cmd = Commands_GetMatch(&args[0]);
	if (cmd == NULL) return;
	cmd->Execute(args, argsCount);
}

void Commands_PrintDefined(void) {
	UInt8 strBuffer[String_BufferSize(STRING_SIZE)];
	String str = String_InitAndClearArray(strBuffer);
	UInt32 i;

	for (i = 0; i < commands_Count; i++) {
		ChatCommand* cmd = &commands_List[i];
		String name = cmd->Name;

		if ((str.length + name.length + 2) > str.capacity) {
			Chat_Add(&str);
			String_Clear(&str);
		}
		String_Append(&str, &name);
		String_AppendConst(&str, ", ");
	}

	if (str.length > 0) { Chat_Add(&str); }
}

void Dispose() {
	RegisteredCommands.Clear();
}