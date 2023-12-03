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

static cc_bool IsSingleplayerCommandPrefix(const cc_string* input) {
	return input->length && Server.IsSinglePlayer && input->buffer[0] == '/';
}

cc_bool Commands_Execute(const cc_string* input) {
	static const cc_string prefixSpace = String_FromConst(COMMANDS_PREFIX_SPACE);
	static const cc_string prefix      = String_FromConst(COMMANDS_PREFIX);
	cc_string text;

	struct ChatCommand* cmd;
	int offset, count;
	cc_string name, value;
	cc_string args[50];

	if (String_CaselessStarts(input, &prefixSpace)) { /* /client command args */
		offset = prefixSpace.length;
	} else if (String_CaselessStarts(input, &prefix)) { /* /clientcommand args */
		offset = prefix.length;
	} else if (IsSingleplayerCommandPrefix(input)) { /* /command args */
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

	if (!BlockEditCommand_GetInt(&parts[0], "X coord", &xyz.X, -127, 127)) return false;
	if (!BlockEditCommand_GetInt(&parts[1], "Y coord", &xyz.Y, -127, 127)) return false;
	if (!BlockEditCommand_GetInt(&parts[2], "Z coord", &xyz.Z, -127, 127)) return false;

	coords->X = xyz.X / 16.0f;
	coords->Y = xyz.Y / 16.0f;
	coords->Z = xyz.Z / 16.0f;
	return true;
}

static void BlockEditCommand_Execute(const cc_string* args, int argsCount__) {
	cc_string parts[3];
	cc_string* prop;
	cc_string* value;
	int argsCount, block, v;
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
	Commands_Register(&CuboidCommand);
	Commands_Register(&TeleportCommand);
	Commands_Register(&ClearDeniedCommand);
	Commands_Register(&BlockEditCommand);
}

static void OnFree(void) {
	cmds_head = NULL;
}

struct IGameComponent Commands_Component = {
	OnInit, /* Init  */
	OnFree  /* Free  */
};
