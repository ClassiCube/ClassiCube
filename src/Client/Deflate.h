#ifndef CC_DEFLATE_H
#define CC_DEFLATE_H
#include "Typedefs.h"
/* Decodes data compressed using DEFLATE in a streaming manner.
   Partially based off information from
	https://handmade.network/forums/wip/t/2363-implementing_a_basic_png_reader_the_handmade_way
	http://commandlinefanatic.com/cgi-bin/showarticle.cgi?article=art001
	https://www.ietf.org/rfc/rfc1951.txt
	https://github.com/nothings/stb/blob/master/stb_image.h
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/
typedef struct Stream_ Stream;

typedef struct GZipHeader_ {
	UInt8 State; bool Done; UInt8 PartsRead; Int32 Flags;
} GZipHeader;
void GZipHeader_Init(GZipHeader* header);
void GZipHeader_Read(Stream* s, GZipHeader* header);

typedef struct ZLibHeader_ {
	UInt8 State; bool Done; Int32 LZ77WindowSize;
} ZLibHeader;
void ZLibHeader_Init(ZLibHeader* header);
void ZLibHeader_Read(Stream* s, ZLibHeader* header);


#define DEFLATE_MAX_INPUT 8192
#define DEFLATE_MAX_CODELENS 19
#define DEFLATE_MAX_LITS 288
#define DEFLATE_MAX_DISTS 32
#define DEFLATE_MAX_LITS_DISTS (DEFLATE_MAX_LITS + DEFLATE_MAX_DISTS)
#define DEFLATE_MAX_BITS 16
#define DEFLATE_ZFAST_BITS 9
#define DEFLATE_WINDOW_SIZE 0x8000UL
#define DEFLATE_WINDOW_MASK 0x7FFFUL

typedef struct HuffmanTable_ {
	UInt16 FirstCodewords[DEFLATE_MAX_BITS]; /* Starting codeword for each bit length */
	UInt16 EndCodewords[DEFLATE_MAX_BITS];    /* (Last codeword + 1) for each bit length. 0 is ignored. */
	UInt16 FirstOffsets[DEFLATE_MAX_BITS];   /* Base offset into Values for codewords of each bit length. */
	UInt16 Values[DEFLATE_MAX_LITS];         /* Values/Symbols list */
	Int16 Fast[1 << DEFLATE_ZFAST_BITS];     /* Fast lookup table for huffman codes */
} HuffmanTable;

typedef struct DeflateState_ {
	UInt8 State;
	Stream* Source;
	bool LastBlock; /* Whether the last DEFLATE block has been encounted in the stream */
	UInt32 Bits;    /* Holds bits across byte boundaries*/
	UInt32 NumBits; /* Number of bits in Bits buffer*/
	
	UInt32 NextIn;   /* Index within Input of byte being read */
	UInt32 AvailIn;  /* Number of bytes that can be read from Input */
	UInt8* Output;   /* Pointer for output data */
	UInt32 AvailOut; /* Max number of bytes to output */

	UInt32 Index;                          /* General purpose index / counter */
	UInt32 WindowIndex;                    /* Current index within window circular buffer */
	UInt32 NumCodeLens, NumLits, NumDists; /* Temp counters */
	UInt32 TmpCodeLens, TmpLit, TmpDist;   /* Temp huffman codes */

	UInt8 Input[DEFLATE_MAX_INPUT];       /* Buffer for input to DEFLATE */
	UInt8 Buffer[DEFLATE_MAX_LITS_DISTS]; /* General purpose array */
	HuffmanTable CodeLensTable;           /* Values represent codeword lengths of lits/dists codewords */
	HuffmanTable LitsTable;               /* Values represent literal or lengths */
	HuffmanTable DistsTable;              /* Values represent distances back */
	UInt8 Window[DEFLATE_WINDOW_SIZE];    /* Holds circular buffer of recent output data, used for LZ77 */
} DeflateState;

void Deflate_Init(DeflateState* state, Stream* source);
void Deflate_Process(DeflateState* state);
void Deflate_MakeStream(Stream* stream, DeflateState* state, Stream* underlying);
#endif