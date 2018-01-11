#ifndef CC_CONSTANTS_H
#define CC_CONSTANTS_H
/* Defines useful constants.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define PROGRAM_APP_NAME "ClassicalSharp 0.99.9.2"

#define USE16_BIT FALSE

/* Max number of characters strings can have. */
#define STRING_SIZE 64
/* Max number of characters filenames can have. */
#define FILENAME_SIZE 260

/* Chunk axis length, in blocks. */
#define CHUNK_SIZE 16
#define HALF_CHUNK_SIZE 8
#define CHUNK_SIZE_2 (CHUNK_SIZE * CHUNK_SIZE)
#define CHUNK_SIZE_3 (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)

/* Maximum index in /mask for a chunk. */
#define CHUNK_MAX 15
#define CHUNK_SHIFT 4

/* Chunk axis length (and neighbours), in blocks. */
#define EXTCHUNK_SIZE 18
#define EXTCHUNK_SIZE_2 (EXTCHUNK_SIZE * EXTCHUNK_SIZE)
#define EXTCHUNK_SIZE_3 (EXTCHUNK_SIZE * EXTCHUNK_SIZE * EXTCHUNK_SIZE)

/* Minor adjustment to max UV coords, to avoid pixel bleeding errors due to rounding. */
#define UV2_Scale (15.99f / 16.0f)

/* Face X = 0. */
#define FACE_XMIN 0
/* Face X = 1. */
#define FACE_XMAX 1
/* Face Z = 0. */
#define FACE_ZMIN 2
/* Face Z = 1. */
#define FACE_ZMAX 3
/* Face Y = 0. */
#define FACE_YMIN 4
/* Face Y = 1. */
#define FACE_YMAX 5
/* Number of faces on a cube. */
#define FACE_COUNT 6

#define SKIN_TYPE_64x32 0
#define SKIN_TYPE_64x64 1
#define SKIN_TYPE_64x64_SLIM 2
#define SKIN_TYPE_INVALID 3

#define FONT_STYLE_NORMAL    0
#define FONT_STYLE_BOLD      1
#define FONT_STYLE_UNDERLINE 2

#define DRAWER2D_MAX_COLS 256

#define WEATHER_SUNNY 0
#define WEATHER_RAINY 1
#define WEATHER_SNOWY 2

#define ENV_VAR_EDGE_BLOCK       0
#define ENV_VAR_SIDES_BLOCK      1
#define ENV_VAR_EDGE_HEIGHT      2
#define ENV_VAR_SIDES_OFFSET     3
#define ENV_VAR_CLOUDS_HEIGHT    4
#define ENV_VAR_CLOUDS_SPEED     5
#define ENV_VAR_WEATHER_SPEED    6
#define ENV_VAR_WEATHER_FADE     7
#define ENV_VAR_WEATHER          8
#define ENV_VAR_EXP_FOG          9
#define ENV_VAR_SKYBOX_HOR_SPEED 10
#define ENV_VAR_SKYBOX_VER_SPEED 11
#define ENV_VAR_SKY_COL          12
#define ENV_VAR_CLOUDS_COL       13
#define ENV_VAR_FOG_COL          14
#define ENV_VAR_SUN_COL          15
#define ENV_VAR_SHADOW_COL       16
#endif