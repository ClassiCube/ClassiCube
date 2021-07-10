#ifndef CC_CONSTANTS_H
#define CC_CONSTANTS_H
/* Defines useful constants.
   Copyright 2014-2021 ClassiCube | Licensed under BSD-3
*/

#define GAME_MAX_CMDARGS 5
#define GAME_APP_VER "1.2.6"
#define GAME_API_VER 1

#if defined CC_BUILD_WEB
#define GAME_APP_ALT   "ClassiCube 1.2.6 web mobile"
#define GAME_APP_NAME  "ClassiCube 1.2.6 web"
#define GAME_APP_TITLE "ClassiCube"
#elif defined CC_BUILD_COCOA
#define GAME_APP_NAME  "ClassiCube 1.2.6 alpha"
#define GAME_APP_TITLE "ClassiCube 1.2.6 alpha"
#elif defined CC_BUILD_ANDROID
#define GAME_APP_NAME  "ClassiCube 1.2.6 android alpha"
#define GAME_APP_TITLE "ClassiCube 1.2.6 android alpha"
#else
#define GAME_APP_NAME  "ClassiCube 1.2.6"
#define GAME_APP_TITLE "ClassiCube 1.2.6"
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
#define DRAWER2D_MAX_COLS 256

#define UInt8_MaxValue  ((cc_uint8)255)
#define Int16_MaxValue  ((cc_int16)32767)
#define UInt16_MaxValue ((cc_uint16)65535)
#define Int32_MinValue  ((cc_int32)-2147483647L - (cc_int32)1L)
#define Int32_MaxValue  ((cc_int32)2147483647L)

/* Skins were moved to use Amazon S3, so link directly to avoid a pointless redirect */
#define SKINS_SERVER    "http://classicube.s3.amazonaws.com/skin"
#define UPDATES_SERVER  "http://cs.classicube.net/client"
#define SERVICES_SERVER "https://www.classicube.net/api"
#define RESOURCE_SERVER "http://static.classicube.net"
#define REGISTERNEW_URL "https://www.classicube.net/acc/register/"
#endif
