#ifndef CC_AUDIO_H
#define CC_AUDIO_H
#include "Typedefs.h"
/* Manages playing sound and music.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;
struct Stream;

void Audio_MakeComponent(struct IGameComponent* comp);
void Audio_SetMusic(Int32 volume);
void Audio_SetSounds(Int32 volume);
void Audio_PlayDigSound(UInt8 type);
void Audio_PlayStepSound(UInt8 type);

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
};

struct Codebook; struct Floor; struct Residue; struct Mapping; struct Mode;
struct VorbisState {
	UInt32 Bits;    /* Holds bits across byte boundaries*/
	UInt32 NumBits; /* Number of bits in Bits buffer*/
	struct Stream* Source;  /* Source for filling Input buffer */

	Int32 Channels, SampleRate;
	Int32 BlockSizes[2];

	struct Codebook* Codebooks;
	struct Floor* Floors;
	struct Residue* Residues;
	struct Mapping* Mappings;
	struct Mode* Modes;
};
void Vorbis_Init(struct VorbisState* state, struct Stream* source);
#endif
