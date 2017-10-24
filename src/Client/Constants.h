#ifndef CC_CONSTANTS_H
#define CC_CONSTANTS_H
/* Defines useful constants.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define Program_AppName "ClassicalSharp 0.99.9.2"

#define USE16_BIT FALSE

/* Max number of characters strings can have. */
#define STRING_SIZE 64
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
#define Face_XMin 0
/* Face X = 1. */
#define Face_XMax 1
/* Face Z = 0. */
#define Face_ZMin 2
/* Face Z = 1. */
#define Face_ZMax 3
/* Face Y = 0. */
#define Face_YMin 4
/* Face Y = 1. */
#define Face_YMax 5
/* Number of faces on a cube. */
#define Face_Count 6

#define SkinType_64x32 0
#define SkinType_64x64 1
#define SkinType_64x64Slim 2
#define SkinType_Invalid 3

#define FONT_STYLE_NORMAL    0
#define FONT_STYLE_BOLD      1
#define FONT_STYLE_UNDERLINE 2

#define DRAWER2D_MAX_COLS 256
#endif