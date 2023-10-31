#ifndef CC_DEFLATE_H
#define CC_DEFLATE_H
#include "Core.h"
/* Decodes data compressed using DEFLATE in a streaming manner.
   Partially based off information from
	https://handmade.network/forums/wip/t/2363-implementing_a_basic_png_reader_the_handmade_way
	http://commandlinefanatic.com/cgi-bin/showarticle.cgi?article=art001
	https://www.ietf.org/rfc/rfc1951.txt
	https://github.com/nothings/stb/blob/master/stb_image.h
	https://www.hanshq.net/zip.html
   Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/
struct Stream;

struct GZipHeader { cc_uint8 state; cc_bool done; cc_uint8 partsRead; int flags; };
void GZipHeader_Init(struct GZipHeader* header);
cc_result GZipHeader_Read(struct Stream* s, struct GZipHeader* header);

struct ZLibHeader { cc_uint8 state; cc_bool done; };
void ZLibHeader_Init(struct ZLibHeader* header);
cc_result ZLibHeader_Read(struct Stream* s, struct ZLibHeader* header);


#define INFLATE_MAX_INPUT 8192
#define INFLATE_MAX_CODELENS 19
#define INFLATE_MAX_LITS 288
#define INFLATE_MAX_DISTS 32
#define INFLATE_MAX_LITS_DISTS (INFLATE_MAX_LITS + INFLATE_MAX_DISTS)
#define INFLATE_MAX_BITS 16
#define INFLATE_FAST_BITS 9
#define INFLATE_WINDOW_SIZE 0x8000UL
#define INFLATE_WINDOW_MASK 0x7FFFUL

struct HuffmanTable {
	cc_int16 Fast[1 << INFLATE_FAST_BITS];      /* Fast lookup table for huffman codes */
	cc_uint16 FirstCodewords[INFLATE_MAX_BITS]; /* Starting codeword for each bit length */
	cc_uint16 EndCodewords[INFLATE_MAX_BITS];   /* (Last codeword + 1) for each bit length. 0 is ignored. */
	cc_uint16 FirstOffsets[INFLATE_MAX_BITS];   /* Base offset into Values for codewords of each bit length. */
	cc_uint16 Values[INFLATE_MAX_LITS];         /* Values/Symbols list */
};

struct InflateState {
	cc_uint8 State;
	cc_bool LastBlock; /* Whether the last DEFLATE block has been encounted in the stream */
	cc_uint32 Bits;    /* Holds bits across byte boundaries */
	cc_uint32 NumBits; /* Number of bits in Bits buffer */

	cc_uint8* NextIn;   /* Pointer within Input buffer to next byte that can be read */
	cc_uint32 AvailIn;  /* Max number of bytes that can be read from Input buffer */
	cc_uint8* Output;   /* Pointer for output data */
	cc_uint32 AvailOut; /* Max number of bytes that can be written to Output buffer */
	struct Stream* Source;  /* Source for filling Input buffer */

	cc_uint32 Index;                          /* General purpose index / counter */
	cc_uint32 WindowIndex;                    /* Current index within window circular buffer */
	cc_uint32 NumCodeLens, NumLits, NumDists; /* Temp counters */
	cc_uint32 TmpCodeLens, TmpLit, TmpDist;   /* Temp huffman codes */

	cc_uint8 Input[INFLATE_MAX_INPUT];       /* Buffer for input to DEFLATE */
	cc_uint8 Buffer[INFLATE_MAX_LITS_DISTS]; /* General purpose temp array */
	union {
		struct HuffmanTable CodeLens;       /* Values represent codeword lengths of lits/dists codewords */
		struct HuffmanTable Lits;           /* Values represent literal or lengths */
	} Table; /* union to save on memory */
	struct HuffmanTable TableDists;         /* Values represent distances back */
	cc_uint8 Window[INFLATE_WINDOW_SIZE];    /* Holds circular buffer of recent output data, used for LZ77 */
	cc_result result;
};

/* Initialises DEFLATE decompressor state to defaults. */
CC_API void Inflate_Init2(struct InflateState* state, struct Stream* source);
/* Attempts to decompress as much of the currently pending data as possible. */
/* NOTE: This is a low level call - usually you treat as a stream via Inflate_MakeStream. */
void Inflate_Process(struct InflateState* s);
/* Deompresses input data read from another stream using DEFLATE. Read only stream. */
/* NOTE: This only uncompresses pure DEFLATE compressed data. */
/* If data starts with a GZIP or ZLIB header, use GZipHeader_Read or ZLibHeader_Read to first skip it. */
CC_API void Inflate_MakeStream2(struct Stream* stream, struct InflateState* state, struct Stream* underlying);


#define DEFLATE_BLOCK_SIZE  16384
#define DEFLATE_BUFFER_SIZE 32768
#define DEFLATE_OUT_SIZE 8192
#define DEFLATE_HASH_SIZE 0x1000UL
#define DEFLATE_HASH_MASK 0x0FFFUL
struct DeflateState {
	cc_uint32 Bits;         /* Holds bits across byte boundaries */
	cc_uint32 NumBits;      /* Number of bits in Bits buffer */
	cc_uint32 InputPosition;

	cc_uint8* NextOut;    /* Pointer within Output buffer to next byte that can be written */
	cc_uint32 AvailOut;   /* Max number of bytes that can be written to Output buffer */
	struct Stream* Dest; /* Destination that Output buffer is written to */

	cc_uint16 LitsCodewords[INFLATE_MAX_LITS]; /* Codewords for each value */
	cc_uint8 LitsLens[INFLATE_MAX_LITS];       /* Bit lengths of each codeword */
	
	cc_uint8 Input[DEFLATE_BUFFER_SIZE];
	cc_uint8 Output[DEFLATE_OUT_SIZE];
	cc_uint16 Head[DEFLATE_HASH_SIZE];
	cc_uint16 Prev[DEFLATE_BUFFER_SIZE];
	/* NOTE: The largest possible value that can get */
	/*  stored in Head/Prev is <= DEFLATE_BUFFER_SIZE */
	cc_bool WroteHeader;
};
/* Compresses input data using DEFLATE, then writes compressed output to another stream. Write only stream. */
/* DEFLATE compression is pure compressed data, there is no header or footer. */
CC_API void Deflate_MakeStream(struct Stream* stream, struct DeflateState* state, struct Stream* underlying);

struct GZipState { struct DeflateState Base; cc_uint32 Crc32, Size; };
/* Compresses input data using GZIP, then writes compressed output to another stream. Write only stream. */
/* GZIP compression is GZIP header, followed by DEFLATE compressed data, followed by GZIP footer. */
CC_API  void GZip_MakeStream(      struct Stream* stream, struct GZipState* state, struct Stream* underlying);
typedef void (*FP_GZip_MakeStream)(struct Stream* stream, struct GZipState* state, struct Stream* underlying);

struct ZLibState { struct DeflateState Base; cc_uint32 Adler32; };
/* Compresses input data using ZLIB, then writes compressed output to another stream. Write only stream. */
/* ZLIB compression is ZLIB header, followed by DEFLATE compressed data, followed by ZLIB footer. */
CC_API  void ZLib_MakeStream(      struct Stream* stream, struct ZLibState* state, struct Stream* underlying);
typedef void (*FP_ZLib_MakeStream)(struct Stream* stream, struct ZLibState* state, struct Stream* underlying);

/* Minimal data needed to describe an entry in a .zip archive */
struct ZipEntry { cc_uint32 CompressedSize, UncompressedSize, LocalHeaderOffset, CRC32; };
/* Callback function to process the data in a .zip archive entry */
/* Return non-zero to indicate an error and stop further processing */
/* NOTE: data stream MAY NOT be seekable (i.e. entry data might be compressed) */
typedef cc_result (*Zip_ProcessEntry)(const cc_string* path, struct Stream* data, struct ZipEntry* entry);
/* Predicate used to select which entries in a .zip archive get processed */
/* NOTE: returning false entirely skips the entry (avoids pointless seek to entry) */
typedef cc_bool (*Zip_SelectEntry)(const cc_string* path);

CC_API cc_result Zip_Extract(struct Stream* source, Zip_SelectEntry selector, Zip_ProcessEntry processor);
#endif
