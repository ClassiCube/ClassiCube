#ifndef CC_MAPFORMATS_H
#define CC_MAPFORMATS_H
#include "Core.h"
/* Imports/exports a world and associated metadata from/to a particular map file format.
   Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/

struct Stream; 
struct IGameComponent;
extern struct IGameComponent Formats_Component;

/* Imports a world encoded in a particular map file format */
typedef cc_result (*MapImportFunc)(struct Stream* stream);
struct MapImporter;
/* Reads/Loads world data (and potentially metadata) encoded in a particular format */
struct MapImporter {
	const char* fileExt;      /* File extension of the map format */
	MapImportFunc import;     /* Function that imports the encoded data */
	struct MapImporter* next; /* Next importer in linked-list of map importers */
};

/* Adds the given importer to the list of map importers */
CC_API void MapImporter_Register(struct MapImporter* imp);
/* Attempts to find a suitable map importer based on filename */
/* Returns NULL if no match found */
CC_API struct MapImporter* MapImporter_Find(const cc_string* path);
/* Attempts to import a map from the given file */
CC_API cc_result Map_LoadFrom(const cc_string* path);

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
