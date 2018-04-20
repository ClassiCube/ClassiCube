#ifndef CC_MAPFORMATS_H
#define CC_MAPFORMATS_H
#include "Stream.h"
/* Imports/exports a world and associated metadata from/to a particular map file format.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

/* Imports a world from a LVL map file (MCLawl server map) */
void Lvl_Load(Stream* stream);
/* Imports a world from a FCMv3 map file (fCraft server map)
   Part of fCraft | Copyright (c) 2009-2014 Matvei Stefarov <me@matvei.org> | BSD-3 | See LICENSE.txt */
void Fcm_Load(Stream* stream);
void Cw_Save(Stream* stream);
void Cw_Load(Stream* stream);
void Dat_Load(Stream* stream);
void Schematic_Save(Stream* stream);
#endif