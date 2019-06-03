#ifndef CC_VORBIS_H
#define CC_VORBIS_H
#include "Core.h"
/* Decodes ogg vorbis audio
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
struct Stream;
#define VORBIS_MAX_CHANS 8
#define VORBIS_MAX_BLOCK_SIZE 8192
#define OGG_BUFFER_SIZE (255 * 256)

void Ogg_MakeStream(struct Stream* stream, uint8_t* buffer, struct Stream* source);
struct Codebook; struct Floor; struct Residue; struct Mapping; struct Mode;

struct imdct_state {
	int n, log2_n;
	float A[VORBIS_MAX_BLOCK_SIZE / 2];
	float B[VORBIS_MAX_BLOCK_SIZE / 2];
	float C[VORBIS_MAX_BLOCK_SIZE / 4];
	uint32_t Reversed[VORBIS_MAX_BLOCK_SIZE / 8];
};

struct VorbisWindow { float* Prev; float* Cur; };
struct VorbisState {
	uint32_t Bits;    /* Holds bits across byte boundaries*/
	uint32_t NumBits; /* Number of bits in Bits buffer*/
	struct Stream* Source;  /* Source for filling Input buffer */

	uint8_t Channels, ModeNumBits;
	uint16_t CurBlockSize, PrevBlockSize, DataSize, NumCodebooks;
	int SampleRate; int BlockSizes[2];
	float* Temp; /* temp array reused in places */
	float* Values[2]; /* swapped each frame */
	float* PrevOutput[VORBIS_MAX_CHANS];
	float* CurOutput[VORBIS_MAX_CHANS];

	struct Codebook* Codebooks;
	struct Floor* Floors;
	struct Residue* Residues;
	struct Mapping* Mappings;
	struct Mode* Modes;

	float* WindowRaw;
	struct VorbisWindow Windows[2];
	struct imdct_state imdct[2];
};

void Vorbis_Free(struct VorbisState* ctx);
ReturnCode Vorbis_DecodeHeaders(struct VorbisState* ctx);
ReturnCode Vorbis_DecodeFrame(struct VorbisState* ctx);
int Vorbis_OutputFrame(struct VorbisState* ctx, int16_t* data);
#endif
