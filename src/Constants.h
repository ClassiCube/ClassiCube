#ifndef CC_CONSTANTS_H
#define CC_CONSTANTS_H
/* Defines useful constants.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define PROGRAM_MAX_CMDARGS 5
#define PROGRAM_APP_NAME "ClassiCube 0.99.9.2"

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

enum SKIN_TYPE { SKIN_64x32, SKIN_64x64, SKIN_64x64_SLIM, SKIN_INVALID };
enum FONT_STYLE { FONT_STYLE_NORMAL, FONT_STYLE_BOLD, FONT_STYLE_UNDERLINE };
#define DRAWER2D_MAX_COLS 256

#define UInt8_MaxValue  ((UInt8)255)
#define Int16_MinValue  ((Int16)-32768)
#define Int16_MaxValue  ((Int16)32767)
#define UInt16_MaxValue ((UInt16)65535)
#define Int32_MinValue  ((Int32)-2147483647L - (Int32)1L)
#define Int32_MaxValue  ((Int32)2147483647L)
#endif
