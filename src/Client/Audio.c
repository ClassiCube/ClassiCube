#include "Audio.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Event.h"
#include "Block.h"
#include "ExtMath.h"
#include "Funcs.h"

/*########################################################################################################################*
*-------------------------------------------------------Ogg stream--------------------------------------------------------*
*#########################################################################################################################*/
#define OGG_FourCC(a, b, c, d) (((UInt32)a << 24) | ((UInt32)b << 16) | ((UInt32)c << 8) | (UInt32)d)
static ReturnCode Ogg_NextPage(struct Stream* stream) {
	UInt8 header[27];
	struct Stream* source = stream->Meta.Ogg.Source;
	Stream_Read(source, header, sizeof(header));

	UInt32 sig = Stream_GetU32_BE(&header[0]);
	if (sig != OGG_FourCC('O','g','g','S')) return OGG_ERR_INVALID_SIG;
	if (header[4] != 0) return OGG_ERR_VERSION;
	UInt8 bitflags = header[5];
	/* (8) granule position */
	/* (4) serial number */
	/* (4) page sequence number */
	/* (4) page checksum */

	Int32 i, numSegments = header[26];
	UInt8 segments[255];
	Stream_Read(source, segments, numSegments);

	UInt32 dataSize = 0;
	for (i = 0; i < numSegments; i++) { dataSize += segments[i]; }
	Stream_Read(source, stream->Meta.Ogg.Base, dataSize);

	stream->Meta.Ogg.Cur  = stream->Meta.Ogg.Base;
	stream->Meta.Ogg.Left = dataSize;
	stream->Meta.Ogg.Last = bitflags & 4;
	return 0;
}

static ReturnCode Ogg_Read(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	for (;;) {
		if (stream->Meta.Ogg.Left) {
			count = min(count, stream->Meta.Ogg.Left);
			Platform_MemCpy(data, stream->Meta.Ogg.Cur, count);

			*modified = count;
			stream->Meta.Ogg.Cur  += count;
			stream->Meta.Ogg.Left -= count;
			return 0;
		}

		/* try again with data from next page*/
		*modified = 0;
		if (stream->Meta.Ogg.Last) return 0;

		ReturnCode result = Ogg_NextPage(stream);
		if (result != 0) return result;
	}
}

void Ogg_MakeStream(struct Stream* stream, UInt8* buffer, struct Stream* source) {
	Stream_Init(stream, &source->Name);
	stream->Read = Ogg_Read;

	stream->Meta.Ogg.Cur = buffer;
	stream->Meta.Ogg.Base = buffer;
	stream->Meta.Ogg.Left = 0;
	stream->Meta.Ogg.Last = 0;
	stream->Meta.Ogg.Source = source;
}


/*########################################################################################################################*
*------------------------------------------------------Vorbis utils-------------------------------------------------------*
*#########################################################################################################################*/
/* Insert next byte into the bit buffer */
#define Vorbis_GetByte(state) state->Bits |= (UInt32)Stream_ReadU8(state->Source) << state->NumBits; state->NumBits += 8;
/* Retrieves bits from the bit buffer */
#define Vorbis_PeekBits(state, bits) (state->Bits & ((1UL << (bits)) - 1UL))
/* Consumes/eats up bits from the bit buffer */
#define Vorbis_ConsumeBits(state, bits) state->Bits >>= (bits); state->NumBits -= (bits);
/* Aligns bit buffer to be on a byte boundary */
#define Vorbis_AlignBits(state) UInt32 alignSkip = state->NumBits & 7; Vorbis_ConsumeBits(state, alignSkip);
/* Ensures there are 'bitsCount' bits */
#define Vorbis_EnsureBits(state, bitsCount) while (state->NumBits < bitsCount) { Vorbis_GetByte(state); }
/* Peeks then consumes given bits */
/* TODO: Make this an inline macro somehow */
UInt32 Vorbis_ReadBits(struct VorbisState* state, UInt32 bitsCount) {
	Vorbis_EnsureBits(state, bitsCount);
	UInt32 result = Vorbis_PeekBits(state, bitsCount); Vorbis_ConsumeBits(state, bitsCount);
	return result;
}

#define VORBIS_MAX_CHANS 8

Int32 iLog(Int32 x) {
	Int32 bits = 0;
	while (x > 0) { bits++; x >>= 2; }
	return bits;
}

Real32 float32_unpack(struct VorbisState* state) {
	/* encoder macros can't reliably read over 24 bits */
	UInt32 lo = Vorbis_ReadBits(state, 16);
	UInt32 hi = Vorbis_ReadBits(state, 16);
	UInt32 x = (hi << 16) | lo;

	Int32 mantissa = x & 0x1fffff;
	UInt32 exponent = (x & 0x7fe00000) >> 21;
	if (x & 0x80000000UL) mantissa = -mantissa;

	#define LOG_2 0.693147180559945
	/* TODO: Can we make this more accurate? maybe ldexp ?? */
	return (Real32)(mantissa * Math_Exp(LOG_2 * ((Int32)exponent - 788))); /* pow(2, x) */
}


/*########################################################################################################################*
*----------------------------------------------------Vorbis codebooks-----------------------------------------------------*
*#########################################################################################################################*/
#define CODEBOOK_SYNC 0x564342
struct Codebook {
	UInt32 Dimensions, Entries;
};

UInt32 Codebook_Pow(UInt32 base, UInt32 exp) {
	UInt32 result = 1; /* exponentiation by squaring */
	while (exp) {
		if (exp & 1) result *= base;
		exp >>= 1;
		base *= base;
	}
	return result;
}

UInt32 lookup1_values(UInt32 entries, UInt32 dimensions) {
	UInt32 i;
	/* the greatest integer value for which [value] to the power of [dimensions] is less than or equal to [entries] */
	/* TODO: verify this */
	for (i = 1; ; i++) {
		UInt32 pow  = Codebook_Pow(i, dimensions);
		UInt32 next = Codebook_Pow(i + 1, dimensions);

		if (next < pow)        return i; /* overflow */
		if (pow == entries) return i;
		if (next > entries) return i;
	}
	return 0;
}

ReturnCode Codebook_DecodeSetup(struct VorbisState* state, struct Codebook* codebook) {
	UInt32 sync = Vorbis_ReadBits(state, 24);
	if (sync != CODEBOOK_SYNC) return VORBIS_ERR_CODEBOOK_SYNC;
	codebook->Dimensions = Vorbis_ReadBits(state, 16);
	codebook->Entries = Vorbis_ReadBits(state, 24);

	Int32 i, ordered = Vorbis_ReadBits(state, 1);
	if (!ordered) {
		Int32 sparse = Vorbis_ReadBits(state, 1);
		for (i = 0; i < codebook->Entries; i++) {
			if (sparse) {
				Int32 flag = Vorbis_ReadBits(state, 1);
				if (!flag) continue; /* TODO: unused entry */
			}

			Int32 len = Vorbis_ReadBits(state, 5); len++;
		}
	} else {
		Int32 entry;
		Int32 curLength = Vorbis_ReadBits(state, 5); curLength++;
		for (entry = 0; entry < codebook->Entries;) {
			Int32 runBits = iLog(codebook->Entries - entry);
			Int32 runLen = Vorbis_ReadBits(state, runBits);

			for (i = entry; i < entry + runLen; i++) {
				Int32 len = curLength;
			}

			entry += runLen;
			curLength++;
			if (entry > codebook->Entries) return VORBIS_ERR_CODEBOOK_ENTRY;
		}
	}

	Int32 lookupType = Vorbis_ReadBits(state, 4);
	if (lookupType == 0) return 0;
	if (lookupType > 2) return VORBIS_ERR_CODEBOOK_LOOKUP;

	Real32 minValue   = float32_unpack(state);
	Real32 deltaValue = float32_unpack(state);
	Int32 valueBits = Vorbis_ReadBits(state, 4); valueBits++;
	Int32 sequenceP = Vorbis_ReadBits(state, 1);

	UInt32 lookupValues;
	if (lookupType == 1) {
		lookupValues = lookup1_values(codebook->Entries, codebook->Dimensions);
	} else {
		lookupValues = codebook->Entries * codebook->Dimensions;
	}

	for (i = 0; i < lookupValues; i++) {
		Vorbis_ReadBits(state, valueBits);
	}
	return 0;
}


/*########################################################################################################################*
*-----------------------------------------------------Vorbis floors-------------------------------------------------------*
*#########################################################################################################################*/
#define FLOOR_MAX_PARTITIONS 32
#define FLOOR_MAX_CLASSES 16
struct Floor {
	UInt8 Partitions, Multiplier, RangeBits;
	UInt8 PartitionClasses[FLOOR_MAX_PARTITIONS];
	UInt8 ClassDimensions[FLOOR_MAX_CLASSES];
	UInt8 ClassSubClasses[FLOOR_MAX_CLASSES];
	UInt8 ClassMasterbooks[FLOOR_MAX_CLASSES];
	Int16 SubclassBooks[FLOOR_MAX_CLASSES][8];
	Int16 XList[FLOOR_MAX_PARTITIONS * 8];
};

ReturnCode Floor_DecodeSetup(struct VorbisState* state, struct Floor* floor) {
	floor->Partitions = Vorbis_ReadBits(state, 5);
	Int32 i, j, idx, maxClass = -1;

	for (i = 0; i < floor->Partitions; i++) {
		floor->PartitionClasses[i] = Vorbis_ReadBits(state, 4);
		maxClass = max(maxClass, floor->PartitionClasses[i]);
	}

	for (i = 0; i <= maxClass; i++) {
		floor->ClassDimensions[i] = Vorbis_ReadBits(state, 3); floor->ClassDimensions[i]++;
		floor->ClassSubClasses[i] = Vorbis_ReadBits(state, 2);
		if (floor->ClassSubClasses[i]) {
			floor->ClassMasterbooks[i] = Vorbis_ReadBits(state, 8);
		}
		for (j = 0; j < (1 << floor->ClassSubClasses[i]); j++) {
			floor->SubclassBooks[i][j] = Vorbis_ReadBits(state, 8); floor->SubclassBooks[i][j]--;
		}
	}

	floor->Multiplier = Vorbis_ReadBits(state, 2); floor->Multiplier++;
	floor->RangeBits  = Vorbis_ReadBits(state, 4);
	floor->XList[0] = 0;
	floor->XList[1] = 1 << floor->RangeBits;

	for (i = 0, idx = 2; i < floor->Partitions; i++) {
		Int32 classNum = floor->PartitionClasses[i];
		for (j = 0; j < floor->ClassDimensions[classNum]; j++) {
			floor->XList[idx++] = Vorbis_ReadBits(state, floor->RangeBits);
		}
	}
	return 0;
}


/*########################################################################################################################*
*----------------------------------------------------Vorbis residues------------------------------------------------------*
*#########################################################################################################################*/
#define RESIDUE_MAX_CLASSIFICATIONS 65
struct Residue { 
	UInt8 Type, Classifications, Classbook;
	UInt32 Begin, End, PartitionSize;
	UInt8 Cascade[RESIDUE_MAX_CLASSIFICATIONS];
	Int16 Books[RESIDUE_MAX_CLASSIFICATIONS][8];
};

ReturnCode Residue_DecodeSetup(struct VorbisState* state, struct Residue* residue, UInt16 type) {
	residue->Type = type;
	residue->Begin = Vorbis_ReadBits(state, 24);
	residue->End   = Vorbis_ReadBits(state, 24);
	residue->PartitionSize = Vorbis_ReadBits(state, 24); residue->PartitionSize++;
	residue->Classifications = Vorbis_ReadBits(state, 6); residue->Classifications++;
	residue->Classbook = Vorbis_ReadBits(state, 8);

	Int32 i;
	for (i = 0; i < residue->Classifications; i++) {
		residue->Cascade[i] = Vorbis_ReadBits(state, 3);
		Int32 hasBits = Vorbis_ReadBits(state, 1);
		if (!hasBits) continue;
		Int32 bits = Vorbis_ReadBits(state, 5);
		residue->Cascade[i] |= bits << 3;
	}

	Int32 j;
	for (i = 0; i < residue->Classifications; i++) {
		for (j = 0; j < 8; j++) {
			Int16 codebook = -1;
			if (residue->Cascade[i] & (1 << j)) {
				codebook = Vorbis_ReadBits(state, 8);
			}
			residue->Books[i][j] = codebook;
		}
	}
	return 0;
}


/*########################################################################################################################*
*----------------------------------------------------Vorbis mappings------------------------------------------------------*
*#########################################################################################################################*/
#define MAPPING_MAX_COUPLINGS 256
#define MAPPING_MAX_SUBMAPS 15
struct Mapping {
	UInt8 Mux[VORBIS_MAX_CHANS];
	UInt8 FloorIdx[MAPPING_MAX_SUBMAPS];
	UInt8 ResidueIdx[MAPPING_MAX_SUBMAPS];
	UInt8 Magnitude[MAPPING_MAX_COUPLINGS];
	UInt8 Angle[MAPPING_MAX_COUPLINGS];
};
ReturnCode Mapping_DecodeSetup(struct VorbisState* state, struct Mapping* mapping) {
	Int32 i, submaps = 1, submapFlag = Vorbis_ReadBits(state, 1);
	if (submapFlag) {
		submaps = Vorbis_ReadBits(state, 4); submaps++;
	}

	Int32 couplingSteps = 0, couplingFlag = Vorbis_ReadBits(state, 1);
	if (couplingFlag) {
		couplingSteps = Vorbis_ReadBits(state, 8); couplingSteps++;
		/* TODO: How big can couplingSteps ever really get in practice? */
		Int32 couplingBits = iLog(state->Channels - 1);
		for (i = 0; i < couplingSteps; i++) {
			mapping->Magnitude[i] = Vorbis_ReadBits(state, couplingBits);
			mapping->Angle[i]    = Vorbis_ReadBits(state, couplingBits);
			if (mapping->Magnitude[i] == mapping->Angle[i]) return VORBIS_ERR_MAPPING_CHANS;
		}
	}

	Int32 reserved = Vorbis_ReadBits(state, 2);
	if (reserved != 0) return VORBIS_ERR_MAPPING_RESERVED;

	if (submaps > 1) {
		for (i = 0; i < state->Channels; i++) {
			mapping->Mux[i] = Vorbis_ReadBits(state, 4);
		}
	} else {
		for (i = 0; i < state->Channels; i++) {
			mapping->Mux[i] = 0;
		}
	}

	for (i = 0; i < submaps; i++) {
		Vorbis_ReadBits(state, 8); /* time value */
		mapping->FloorIdx[i]   = Vorbis_ReadBits(state, 8);
		mapping->ResidueIdx[i] = Vorbis_ReadBits(state, 8);
	}
	return 0;
}


/*########################################################################################################################*
*----------------------------------------------------Vorbis modes------------------------------------------------------*
*#########################################################################################################################*/
struct Mode { UInt8 BlockSizeIdx, MappingIdx; };
ReturnCode Mode_DecodeSetup(struct VorbisState* state, struct Mode* mode) {
	mode->BlockSizeIdx = Vorbis_ReadBits(state, 1);
	UInt16 windowType = Vorbis_ReadBits(state, 16);
	if (windowType != 0) return VORBIS_ERR_MODE_WINDOW;

	UInt16 transformType = Vorbis_ReadBits(state, 16);
	if (transformType != 0) return VORBIS_ERR_MODE_TRANSFORM;
	mode->MappingIdx = Vorbis_ReadBits(state, 8);
	return 0;
}


/*########################################################################################################################*
*----------------------------------------------------Vorbis decoding------------------------------------------------------*
*#########################################################################################################################*/
void Vorbis_Init(struct VorbisState* state, struct Stream* source) {
	Platform_MemSet(state, 0, sizeof(struct VorbisState));
	state->Source = source;
}

/* TODO: Work out Vorbis_Free implementation */

static bool Vorbis_ValidBlockSize(UInt32 blockSize) {
	return blockSize >= 64 && blockSize <= 8192 && Math_IsPowOf2(blockSize);
}

static ReturnCode Vorbis_DecodeHeader(struct VorbisState* state, UInt8 type) {
	UInt8 header[7];
	Stream_Read(state->Source, header, sizeof(header));
	if (header[0] != type) return VORBIS_ERR_WRONG_HEADER;

	bool OK = 
		header[1] == 'v' && header[2] == 'o' && header[3] == 'r' &&
		header[4] == 'b' && header[5] == 'i' && header[6] == 's';
	return OK ? 0 : ReturnCode_InvalidArg;
}

static ReturnCode Vorbis_DecodeIdentifier(struct VorbisState* state) {
	UInt8 header[23];
	Stream_Read(state->Source, header, sizeof(header));
	UInt32 version    = Stream_GetU32_LE(&header[0]);
	if (version != 0) return VORBIS_ERR_VERSION;

	state->Channels   = header[4];
	state->SampleRate = Stream_GetU32_LE(&header[5]);
	/* (12) bitrate_maximum, nominal, minimum */
	state->BlockSizes[0] = 1 << (header[21] & 0xF);
	state->BlockSizes[1] = 1 << (header[21] >>  4);

	if (!Vorbis_ValidBlockSize(state->BlockSizes[0])) return VORBIS_ERR_BLOCKSIZE;
	if (!Vorbis_ValidBlockSize(state->BlockSizes[1])) return VORBIS_ERR_BLOCKSIZE;
	if (state->BlockSizes[0] > state->BlockSizes[1])  return VORBIS_ERR_BLOCKSIZE;

	/* check framing flag */
	return (header[22] & 1) ? 0 : VORBIS_ERR_FRAMING;
}

static ReturnCode Vorbis_DecodeComments(struct VorbisState* state) {
	UInt32 vendorLen = Stream_ReadU32_LE(state->Source);
	Stream_Skip(state->Source, vendorLen);

	UInt32 i, comments = Stream_ReadU32_LE(state->Source);
	for (i = 0; i < comments; i++) {
		UInt32 commentLen = Stream_ReadU32_LE(state->Source);
		Stream_Skip(state->Source, commentLen);
	}

	/* check framing flag */
	return (Stream_ReadU8(state->Source) & 1) ? 0 : VORBIS_ERR_FRAMING;
}

static ReturnCode Vorbis_DecodeSetup(struct VorbisState* state) {
	Int32 i, count;
	ReturnCode result;

	count = Vorbis_ReadBits(state, 8); count++;
	state->Codebooks = Platform_MemAlloc(count, sizeof(struct Codebook), "vorbis codebooks");
	for (i = 0; i < count; i++) {
		result = Codebook_DecodeSetup(state, &state->Codebooks[i]);	
		if (result) return result;
	}

	count = Vorbis_ReadBits(state, 6); count++;
	for (i = 0; i < count; i++) {
		UInt16 time = Vorbis_ReadBits(state, 16);
		if (time != 0) return VORBIS_ERR_TIME_TYPE;
	}

	count = Vorbis_ReadBits(state, 6); count++;
	state->Floors = Platform_MemAlloc(count, sizeof(struct Floor), "vorbis floors");
	for (i = 0; i < count; i++) {
		UInt16 floor = Vorbis_ReadBits(state, 16);
		if (floor != 1) return VORBIS_ERR_FLOOR_TYPE;
		result = Floor_DecodeSetup(state, &state->Floors[i]);
		if (result) return result;
	}

	count = Vorbis_ReadBits(state, 6); count++;
	state->Residues = Platform_MemAlloc(count, sizeof(struct Residue), "vorbis residues");
	for (i = 0; i < count; i++) {
		UInt16 residue = Vorbis_ReadBits(state, 16);
		if (residue > 2) return VORBIS_ERR_FLOOR_TYPE;
		result = Residue_DecodeSetup(state, &state->Residues[i], residue);
		if (result) return result;
	}

	count = Vorbis_ReadBits(state, 6); count++;
	state->Mappings = Platform_MemAlloc(count, sizeof(struct Mapping), "vorbis mappings");
	for (i = 0; i < count; i++) {
		UInt16 mapping = Vorbis_ReadBits(state, 16);
		if (mapping != 0) return VORBIS_ERR_MAPPING_TYPE;
		result = Mapping_DecodeSetup(state, &state->Mappings[i]);
		if (result) return result;
	}

	count = Vorbis_ReadBits(state, 6); count++;
	state->Modes = Platform_MemAlloc(count, sizeof(struct Mode), "vorbis modes");
	for (i = 0; i < count; i++) {
		result = Mode_DecodeSetup(state, &state->Modes[i]);
		if (result) return result;
	}

	UInt8 framing = Vorbis_ReadBits(state, 1);
	Vorbis_AlignBits(state);
	/* check framing flag */
	return (framing & 1) ? 0 : VORBIS_ERR_FRAMING;
}

enum VORBIS_PACKET { VORBIS_IDENTIFIER = 1, VORBIS_COMMENTS = 3, VORBIS_SETUP = 5, };
ReturnCode Vorbis_DecodeHeaders(struct VorbisState* state) {
	ReturnCode result;
	
	result = Vorbis_DecodeHeader(state, VORBIS_IDENTIFIER);
	if (result) return result;
	result = Vorbis_DecodeIdentifier(state);
	if (result) return result;

	result = Vorbis_DecodeHeader(state, VORBIS_COMMENTS);
	if (result) return result;
	result = Vorbis_DecodeComments(state);
	if (result) return result;

	result = Vorbis_DecodeHeader(state, VORBIS_SETUP);
	if (result) return result;
	result = Vorbis_DecodeSetup(state);
	if (result) return result;

	return 0;
}