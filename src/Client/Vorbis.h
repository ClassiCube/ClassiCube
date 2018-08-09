#ifndef CC_VORBIS_H
#define CC_VORBIS_H
#include "Typedefs.h"
/* Decodes ogg vorbis audio
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;
struct Stream;
#define VORBIS_MAX_CHANS 8
#define VORBIS_MAX_BLOCK_SIZE 8192
#define OGG_BUFFER_SIZE (255 * 256)

void Ogg_MakeStream(struct Stream* stream, UInt8* buffer, struct Stream* source);
struct Codebook; struct Floor; struct Residue; struct Mapping; struct Mode;

struct imdct_state {
	Int32 n, log2_n;
	Real32 A[VORBIS_MAX_BLOCK_SIZE / 2];
	Real32 B[VORBIS_MAX_BLOCK_SIZE / 2];
	Real32 C[VORBIS_MAX_BLOCK_SIZE / 4];
	UInt32 Reversed[VORBIS_MAX_BLOCK_SIZE / 8];
};

struct VorbisState {
	UInt32 Bits;    /* Holds bits across byte boundaries*/
	UInt32 NumBits; /* Number of bits in Bits buffer*/
	struct Stream* Source;  /* Source for filling Input buffer */

	UInt8 Channels, ModeNumBits; 
	UInt16 CurBlockSize, PrevBlockSize, DataSize;
	Int32 SampleRate; Int32 BlockSizes[2];
	Real32* Values;
	Real32* PrevOutput[VORBIS_MAX_CHANS];
	Real32* CurOutput[VORBIS_MAX_CHANS];

	struct Codebook* Codebooks;
	struct Floor* Floors;
	struct Residue* Residues;
	struct Mapping* Mappings;
	struct Mode* Modes;

	Real32* WindowShort;
	Real32* WindowLong[2][2];
	struct imdct_state imdct[2];
};

void Vorbis_Free(struct VorbisState* ctx);
ReturnCode Vorbis_DecodeHeaders(struct VorbisState* ctx);
ReturnCode Vorbis_DecodeFrame(struct VorbisState* ctx);
Int32 Vorbis_OutputFrame(struct VorbisState* ctx, Int16* data);
#endif
