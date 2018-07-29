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
	UInt32 Dimensions, Entries, NumCodewords;
	UInt32* Codewords;
	UInt8* CodewordLens;
	Real32 MinValue, DeltaValue;
	UInt32 SequenceP, LookupType, LookupValues;
	UInt16* Multiplicands;
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

void Codebook_CalcCodewords(struct Codebook* codebook, UInt8* codewordLens, Int16 usedEntries) {
	codebook->NumCodewords = usedEntries;
	codebook->Codewords    = Platform_MemAlloc(usedEntries, sizeof(UInt32), "codewords");
	codebook->CodewordLens = Platform_MemAlloc(usedEntries, sizeof(UInt8), "raw codeword lens");

	Int32 i, j;
	UInt32 lastCodeword = UInt32_MaxValue;
	UInt32 lastLen = 32; UInt8 len;

	for (i = 0, j = 0; i < codebook->Entries; i++) {
		len = codewordLens[i];
		if (len == UInt8_MaxValue) continue;
		codebook->CodewordLens[j] = len;

		/* work out where to start depth of next codeword */
		UInt32 depth = min(lastLen, len);
		UInt32 mask = ~(UInt32_MaxValue >> depth); /* e.g. depth of 4, 0xF0 00 00 00 */

		/* for example, assume Tree is like this: 
		      #
		    0/ \
			#
		  0/ \1
		  #   #
		$3  0/                                               
			
		*/

		/* NOPE, THIS ASSUMPTION IS WRONG */
		UInt32 nextCodeword = (lastCodeword & mask) + (1UL << (32 - depth));
		codebook->Codewords[j] = nextCodeword;

		lastCodeword = nextCodeword; 
		lastLen = len;
	}
}

ReturnCode Codebook_DecodeSetup(struct VorbisState* state, struct Codebook* codebook) {
	UInt32 sync = Vorbis_ReadBits(state, 24);
	if (sync != CODEBOOK_SYNC) return VORBIS_ERR_CODEBOOK_SYNC;
	codebook->Dimensions = Vorbis_ReadBits(state, 16);
	codebook->Entries = Vorbis_ReadBits(state, 24);

	UInt8* codewordLens = Platform_MemAlloc(codebook->Entries, sizeof(UInt8), "raw codeword lens");
	Int32 i, ordered = Vorbis_ReadBits(state, 1), usedEntries = 0;

	if (!ordered) {
		Int32 sparse = Vorbis_ReadBits(state, 1);
		for (i = 0; i < codebook->Entries; i++) {
			if (sparse) {
				Int32 flag = Vorbis_ReadBits(state, 1);
				if (!flag) {
					codewordLens[i] = UInt8_MaxValue; 
					continue; /* unused entry */
				}
			}

			Int32 len = Vorbis_ReadBits(state, 5); len++;
			codewordLens[i] = len;
			usedEntries++;
		}
	} else {
		Int32 entry;
		Int32 curLength = Vorbis_ReadBits(state, 5); curLength++;
		for (entry = 0; entry < codebook->Entries;) {
			Int32 runBits = iLog(codebook->Entries - entry);
			Int32 runLen = Vorbis_ReadBits(state, runBits);

			for (i = entry; i < entry + runLen; i++) {
				codewordLens[i] = curLength;
			}

			entry += runLen;
			curLength++;
			if (entry > codebook->Entries) return VORBIS_ERR_CODEBOOK_ENTRY;
		}
		usedEntries = codebook->Entries;
	}

	Codebook_CalcCodewords(codebook, codewordLens, usedEntries);
	Platform_MemFree(&codewordLens);

	codebook->LookupType = Vorbis_ReadBits(state, 4);
	if (codebook->LookupType == 0) return 0;
	if (codebook->LookupType > 2)  return VORBIS_ERR_CODEBOOK_LOOKUP;

	codebook->MinValue   = float32_unpack(state);
	codebook->DeltaValue = float32_unpack(state);
	Int32 valueBits = Vorbis_ReadBits(state, 4); valueBits++;
	codebook->SequenceP  = Vorbis_ReadBits(state, 1);

	UInt32 lookupValues;
	if (codebook->LookupType == 1) {
		lookupValues = lookup1_values(codebook->Entries, codebook->Dimensions);
	} else {
		lookupValues = codebook->Entries * codebook->Dimensions;
	}
	codebook->LookupValues = lookupValues;

	codebook->Multiplicands = Platform_MemAlloc(lookupValues, sizeof(UInt16), "multiplicands");
	for (i = 0; i < lookupValues; i++) {
		codebook->Multiplicands[i] = Vorbis_ReadBits(state, valueBits);
	}
	return 0;
}

Real32* Codebook_DecodeVQ_Type1(struct Codebook* codebook, UInt32 lookupOffset) {
	Real32 last = 0.0f;
	UInt32 indexDivisor = 1, i;
	Real32* values;

	for (i = 0; i < codebook->Dimensions; i++) {
		UInt32 offset = (lookupOffset / indexDivisor) % codebook->LookupValues;
		values[i] = codebook->Multiplicands[offset] * codebook->DeltaValue + codebook->MinValue + last;
		if (codebook->SequenceP) last = values[i];
		indexDivisor *= codebook->LookupValues;
	}
	return values;
}

Real32* Codebook_DecodeVQ_Type2(struct Codebook* codebook, UInt32 lookupOffset) {
	Real32 last = 0.0f;
	UInt32 i, offset = lookupOffset * codebook->Dimensions;
	Real32* values;

	for (i = 0; i < codebook->Dimensions; i++, offset++) {
		values[i] = codebook->Multiplicands[offset] * codebook->DeltaValue + codebook->MinValue + last;
		if (codebook->SequenceP) last = values[i];
	}
	return values;
}

UInt32 Codebook_DecodeScalar(struct Codebook* codebook) {
	return -1;
}

Real32* Codebook_DecodeVectors(struct Codebook* codebook) {
	UInt32 lookupOffset = Codebook_DecodeScalar(codebook);
	switch (codebook->LookupType) {
	case 1: return Codebook_DecodeVQ_Type1(codebook, lookupOffset);
	case 2: return Codebook_DecodeVQ_Type2(codebook, lookupOffset);
	}
	return NULL;
}

/*########################################################################################################################*
*-----------------------------------------------------Vorbis floors-------------------------------------------------------*
*#########################################################################################################################*/
#define FLOOR_MAX_PARTITIONS 32
#define FLOOR_MAX_CLASSES 16
#define FLOOR_MAX_VALUES (FLOOR_MAX_PARTITIONS * 8)
struct Floor {
	UInt8 Partitions, Multiplier; Int32 Range, Values;
	UInt8 PartitionClasses[FLOOR_MAX_PARTITIONS];
	UInt8 ClassDimensions[FLOOR_MAX_CLASSES];
	UInt8 ClassSubClasses[FLOOR_MAX_CLASSES];
	UInt8 ClassMasterbooks[FLOOR_MAX_CLASSES];
	Int16 SubclassBooks[FLOOR_MAX_CLASSES][8];
	Int16 XList[FLOOR_MAX_VALUES];
	Int32 YList[FLOOR_MAX_VALUES];
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
	static Int16 ranges[4] = { 256, 128, 84, 64 };
	floor->Range = ranges[floor->Multiplier - 1];

	UInt32 rangeBits = Vorbis_ReadBits(state, 4);
	floor->XList[0]  = 0;
	floor->XList[1]  = 1 << rangeBits;

	for (i = 0, idx = 2; i < floor->Partitions; i++) {
		Int32 classNum = floor->PartitionClasses[i];
		for (j = 0; j < floor->ClassDimensions[classNum]; j++) {
			floor->XList[idx++] = Vorbis_ReadBits(state, rangeBits);
		}
	}
	floor->Values = idx;
	return 0;
}

bool Floor_DecodeFrame(struct VorbisState* state, struct Floor* floor) {
	Int32 nonZero = Vorbis_ReadBits(state, 1);
	if (!nonZero) return false;

	Int32 i, j, idx, rangeBits = iLog(floor->Range - 1);
	floor->YList[0] = Vorbis_ReadBits(state, rangeBits);
	floor->YList[1] = Vorbis_ReadBits(state, rangeBits);

	for (i = 0, idx = 2; i < floor->Partitions; i++) {
		UInt8 class = floor->PartitionClasses[i];
		UInt8 cdim = floor->ClassDimensions[class];
		UInt8 cbits = floor->ClassSubClasses[class];

		UInt32 csub = (1 << cbits) - 1;
		UInt32 cval = 0;
		if (cbits) {
			UInt8 bookNum = floor->ClassMasterbooks[class];
			cval = Codebook_DecodeScalar(&state->Codebooks[bookNum]);
		}

		for (j = 0; j < cdim; j++) {
			Int16 bookNum = floor->SubclassBooks[class][cval & csub];
			cval <<= cbits;
			if (bookNum >= 0) {
				floor->YList[idx + j] = Codebook_DecodeScalar(&state->Codebooks[bookNum]);
			} else {
				floor->YList[idx + j] = 0;
			}
		}
		idx += cdim;
	}
	return true;
}

Int32 render_point(Int32 x0, Int32 y0, Int32 x1, Int32 y1, Int32 X) {
	Int32 dy = y1 - y0, adx = x1 - x0;
	Int32 ady = Math_AbsI(dy);
	Int32 err = ady * (X - x0);
	Int32 off = err / adx;

	if (dy < 0) {
		return y0 - off;
	} else {
		return y0 + off;
	}
}

void render_line(Int32 x0, Int32 y0, Int32 x1, Int32 y1, Int32* v) {
	Int32 dy = y1 - y0, adx = x1 - x0;
	Int32 ady = Math_AbsI(dy);
	Int32 base = dy / adx, sy;
	Int32 x = x0, y = y0, err = 0;

	if (dy < 0) {
		sy = base - 1;
	} else {
		sy = base + 1;
	}

	ady = ady - Math_AbsI(base) * adx;
	v[x] = y;

	for (x = x0 + 1; x < x1; x++) {
		err = err + ady;
		if (err >= adx) {
			err = err - adx;
			y = y + sy;
		} else {
			y = y + base;
		}
v[x] = y;
	}
}

void Floor_Synthesis(struct VorbisState* state, struct Floor* floor) {
	/* amplitude value synthesis */
	Int32 YFinal[FLOOR_MAX_VALUES];
	bool Step2[FLOOR_MAX_VALUES];

	Step2[0] = true;
	Step2[1] = true;
	YFinal[0] = floor->YList[0];
	YFinal[1] = floor->YList[1];

	Int32 i;
	for (i = 2; i < floor->Values; i++) {
		Int32 lo_offset = low_neighbor(floor->XList, i);
		Int32 hi_offset = high_neighbor(floor->XList, i);

		Int32 predicted = render_point(floor->XList[lo_offset], YFinal[lo_offset],
			floor->XList[hi_offset], YFinal[hi_offset], floor->XList[i]);

		Int32 val = floor->YList[i];
		Int32 highroom = floor->Range - predicted;
		Int32 lowroom = predicted, room;

		if (highroom < lowroom) {
			room = highroom * 2;
		} else {
			room = lowroom * 2;
		}

		if (val) {
			Step2[lo_offset] = true;
			Step2[hi_offset] = true;
			Step2[i] = true;

			if (val >= room) {
				if (highroom > lowroom) {
					YFinal[i] = val - lowroom + predicted;
				} else {
					YFinal[i] = predicted - val + highroom - 1;
				}
			} else {
				if (val & 1) {
					YFinal[i] = predicted - (val + 1) / 2;
				} else {
					YFinal[i] = predicted + val / 2;
				}
			}
		} else {
			Step2[i] = false;
			YFinal[i] = predicted;
		}
	}
	/* TODO: curve synthesis */
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
	residue->End = Vorbis_ReadBits(state, 24);
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

void Residue_DecodeFrame(struct VorbisState* state, struct Residue* residue, Int32 ch, bool* doNotDecode) {
	UInt32 size = state->CurBlockSize / 2;
	if (residue->Type == 2) size *= ch;

	UInt32 residueBeg = max(residue->Begin, size);
	UInt32 residueEnd = max(residue->End,   size);

	struct Codebook* classbook = &state->Codebooks[residue->Classbook];
	UInt32 classwordsPerCodeword = classbook->Dimensions;
	UInt32 pass, i, j, nToRead = residueEnd - residueBeg;
	UInt32 partitionsToRead = nToRead / residue->PartitionSize;

/* TODO:  allocate and zero all vectors that will be returned. */
	if (nToRead == 0) return;
	for (pass = 0; pass < 8; pass++) {
		UInt32 partitionCount = 0;
		while (partitionCount < partitionsToRead) {

			/* read classifications in pass 0 */
			if (pass == 0) {
				for (j = 0; j < ch; j++) {
					if (doNotDecode[j]) continue;

					UInt32 temp = Codebook_DecodeScalar(&classbook);
					/* TODO: i must be signed, otherwise infinite loop */
					for (i = classwordsPerCodeword - 1; i >= 0; i--) {
						classifications[j][i + partitionCount] = temp % residue->Classifications;
						temp /= residue->Classifications;
					}
				}
			}

			for (i = 0; i < classwordsPerCodeword && partitionCount < partitionsToRead; i++) {
				for (j = 0; j < ch; j++) {
					if (doNotDecode[j]) continue;

					UInt8 class = classifications[j][partitionCount];
					Int16 book = residue->Books[class][pass];

					if (book >= 0) {
						decode partition into output vector number[j], starting at scalar
							offset[limit\_residue\_begin] + [partition\_count] * [residue\_partition\_size] using
							codebook number[vqbook] in VQ context
					}
				}
				partitionCount++;
			}
		}
	}
}


/*########################################################################################################################*
*----------------------------------------------------Vorbis mappings------------------------------------------------------*
*#########################################################################################################################*/
#define MAPPING_MAX_COUPLINGS 256
#define MAPPING_MAX_SUBMAPS 15
struct Mapping {
	UInt8 CouplingSteps, Submaps;
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
			mapping->Angle[i]     = Vorbis_ReadBits(state, couplingBits);
			if (mapping->Magnitude[i] == mapping->Angle[i]) return VORBIS_ERR_MAPPING_CHANS;
		}
	}

	Int32 reserved = Vorbis_ReadBits(state, 2);
	if (reserved != 0) return VORBIS_ERR_MAPPING_RESERVED;
	mapping->Submaps = submaps;
	mapping->CouplingSteps = couplingSteps;

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
*-----------------------------------------------------Vorbis setup--------------------------------------------------------*
*#########################################################################################################################*/
struct Mode { UInt8 BlockSizeFlag, MappingIdx; };
ReturnCode Mode_DecodeSetup(struct VorbisState* state, struct Mode* mode) {
	mode->BlockSizeFlag = Vorbis_ReadBits(state, 1);
	UInt16 windowType = Vorbis_ReadBits(state, 16);
	if (windowType != 0) return VORBIS_ERR_MODE_WINDOW;

	UInt16 transformType = Vorbis_ReadBits(state, 16);
	if (transformType != 0) return VORBIS_ERR_MODE_TRANSFORM;
	mode->MappingIdx = Vorbis_ReadBits(state, 8);
	return 0;
}

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

	if (state->Channels == 0 || state->Channels > VORBIS_MAX_CHANS) return VORBIS_ERR_CHANS;
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
	
	state->ModeNumBits = iLog(count - 1); /* ilog([vorbis_mode_count]-1) bits */
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


/*########################################################################################################################*
*-----------------------------------------------------Vorbis frame--------------------------------------------------------*
*#########################################################################################################################*/
ReturnCode Vorbis_DecodeFrame(struct VorbisState* state) {
	Int32 i, j, packetType = Vorbis_ReadBits(state, 1);
	if (packetType) return VORBIS_ERR_FRAME_TYPE;

	Int32 modeIdx = Vorbis_ReadBits(state, state->ModeNumBits);
	struct Mode* mode = &state->Modes[modeIdx];
	struct Mapping* mapping = &state->Mappings[mode->MappingIdx];

	/* decode window shape */
	state->CurBlockSize = state->BlockSizes[mode->BlockSizeFlag];
	Int32 prev_window, next_window;
	/* long window lapping*/
	if (mode->BlockSizeFlag) {
		prev_window = Vorbis_ReadBits(state, 1);
		next_window = Vorbis_ReadBits(state, 1);
	}	

	/* decode floor */
	bool hasFloor[VORBIS_MAX_CHANS], hasResidue[VORBIS_MAX_CHANS];
	for (i = 0; i < state->Channels; i++) {
		Int32 submap = mapping->Mux[i];
		Int32 floorIdx = mapping->FloorIdx[submap];
		hasFloor[i] = Floor_DecodeFrame(state, &state->Floors[floorIdx]);
		hasResidue[i] = hasFloor[i];
	}

	/* non-zero vector propogate */
	for (i = 0; i < mapping->CouplingSteps; i++) {
		Int32 magChannel = mapping->Magnitude[i];
		Int32 angChannel = mapping->Angle[i];

		if (hasResidue[magChannel] || hasResidue[angChannel]) {
			hasResidue[magChannel] = true;
			hasResidue[angChannel] = true;
		}
	}

	/* decode residue */
	for (i = 0; i < mapping->Submaps; i++) {
		Int32 ch = 0;
		bool doNotDecode[VORBIS_MAX_CHANS];
		for (j = 0; j < state->Channels; j++) {
			if (mapping->Mux[j] != i) continue;

			doNotDecode[ch] = !hasResidue[j];
			ch++;
		}

		Int32 residueIdx = mapping->FloorIdx[i];
		Residue_DecodeFrame(state, &state->Residues[residueIdx], ch, doNotDecode);
		ch = 0;

		for (j = 0; j < state->Channels; j++) {
			if (mapping->Mux[j] != i) continue;
			residue vector for channel[j] is set to decoded residue vector[ch]
			ch++;
		}
	}

	/* compute dot product of floor and residue, producing audio spectrum vector */

	/* inverse monolithic transform of audio spectrum vector */

}