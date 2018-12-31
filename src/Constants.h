#ifndef CC_CONSTANTS_H
#define CC_CONSTANTS_H
/* Defines useful constants.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define GAME_MAX_CMDARGS 5
#define GAME_APP_NAME "ClassiCube 0.99.9.2"
#define GAME_APP_VER  "0.99.9.2"
#define GAME_API_VER 1

/* Max number of characters strings can have. */
#define STRING_SIZE 64
/* Max number of characters filenames can have. */
#define FILENAME_SIZE 260

/* Chunk axis length in blocks. */
#define CHUNK_SIZE 16
#define HALF_CHUNK_SIZE 8
#define CHUNK_SIZE_2 (CHUNK_SIZE * CHUNK_SIZE)
#define CHUNK_SIZE_3 (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)

#define CHUNK_MAX 15
/* Local index in a chunk for a coordinate. */
#define CHUNK_MASK 15
/* Chunk index for a coordinate. */
#define CHUNK_SHIFT 4

/* Chunk axis length (plus neighbours) in blocks. */
#define EXTCHUNK_SIZE 18
#define EXTCHUNK_SIZE_2 (EXTCHUNK_SIZE * EXTCHUNK_SIZE)
#define EXTCHUNK_SIZE_3 (EXTCHUNK_SIZE * EXTCHUNK_SIZE * EXTCHUNK_SIZE)

/* Minor adjustment to max UV coords, to avoid pixel bleeding errors due to rounding. */
#define UV2_Scale (15.99f / 16.0f)

#define GAME_DEF_TICKS (1.0 / 20)
#define GAME_NET_TICKS (1.0 / 60)

enum FACE_CONSTS {
	FACE_XMIN = 0, /* Face X = 0 */
	FACE_XMAX = 1, /* Face X = 1 */
	FACE_ZMIN = 2, /* Face Z = 0 */
	FACE_ZMAX = 3, /* Face Z = 1 */
	FACE_YMIN = 4, /* Face Y = 0 */
	FACE_YMAX = 5, /* Face Y = 1 */
	FACE_COUNT= 6  /* Number of faces on a cube */
};

enum SKIN_TYPE { SKIN_64x32, SKIN_64x64, SKIN_64x64_SLIM, SKIN_INVALID = 0xF0 };
enum FONT_STYLE { FONT_STYLE_NORMAL, FONT_STYLE_BOLD, FONT_STYLE_ITALIC, FONT_FLAG_UNDERLINE = 0x80 };
#define DRAWER2D_MAX_COLS 256

#define UInt8_MaxValue  ((uint8_t)255)
#define Int16_MinValue  ((int16_t)-32768)
#define Int16_MaxValue  ((int16_t)32767)
#define UInt16_MaxValue ((uint16_t)65535)
#define Int32_MinValue  ((int32_t)-2147483647L - (int32_t)1L)
#define Int32_MaxValue  ((int32_t)2147483647L)
#endif
