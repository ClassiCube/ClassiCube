#ifndef CC_MAPFORMATS_H
#define CC_MAPFORMATS_H
#include "Core.h"
/* Imports/exports a world and associated metadata from/to a particular map file format.
   Copyright 2014-2022 ClassiCube | Licensed under BSD-3
*/

struct Stream;
/* Imports a world encoded in a particular map file format. */
typedef cc_result (*IMapImporter)(struct Stream* stream);
/* Attempts to find a suitable importer based on filename. */
/* Returns NULL if no match found. */
CC_API IMapImporter Map_FindImporter(const cc_string* path);
/* Attempts to import the map from the given file. */
/* NOTE: Uses Map_FindImporter to import based on filename. */
CC_API cc_result Map_LoadFrom(const cc_string* path);

/* Imports a world from a .lvl MCSharp server map file. */
/* Used by MCSharp/MCLawl/MCForge/MCDzienny/MCGalaxy. */
cc_result Lvl_Load(struct Stream* stream);
/* Imports a world from a .fcm fCraft server map file. (v3 only) */
/* Used by fCraft/800Craft/LegendCraft/ProCraft. */
cc_result Fcm_Load(struct Stream* stream);
/* Imports a world from a .cw ClassicWorld map file. */
/* Used by ClassiCube/ClassicalSharp. */
cc_result Cw_Load(struct Stream* stream);
/* Imports a world from a .dat classic map file. */
/* Used by Minecraft Classic/WoM client. */
cc_result Dat_Load(struct Stream* stream);

/* Exports a world to a .cw ClassicWorld map file. */
/* Compatible with ClassiCube/ClassicalSharp */
cc_result Cw_Save(struct Stream* stream);
/* Exports a world to a .schematic Schematic map file */
/* Used by MCEdit and other tools */
cc_result Schematic_Save(struct Stream* stream);
/* Exports a world to a .dat Classic map file */
/* Used by MineCraft Classic */
cc_result Dat_Save(struct Stream* stream);
#endif
