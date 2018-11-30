#ifndef CC_DEFLATE_H
#define CC_DEFLATE_H
#include "String.h"
/* Decodes data compressed using DEFLATE in a streaming manner.
   Partially based off information from
	https://handmade.network/forums/wip/t/2363-implementing_a_basic_png_reader_the_handmade_way
	http://commandlinefanatic.com/cgi-bin/showarticle.cgi?article=art001
	https://www.ietf.org/rfc/rfc1951.txt
	https://github.com/nothings/stb/blob/master/stb_image.h
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/
struct Stream;

struct GZipHeader { uint8_t State; bool Done; uint8_t PartsRead; int32_t Flags; };
void GZipHeader_Init(struct GZipHeader* header);
ReturnCode GZipHeader_Read(struct Stream* s, struct GZipHeader* header);

struct ZLibHeader { uint8_t State; bool Done; };
void ZLibHeader_Init(struct ZLibHeader* header);
ReturnCode ZLibHeader_Read(struct Stream* s, struct ZLibHeader* header);


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
	int16_t Fast[1 << INFLATE_FAST_BITS];      /* Fast lookup table for huffman codes */
	uint16_t FirstCodewords[INFLATE_MAX_BITS]; /* Starting codeword for each bit length */
	uint16_t EndCodewords[INFLATE_MAX_BITS];   /* (Last codeword + 1) for each bit length. 0 is ignored. */
	uint16_t FirstOffsets[INFLATE_MAX_BITS];   /* Base offset into Values for codewords of each bit length. */
	uint16_t Values[INFLATE_MAX_LITS];         /* Values/Symbols list */
};

struct InflateState {
	uint8_t State;
	bool LastBlock;   /* Whether the last DEFLATE block has been encounted in the stream */
	uint32_t Bits;    /* Holds bits across byte boundaries*/
	uint32_t NumBits; /* Number of bits in Bits buffer*/

	uint8_t* NextIn;   /* Pointer within Input buffer to next byte that can be read */
	uint32_t AvailIn;  /* Max number of bytes that can be read from Input buffer */
	uint8_t* Output;   /* Pointer for output data */
	uint32_t AvailOut; /* Max number of bytes that can be written to Output buffer */
	struct Stream* Source;  /* Source for filling Input buffer */

	uint32_t Index;                          /* General purpose index / counter */
	uint32_t WindowIndex;                    /* Current index within window circular buffer */
	uint32_t NumCodeLens, NumLits, NumDists; /* Temp counters */
	uint32_t TmpCodeLens, TmpLit, TmpDist;   /* Temp huffman codes */

	uint8_t Input[INFLATE_MAX_INPUT];       /* Buffer for input to DEFLATE */
	uint8_t Buffer[INFLATE_MAX_LITS_DISTS]; /* General purpose temp array */
	union {
		struct HuffmanTable CodeLens;       /* Values represent codeword lengths of lits/dists codewords */
		struct HuffmanTable Lits;           /* Values represent literal or lengths */
	} Table; /* union to save on memory */
	struct HuffmanTable TableDists;         /* Values represent distances back */
	uint8_t Window[INFLATE_WINDOW_SIZE];    /* Holds circular buffer of recent output data, used for LZ77 */
};

/* Initialises DEFLATE decompressor state to defaults. */
CC_EXPORT void Inflate_Init(struct InflateState* state, struct Stream* source);
/* Attempts to decompress all currently pending data. */
/* NOTE: This is a low level call - usually you should use Inflate_MakeStream. */
void Inflate_Process(struct InflateState* state);
/* Deompresses input data read from another stream using DEFLATE. Read only stream. */
/* NOTE: This only uncompresses pure DEFLATE compressed data. */
/* If data starts with a GZIP or ZLIB header, use GZipHeader_Read or ZLibHeader_Read to first skip it. */
CC_EXPORT void Inflate_MakeStream(struct Stream* stream, struct InflateState* state, struct Stream* underlying);


#define DEFLATE_BUFFER_SIZE 16384
#define DEFLATE_OUT_SIZE 8192
#define DEFLATE_HASH_SIZE 0x1000UL
#define DEFLATE_HASH_MASK 0x0FFFUL
struct DeflateState {
	uint32_t Bits;         /* Holds bits across byte boundaries */
	uint32_t NumBits;      /* Number of bits in Bits buffer */
	uint32_t InputPosition;

	uint8_t* NextOut;    /* Pointer within Output buffer to next byte that can be written */
	uint32_t AvailOut;   /* Max number of bytes that can be written to Output buffer */
	struct Stream* Dest; /* Destination that Output buffer is written to */
	
	uint8_t Input[DEFLATE_BUFFER_SIZE];
	uint8_t Output[DEFLATE_OUT_SIZE];
	uint16_t Head[DEFLATE_HASH_SIZE];
	uint16_t Prev[DEFLATE_BUFFER_SIZE];
	bool WroteHeader;
};
/* Compresses input data using DEFLATE, then writes compressed output to another stream. Write only stream. */
/* DEFLATE compression is pure compressed data, there is no header or footer. */
CC_EXPORT void Deflate_MakeStream(struct Stream* stream, struct DeflateState* state, struct Stream* underlying);

struct GZipState { struct DeflateState Base; uint32_t Crc32, Size; };
/* Compresses input data using GZIP, then writes compressed output to another stream. Write only stream. */
/* GZIP compression is GZIP header, followed by DEFLATE compressed data, followed by GZIP footer. */
CC_EXPORT void GZip_MakeStream(struct Stream* stream, struct GZipState* state, struct Stream* underlying);

struct ZLibState { struct DeflateState Base; uint32_t Adler32; };
/* Compresses input data using ZLIB, then writes compressed output to another stream. Write only stream. */
/* ZLIB compression is ZLIB header, followed by DEFLATE compressed data, followed by ZLIB footer. */
CC_EXPORT void ZLib_MakeStream(struct Stream* stream, struct ZLibState* state, struct Stream* underlying);

/* Minimal data needed to describe an entry in a .zip archive. */
struct ZipEntry { uint32_t CompressedSize, UncompressedSize, LocalHeaderOffset, Crc32; };
#define ZIP_MAX_ENTRIES 2048

/* Stores state for reading and processing entries in a .zip archive. */
struct ZipState {
	/* Source of the .zip archive data. Must be seekable. */
	struct Stream* Input;
	/* Callback function to process the data in a .zip archive entry. */
	/* NOTE: data stream may not be seekable. (entry data might be compressed) */
	void (*ProcessEntry)(const String* path, struct Stream* data, struct ZipEntry* entry);
	/* Predicate used to select which entries in a .zip archive get proessed. */
	/* NOTE: returning false entirely skips the entry. (avoids pointless seek to entry) */
	bool (*SelectEntry)(const String* path);
	/* Number of entries in the .zip archive. */
	int EntriesCount;
	/* Data for each entry in the .zip archive. */
	struct ZipEntry Entries[ZIP_MAX_ENTRIES];
};

/* Initialises .zip archive reader state to defaults. */
CC_EXPORT void Zip_Init(struct ZipState* state, struct Stream* input);
/* Reads and processes the entries in a .zip archive. */
/* NOTE: Must have been initialised with Zip_Init first. */
CC_EXPORT ReturnCode Zip_Extract(struct ZipState* state);
#endif
