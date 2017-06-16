#ifndef CS_CONSTANTS_H
#define CS_CONSTANTS_H
/* Defines useful constants.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define USE16_BIT FALSE

/* Max number of characters strings can have. */
#define STRING_SIZE 64

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
#endif