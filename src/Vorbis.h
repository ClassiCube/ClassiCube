#ifndef CC_VORBIS_H
#define CC_VORBIS_H
#include "Core.h"
/* Decodes ogg vorbis audio
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct Stream;
#define VORBIS_MAX_CHANS 8
#define VORBIS_MAX_BLOCK_SIZE 8192
#define OGG_BUFFER_SIZE (255 * 256)

/* Wraps an OGG container stream around an existing stream. */
/* NOTE: buffer must be of size OGG_BUFFER_SIZE at least. */
void Ogg_MakeStream(struct Stream* stream, uint8_t* buffer, struct Stream* source);
struct Codebook; struct Floor; struct Residue; struct Mapping; struct Mode;

struct imdct_state {
	int n, log2_n;
	float a[VORBIS_MAX_BLOCK_SIZE / 2];
	float b[VORBIS_MAX_BLOCK_SIZE / 2];
	float c[VORBIS_MAX_BLOCK_SIZE / 4];
	uint32_t reversed[VORBIS_MAX_BLOCK_SIZE / 8];
};

struct VorbisWindow { float* Prev; float* Cur; };
struct VorbisState {
	uint32_t Bits;    /* Holds bits across byte boundaries*/
	uint32_t NumBits; /* Number of bits in Bits buffer*/
	struct Stream* source;  /* Source for filling Input buffer */

	uint8_t channels, modeNumBits;
	uint16_t curBlockSize, prevBlockSize, dataSize, numCodebooks;
	int sampleRate; int blockSizes[2];
	float* temp; /* temp array reused in places */
	float* values[2]; /* swapped each frame */
	float* prevOutput[VORBIS_MAX_CHANS];
	float* curOutput[VORBIS_MAX_CHANS];

	struct Codebook* codebooks;
	struct Floor* floors;
	struct Residue* residues;
	struct Mapping* mappings;
	struct Mode* modes;

	float* windowRaw;
	struct VorbisWindow windows[2];
	struct imdct_state imdct[2];
};

/* Frees all dynamic memory allocated to decode the given vorbis audio. */
void Vorbis_Free(struct VorbisState* ctx);
/* Reads and decodes the initial vorbis headers and setup data. */
ReturnCode Vorbis_DecodeHeaders(struct VorbisState* ctx);
/* Reads and decodes the current frame's audio data. */
ReturnCode Vorbis_DecodeFrame(struct VorbisState* ctx);
/* Produces final interleaved audio samples for the current frame. */
int Vorbis_OutputFrame(struct VorbisState* ctx, int16_t* data);
#endif
