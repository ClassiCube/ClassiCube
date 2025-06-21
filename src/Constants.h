#ifndef CC_CONSTANTS_H
#define CC_CONSTANTS_H
/* 
Defines useful constants
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/

#define GAME_MAX_CMDARGS 5
#define GAME_APP_VER "1.3.7"
#define GAME_API_VER 1

#if defined CC_BUILD_WEB
#define GAME_APP_ALT   "ClassiCube 1.3.7 web mobile"
#define GAME_APP_NAME  "ClassiCube 1.3.7 web"
#define GAME_APP_TITLE "ClassiCube"
#else
#define GAME_APP_NAME  "ClassiCube 1.3.7"
#define GAME_APP_TITLE "ClassiCube 1.3.7"
#endif

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

#define GUI_MAX_CHATLINES 30

enum FACE_CONSTS {
	FACE_XMIN = 0, FACE_BIT_XMIN = 1 << FACE_XMIN, /* Face X = 0 */
	FACE_XMAX = 1, FACE_BIT_XMAX = 1 << FACE_XMAX, /* Face X = 1 */
	FACE_ZMIN = 2, FACE_BIT_ZMIN = 1 << FACE_ZMIN, /* Face Z = 0 */
	FACE_ZMAX = 3, FACE_BIT_ZMAX = 1 << FACE_ZMAX, /* Face Z = 1 */
	FACE_YMIN = 4, FACE_BIT_YMIN = 1 << FACE_YMIN, /* Face Y = 0 */
	FACE_YMAX = 5, FACE_BIT_YMAX = 1 << FACE_YMAX, /* Face Y = 1 */
	FACE_COUNT= 6  /* Number of faces on a cube */
};

enum SKIN_TYPE { SKIN_64x32, SKIN_64x64, SKIN_64x64_SLIM, SKIN_INVALID = 0xF0 };
#define DRAWER2D_MAX_COLORS 256

#define UInt8_MaxValue  ((cc_uint8)255)
#define Int16_MaxValue  ((cc_int16)32767)
#define UInt16_MaxValue ((cc_uint16)65535)
#define Int32_MinValue  ((cc_int32)-2147483647L - (cc_int32)1L)
#define Int32_MaxValue  ((cc_int32)2147483647L)

#define SKINS_SERVER    "http://cdn.classicube.net/skin"
#define UPDATES_SERVER  "http://cdn.classicube.net/client"
#define SERVICES_SERVER "https://www.classicube.net/api"
#define RESOURCE_SERVER "http://static.classicube.net"
/* Webpage where users can register for a new account */
#define REGISTERNEW_URL "https://www.classicube.net/acc/register/"

#define DEFAULT_USERNAME "Singleplayer"
#endif
