#include "Commands.h"
#include "Chat.h"
#include "String.h"
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

#define COMMANDS_PREFIX "/client"
#define COMMANDS_PREFIX_SPACE "/client "
static struct ChatCommand* cmds_head;
static struct ChatCommand* cmds_tail;

void Commands_Register(struct ChatCommand* cmd) {
	LinkedList_Append(cmd, cmds_head, cmds_tail);
}


/*########################################################################################################################*
*------------------------------------------------------Command handling---------------------------------------------------*
*#########################################################################################################################*/
static struct ChatCommand* Commands_FindMatch(const cc_string* cmdName) {
	struct ChatCommand* match = NULL;
	struct ChatCommand* cmd;
	cc_string name;

	for (cmd = cmds_head; cmd; cmd = cmd->next) 
	{
		name = String_FromReadonly(cmd->name);
		if (String_CaselessEquals(&name, cmdName)) return cmd;
	}

	for (cmd = cmds_head; cmd; cmd = cmd->next) 
	{
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

cc_bool Commands_Execute(const cc_string* input) {
	static const cc_string prefixSpace = String_FromConst(COMMANDS_PREFIX_SPACE);
	static const cc_string prefix      = String_FromConst(COMMANDS_PREFIX);
	cc_string text;

	struct ChatCommand* cmd;
	int offset, count;
	cc_string name, value;
	cc_string args[50];

	if (String_CaselessStarts(input, &prefixSpace)) { 
		/* /client [command] [args] */
		offset = prefixSpace.length;
	} else if (String_CaselessEquals(input, &prefix)) { 
		/* /client */
		offset = prefix.length;
	} else if (Server.IsSinglePlayer && String_CaselessStarts(input, &prefix)) {
		/* /client[command] [args] */
		offset = prefix.length;
	} else if (Server.IsSinglePlayer && input->length && input->buffer[0] == '/') {
		/* /[command] [args] */
		offset = 1;
	} else {
		return false;
	}

	text = String_UNSAFE_SubstringAt(input, offset);
	/* Check for only / or /client */
	if (!text.length) { Commands_PrintDefault(); return true; }

	String_UNSAFE_Separate(&text, ' ', &name, &value);
	cmd = Commands_FindMatch(&name);
	if (!cmd) return true;

	if ((cmd->flags & COMMAND_FLAG_SINGLEPLAYER_ONLY) && !Server.IsSinglePlayer) {
		Chat_Add1("&e/client: \"&f%s&e\" can only be used in singleplayer.", &name);
		return true;
	}

	if (cmd->flags & COMMAND_FLAG_UNSPLIT_ARGS) {
		/* argsCount = 0 if value.length is 0, 1 otherwise */
		cmd->Execute(&value, value.length != 0);
	} else {
		count = String_UNSAFE_Split(&value, ' ', args, Array_Elems(args));
		cmd->Execute(args,   value.length ? count : 0);
	}
	return true;
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
				&Window_Main.Width, &DisplayInfo.ScaleX, &Window_Main.Height, &DisplayInfo.ScaleY);
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
		Entity_SetModel(&Entities.CurPlayer->Base, args);
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

static void SkinCommand_Execute(const cc_string* args, int argsCount) {
	if (argsCount) {
		Entity_SetSkin(&Entities.CurPlayer->Base, args);
	} else {
		Chat_AddRaw("&e/client skin: &cYou didn't specify a skin name.");
	}
}

static struct ChatCommand SkinCommand = {
	"Skin", SkinCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY | COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client skin [name]",
		"&eChanges to the skin to the given player",
		"&a/client skin [url]",
		"&eChanges skin to a URL linking directly to a .PNG",
	}
};

static void ClearDeniedCommand_Execute(const cc_string* args, int argsCount) {
	int count = TextureUrls_ClearDenied();
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

static void MotdCommand_Execute(const cc_string* args, int argsCount) {
	if (Server.IsSinglePlayer) {
		Chat_AddRaw("&eThis command can only be used in multiplayer.");
		return;
	}
	Chat_Add1("&eName: &f%s", &Server.Name);
	Chat_Add1("&eMOTD: &f%s", &Server.MOTD);
}

static struct ChatCommand MotdCommand = {
	"Motd", MotdCommand_Execute,
	COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client motd",
		"&eDisplays the server's name and MOTD."
	}
};

/*#######################################################################################################################*
*-------------------------------------------------------PlaceCommand-----------------------------------------------------*
*########################################################################################################################*/

static void PlaceCommand_Execute(const cc_string* args, int argsCount) {
	cc_string name;
	cc_uint8 off;
	int block;
	IVec3 pos;
	
	if (argsCount == 2) {
		Chat_AddRaw("&eToo few arguments.");
		return;
	}
	
	block = !argsCount || argsCount == 3 ? Inventory_SelectedBlock : Block_Parse(&args[0]);
	
	if (block == -1) {
		Chat_AddRaw("&eCould not parse block.");
		return;
	}
	if (block > Game_Version.MaxCoreBlock && !Block_IsCustomDefined(block)) {
		Chat_Add1("&eThere is no block with id \"%i\".", &block); 
		return;
	}
	
	if (argsCount > 2) {
		off = argsCount == 4;
		if (!Convert_ParseInt(&args[0 + off], &pos.x) || !Convert_ParseInt(&args[1 + off], &pos.y) || !Convert_ParseInt(&args[2 + off], &pos.z)) {
			Chat_AddRaw("&eCould not parse coordinates.");
			return;
		}
	} else {
		IVec3_Floor(&pos, &Entities.CurPlayer->Base.Position);
	}
	
	if (!World_Contains(pos.x, pos.y, pos.z)) {
		Chat_AddRaw("&eCoordinates are outside the world boundaries.");
		return;
	}
	
	Game_ChangeBlock(pos.x, pos.y, pos.z, block);
	name = Block_UNSAFE_GetName(block);
	Chat_Add4("&eSuccessfully placed %s block at (%i, %i, %i).", &name, &pos.x, &pos.y, &pos.z);
}

static struct ChatCommand PlaceCommand = {
	"Place", PlaceCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY,
	{
		"&a/client place [block] [x y z]",
		"&ePlaces block at [x y z].",
		"&eIf no block is provided, held block is used.",
		"&eIf no coordinates are provided, current",
		"&e coordinates are used."
	}
};

/*########################################################################################################################*
*-------------------------------------------------------DrawOpCommand-----------------------------------------------------*
*#########################################################################################################################*/
static IVec3 drawOp_mark1, drawOp_mark2;
static cc_bool drawOp_persist, drawOp_hooked, drawOp_hasMark1;
static const char* drawOp_name;
static void (*drawOp_Func)(IVec3 min, IVec3 max);

static void DrawOpCommand_BlockChanged(void* obj, IVec3 coords, BlockID old, BlockID now);
static void DrawOpCommand_ResetState(void) {
	if (drawOp_hooked) {
		Event_Unregister_(&UserEvents.BlockChanged, NULL, DrawOpCommand_BlockChanged);
		drawOp_hooked = false;
	}

	drawOp_hasMark1 = false;
}

static void DrawOpCommand_Begin(void) {
	cc_string msg; char msgBuffer[STRING_SIZE];
	String_InitArray(msg, msgBuffer);

	String_Format1(&msg, "&e%c: &fPlace or delete a block.", drawOp_name);
	Chat_AddOf(&msg, MSG_TYPE_CLIENTSTATUS_1);

	Event_Register_(&UserEvents.BlockChanged, NULL, DrawOpCommand_BlockChanged);
	drawOp_hooked = true;
}


static void DrawOpCommand_Execute(void) {
	IVec3 min, max;

	IVec3_Min(&min, &drawOp_mark1, &drawOp_mark2);
	IVec3_Max(&max, &drawOp_mark1, &drawOp_mark2);
	if (!World_Contains(min.x, min.y, min.z)) return;
	if (!World_Contains(max.x, max.y, max.z)) return;

	drawOp_Func(min, max);
}

static void DrawOpCommand_BlockChanged(void* obj, IVec3 coords, BlockID old, BlockID now) {
	cc_string msg; char msgBuffer[STRING_SIZE];
	String_InitArray(msg, msgBuffer);
	Game_UpdateBlock(coords.x, coords.y, coords.z, old);

	if (!drawOp_hasMark1) {
		drawOp_mark1    = coords;
		drawOp_hasMark1 = true;

		String_Format4(&msg, "&e%c: &fMark 1 placed at (%i, %i, %i), place mark 2.", 
						drawOp_name, &coords.x, &coords.y, &coords.z);
		Chat_AddOf(&msg, MSG_TYPE_CLIENTSTATUS_1);
	} else {
		drawOp_mark2 = coords;

		DrawOpCommand_Execute();
		DrawOpCommand_ResetState();

		if (!drawOp_persist) {			
			Chat_AddOf(&String_Empty, MSG_TYPE_CLIENTSTATUS_1);
		} else {
			DrawOpCommand_Begin();
		}
	}
}

static const cc_string yes_string = String_FromConst("yes");
static void DrawOpCommand_ExtractPersistArg(cc_string* value) {
	drawOp_persist = false;
	if (!String_CaselessEnds(value, &yes_string)) return;

	drawOp_persist = true; 
	value->length  -= 3;
	String_UNSAFE_TrimEnd(value);
}

static int DrawOpCommand_ParseBlock(const cc_string* arg) {
	int block = Block_Parse(arg);

	if (block == -1) {
		Chat_Add2("&e%c: &c\"%s\" is not a valid block name or id.", drawOp_name, arg); 
		return -1;
	}

	if (block > Game_Version.MaxCoreBlock && !Block_IsCustomDefined(block)) {
		Chat_Add2("&e%c: &cThere is no block with id \"%s\".", drawOp_name, arg); 
		return -1;
	}
	return block;
}


/*########################################################################################################################*
*-------------------------------------------------------CuboidCommand-----------------------------------------------------*
*#########################################################################################################################*/
static int cuboid_block;

static void CuboidCommand_Draw(IVec3 min, IVec3 max) {
	BlockID toPlace;
	int x, y, z;

	toPlace = (BlockID)cuboid_block;
	if (cuboid_block == -1) toPlace = Inventory_SelectedBlock;

	for (y = min.y; y <= max.y; y++) {
		for (z = min.z; z <= max.z; z++) {
			for (x = min.x; x <= max.x; x++) {
				Game_ChangeBlock(x, y, z, toPlace);
			}
		}
	}
}

static void CuboidCommand_Execute(const cc_string* args, int argsCount) {
	cc_string value = *args;

	DrawOpCommand_ResetState();
	drawOp_name = "Cuboid";
	drawOp_Func = CuboidCommand_Draw;

	DrawOpCommand_ExtractPersistArg(&value);
	cuboid_block = -1; /* Default to cuboiding with currently held block */

	if (value.length) {
		cuboid_block = DrawOpCommand_ParseBlock(&value);
		if (cuboid_block == -1) return;
	}

	DrawOpCommand_Begin();
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
*-------------------------------------------------------ReplaceCommand-----------------------------------------------------*
*#########################################################################################################################*/
static int replace_source, replace_target;

static void ReplaceCommand_Draw(IVec3 min, IVec3 max) {
	BlockID cur, source, toPlace;
	int x, y, z;

	source  = (BlockID)replace_source;
	toPlace = (BlockID)replace_target;
	if (replace_target == -1) toPlace = Inventory_SelectedBlock;

	for (y = min.y; y <= max.y; y++) {
		for (z = min.z; z <= max.z; z++) {
			for (x = min.x; x <= max.x; x++) {
				cur = World_GetBlock(x, y, z);
				if (cur != source) continue;
				Game_ChangeBlock(x, y, z, toPlace);
			}
		}
	}
}

static void ReplaceCommand_Execute(const cc_string* args, int argsCount) {
	cc_string value = *args;
	cc_string parts[2];
	int count;

	DrawOpCommand_ResetState();
	drawOp_name = "Replace";
	drawOp_Func = ReplaceCommand_Draw;

	DrawOpCommand_ExtractPersistArg(&value);
	replace_target = -1; /* Default to replacing with currently held block */

	if (!value.length) {
		Chat_AddRaw("&eReplace: &cAt least one argument is required"); return;
	}
	count = String_UNSAFE_Split(&value, ' ', parts, 2);

	replace_source = DrawOpCommand_ParseBlock(&parts[0]);
	if (replace_source == -1) return;

	if (count > 1) {
		replace_target = DrawOpCommand_ParseBlock(&parts[1]);
		if (replace_target == -1) return;
	}

	DrawOpCommand_Begin();
}

static struct ChatCommand ReplaceCommand = {
	"Replace", ReplaceCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY | COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client replace [source] [replacement] [persist]",
		"&eReplaces all [source] blocks between two points with [replacement].",
		"&eIf no [replacement] is given, replaces with your currently held block.",
		"&e  If persist is given and is \"yes\", then the command",
		"&e  will repeatedly replace, without needing to be typed in again.",
	}
};


/*########################################################################################################################*
*------------------------------------------------------TeleportCommand----------------------------------------------------*
*#########################################################################################################################*/
static void TeleportCommand_Execute(const cc_string* args, int argsCount) {
	struct Entity* e = &Entities.CurPlayer->Base;
	struct LocationUpdate update;
	Vec3 v;

	if (argsCount != 3) {
		Chat_AddRaw("&e/client teleport: &cYou didn't specify X, Y and Z coordinates.");
		return;
	}
	if (!Convert_ParseFloat(&args[0], &v.x) || !Convert_ParseFloat(&args[1], &v.y) || !Convert_ParseFloat(&args[2], &v.z)) {
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

static cc_bool BlockEditCommand_GetCoords(const cc_string* str, Vec3* coords) {
	cc_string parts[3];
	IVec3 xyz;

	int numParts = String_UNSAFE_Split(str, ' ', parts, 3);
	if (numParts != 3) {
		Chat_AddRaw("&eBlockEdit: &c3 values are required for a coordinate (X Y Z)");
		return false;
	}

	if (!BlockEditCommand_GetInt(&parts[0], "X coord", &xyz.x, -127, 127)) return false;
	if (!BlockEditCommand_GetInt(&parts[1], "Y coord", &xyz.y, -127, 127)) return false;
	if (!BlockEditCommand_GetInt(&parts[2], "Z coord", &xyz.z, -127, 127)) return false;

	coords->x = xyz.x / 16.0f;
	coords->y = xyz.y / 16.0f;
	coords->z = xyz.z / 16.0f;
	return true;
}

static cc_bool BlockEditCommand_GetBool(const cc_string* str, const char* name, cc_bool* value) {
	if (String_CaselessEqualsConst(str, "true") || String_CaselessEqualsConst(str, "yes")) {
		*value = true;
		return true;
	}

	if (String_CaselessEqualsConst(str, "false") || String_CaselessEqualsConst(str, "no")) {
		*value = false;
		return true;
	}

	Chat_Add1("&eBlockEdit: &e%c must be either &ayes &eor &ano", name);
	return false;
}


static void BlockEditCommand_Execute(const cc_string* args, int argsCount__) {
	cc_string parts[3];
	cc_string* prop;
	cc_string* value;
	int argsCount, block, v;
	cc_bool b;
	Vec3 coords;

	if (String_CaselessEqualsConst(args, "properties")) {
		Chat_AddRaw("&eEditable block properties:");
		Chat_AddRaw("&a  name &e- Sets the name of the block");
		Chat_AddRaw("&a  all &e- Sets textures on all six sides of the block");
		Chat_AddRaw("&a  sides &e- Sets textures on four sides of the block");
		Chat_AddRaw("&a  left/right/front/back/top/bottom &e- Sets one texture");
		Chat_AddRaw("&a  collide &e- Sets collision mode of the block");
		Chat_AddRaw("&a  drawmode &e- Sets draw mode of the block");
		Chat_AddRaw("&a  min/max &e- Sets min/max corner coordinates of the block");
		Chat_AddRaw("&eSee &a/client blockedit properties 2 &efor more properties");
		return;
	}
	if (String_CaselessEqualsConst(args, "properties 2")) {
		Chat_AddRaw("&eEditable block properties (page 2):");
		Chat_AddRaw("&a  walksound &e- Sets walk/step sound of the block");
		Chat_AddRaw("&a  breaksound &e- Sets break sound of the block");
		Chat_AddRaw("&a  fullbright &e- Sets whether the block is fully lit");
		Chat_AddRaw("&a  blockslight &e- Sets whether the block stops light");
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
	} else if (String_CaselessEqualsConst(prop, "drawmode")) {
		if (!BlockEditCommand_GetInt(value, "Draw mode", &v, 0, DRAW_SPRITE)) return;

		Blocks.Draw[block] = v;
	} else if (String_CaselessEqualsConst(prop, "min")) {
		if (!BlockEditCommand_GetCoords(value, &coords)) return;

		Blocks.MinBB[block] = coords;
	} else if (String_CaselessEqualsConst(prop, "max")) {
		if (!BlockEditCommand_GetCoords(value, &coords)) return;

		Blocks.MaxBB[block] = coords;
	} else if (String_CaselessEqualsConst(prop, "walksound")) {
		if (!BlockEditCommand_GetInt(value, "Sound", &v, 0, SOUND_COUNT - 1)) return;

		Blocks.StepSounds[block] = v;
	} else if (String_CaselessEqualsConst(prop, "breaksound")) {
		if (!BlockEditCommand_GetInt(value, "Sound", &v, 0, SOUND_COUNT - 1)) return;

		Blocks.DigSounds[block]  = v;
	} else if (String_CaselessEqualsConst(prop, "fullbright")) {
		if (!BlockEditCommand_GetBool(value, "Full brightness", &b))  return;
		//TODO: Fix this, brightness isn't just a bool anymore
		Blocks.Brightness[block] = b;
	} else if (String_CaselessEqualsConst(prop, "blockslight")) {
		if (!BlockEditCommand_GetBool(value, "Blocks light", &b)) return;

		Blocks.BlocksLight[block] = b;
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
*------------------------------------------------------Commands component-------------------------------------------------*
*#########################################################################################################################*/
static void OnInit(void) {
	Commands_Register(&GpuInfoCommand);
	Commands_Register(&HelpCommand);
	Commands_Register(&RenderTypeCommand);
	Commands_Register(&ResolutionCommand);
	Commands_Register(&ModelCommand);
	Commands_Register(&SkinCommand);
	Commands_Register(&TeleportCommand);
	Commands_Register(&ClearDeniedCommand);
	Commands_Register(&MotdCommand);
	Commands_Register(&PlaceCommand);
	Commands_Register(&BlockEditCommand);
	Commands_Register(&CuboidCommand);
	Commands_Register(&ReplaceCommand);
}

static void OnFree(void) {
	cmds_head = NULL;
}

struct IGameComponent Commands_Component = {
	OnInit, /* Init  */
	OnFree  /* Free  */
};
