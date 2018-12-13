#ifndef CC_MAPFORMATS_H
#define CC_MAPFORMATS_H
#include "String.h"
/* Imports/exports a world and associated metadata from/to a particular map file format.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

struct Stream;
/* Imports a world encoded in a particular map file format. */
typedef ReturnCode (*IMapImporter)(struct Stream* stream);
/* Attempts to find the suitable importer based on filename. */
/* Returns NULL if no match found. */
CC_API IMapImporter Map_FindImporter(const String* path);
/* Attempts to import the map from the given file. */
/* NOTE: Uses Map_FindImporter to import based on filename. */
CC_API void Map_LoadFrom(const String* path);

/* Imports a world from a .lvl MCSharp server map file. */
/* Used by MCSharp/MCLawl/MCForge/MCDzienny/MCGalaxy. */
ReturnCode Lvl_Load(struct Stream* stream);
/* Imports a world from a .fcm fCraft server map file. (v3 only) */
/* Used by fCraft/800Craft/LegendCraft/ProCraft. */
ReturnCode Fcm_Load(struct Stream* stream);
/* Imports a world from a .cw ClassicWorld map file. */
/* Used by ClassiCube/ClassicalSharp. */
ReturnCode Cw_Load(struct Stream* stream);
/* Imports a world from a .dat classic map file. */
/* Used by Minecraft Classic/WoM client. */
ReturnCode Dat_Load(struct Stream* stream);

/* Exports a world to a .cw ClassicWorld map file. */
/* Compatible with ClassiCube/ClassicalSharp. */
ReturnCode Cw_Save(struct Stream* stream);
/* Exports a world to a .schematic Schematic map file. */
/* Used by MCEdit and other tools. */
ReturnCode Schematic_Save(struct Stream* stream);
#endif
