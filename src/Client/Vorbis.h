#ifndef CC_VORBIS_H
#define CC_VORBIS_H
#include "Typedefs.h"
/* Decodes ogg vorbis audio
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;
struct Stream;

enum OGG_VER { OGG_ERR_INVALID_SIG = 2552401, OGG_ERR_VERSION };
#define OGG_BUFFER_SIZE (255 * 256)
void Ogg_MakeStream(struct Stream* stream, UInt8* buffer, struct Stream* source);

enum VORBIS_ERR {
	VORBIS_ERR_HEADER = 5238001, VORBIS_ERR_WRONG_HEADER, VORBIS_ERR_FRAMING,
	VORBIS_ERR_VERSION, VORBIS_ERR_BLOCKSIZE, VORBIS_ERR_CHANS,
	VORBIS_ERR_TIME_TYPE, VORBIS_ERR_FLOOR_TYPE, VORBIS_ERR_RESIDUE_TYPE,
	VORBIS_ERR_MAPPING_TYPE, VORBIS_ERR_MODE_TYPE,
	VORBIS_ERR_CODEBOOK_SYNC, VORBIS_ERR_CODEBOOK_ENTRY, VORBIS_ERR_CODEBOOK_LOOKUP,
	VORBIS_ERR_MODE_WINDOW, VORBIS_ERR_MODE_TRANSFORM,
	VORBIS_ERR_MAPPING_CHANS, VORBIS_ERR_MAPPING_RESERVED,
	VORBIS_ERR_FRAME_TYPE,
};

#define VORBIS_MAX_CHANS 8
struct Codebook; struct Floor; struct Residue; struct Mapping; struct Mode;
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
};
void Vorbis_Init(struct VorbisState* ctx, struct Stream* source);
ReturnCode Vorbis_DecodeHeaders(struct VorbisState* ctx);
ReturnCode Vorbis_DecodeFrame(struct VorbisState* ctx);
Int32 Vorbis_OutputFrame(struct VorbisState* ctx, Int16* data);
#endif
