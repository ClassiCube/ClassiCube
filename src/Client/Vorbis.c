#include "Vorbis.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Event.h"
#include "Block.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Errors.h"

/*########################################################################################################################*
*-------------------------------------------------------Ogg stream--------------------------------------------------------*
*#########################################################################################################################*/
#define OGG_FourCC(a, b, c, d) (((UInt32)a << 24) | ((UInt32)b << 16) | ((UInt32)c << 8) | (UInt32)d)
static ReturnCode Ogg_NextPage(struct Stream* stream) {
	UInt8 header[27];
	struct Stream* source = stream->Meta.Ogg.Source;
	ReturnCode result = Stream_TryRead(source, header, sizeof(header));
	if (result) return result;

	UInt32 sig = Stream_GetU32_BE(&header[0]);
	if (sig != OGG_FourCC('O','g','g','S')) return OGG_ERR_INVALID_SIG;
	if (header[4] != 0) return OGG_ERR_VERSION;
	UInt8 bitflags = header[5];
	/* header[6]  (8) granule position */
	/* header[14] (4) serial number */
	/* header[18] (4) page sequence number */
	/* header[22] (4) page checksum */

	Int32 i, numSegments = header[26];
	UInt8 segments[255];
	result = Stream_TryRead(source, segments, numSegments);
	if (result) return result;

	UInt32 dataSize = 0;
	for (i = 0; i < numSegments; i++) { dataSize += segments[i]; }
	result = Stream_TryRead(source, stream->Meta.Ogg.Base, dataSize);
	if (result) return result;

	stream->Meta.Ogg.Cur  = stream->Meta.Ogg.Base;
	stream->Meta.Ogg.Left = dataSize;
	stream->Meta.Ogg.Last = bitflags & 4;
	return 0;
}

static ReturnCode Ogg_Read(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	for (;;) {
		if (stream->Meta.Ogg.Left) {
			count = min(count, stream->Meta.Ogg.Left);
			Mem_Copy(data, stream->Meta.Ogg.Cur, count);

			*modified = count;
			stream->Meta.Ogg.Cur  += count;
			stream->Meta.Ogg.Left -= count;
			return 0;
		}

		/* try again with data from next page*/
		*modified = 0;
		if (stream->Meta.Ogg.Last) return 0;

		ReturnCode result = Ogg_NextPage(stream);
		if (result) return result;
	}
}

static ReturnCode Ogg_ReadU8(struct Stream* stream, UInt8* data) {
	if (!stream->Meta.Ogg.Left) return Stream_DefaultReadU8(stream, data);

	*data = *stream->Meta.Ogg.Cur;
	stream->Meta.Ogg.Cur++; stream->Meta.Ogg.Left--;
	return 0;
}

void Ogg_MakeStream(struct Stream* stream, UInt8* buffer, struct Stream* source) {
	Stream_Init(stream, &source->Name);
	stream->Read   = Ogg_Read;
	stream->ReadU8 = Ogg_ReadU8;

	stream->Meta.Ogg.Cur = buffer;
	stream->Meta.Ogg.Base = buffer;
	stream->Meta.Ogg.Left = 0;
	stream->Meta.Ogg.Last = 0;
	stream->Meta.Ogg.Source = source;
}


/*########################################################################################################################*
*------------------------------------------------------Vorbis utils-------------------------------------------------------*
*#########################################################################################################################*/
#define Vorbis_PushByte(ctx, value) ctx->Bits |= (UInt32)(value) << ctx->NumBits; ctx->NumBits += 8;
#define Vorbis_PeekBits(ctx, bits) (ctx->Bits & ((1UL << (bits)) - 1UL))
#define Vorbis_ConsumeBits(ctx, bits) ctx->Bits >>= (bits); ctx->NumBits -= (bits);
/* Aligns bit buffer to be on a byte boundary */
#define Vorbis_AlignBits(ctx) UInt32 alignSkip = ctx->NumBits & 7; Vorbis_ConsumeBits(ctx, alignSkip);

/* TODO: Make sure this is inlined */
static UInt32 Vorbis_ReadBits(struct VorbisState* ctx, UInt32 bitsCount) {
	while (ctx->NumBits < bitsCount) { Vorbis_PushByte(ctx, Stream_ReadU8(ctx->Source)); }
	UInt32 data = Vorbis_PeekBits(ctx, bitsCount); Vorbis_ConsumeBits(ctx, bitsCount);
	return data;
}

static ReturnCode Vorbis_TryReadBits(struct VorbisState* ctx, UInt32 bitsCount, UInt32* data) {
	UInt8 portion;
	while (ctx->NumBits < bitsCount) {
		ReturnCode result = ctx->Source->ReadU8(ctx->Source, &portion);
		if (result) return result;
		Vorbis_PushByte(ctx, portion);
	}

	*data = Vorbis_PeekBits(ctx, bitsCount); Vorbis_ConsumeBits(ctx, bitsCount);
	return 0;
}


static Int32 iLog(Int32 x) {
	Int32 bits = 0;
	while (x > 0) { bits++; x >>= 1; }
	return bits;
}

static Real32 float32_unpack(struct VorbisState* ctx) {
	/* ReadBits can't reliably read over 24 bits */
	UInt32 lo = Vorbis_ReadBits(ctx, 16);
	UInt32 hi = Vorbis_ReadBits(ctx, 16);
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
	UInt32* Values;
	/* vector quantisation values */
	Real32 MinValue, DeltaValue;
	UInt32 SequenceP, LookupType, LookupValues;
	UInt16* Multiplicands;
};

static void Codebook_Free(struct Codebook* c) {
	Mem_Free(&c->Codewords);
	Mem_Free(&c->CodewordLens);
	Mem_Free(&c->Values);
	Mem_Free(&c->Multiplicands);
}

static UInt32 Codebook_Pow(UInt32 base, UInt32 exp) {
	UInt32 result = 1; /* exponentiation by squaring */
	while (exp) {
		if (exp & 1) result *= base;
		exp >>= 1;
		base *= base;
	}
	return result;
}

static UInt32 lookup1_values(UInt32 entries, UInt32 dimensions) {
	UInt32 i;
	/* the greatest integer value for which [value] to the power of [dimensions] is less than or equal to [entries] */
	/* TODO: verify this */
	for (i = 1; ; i++) {
		UInt32 pow  = Codebook_Pow(i, dimensions);
		UInt32 next = Codebook_Pow(i + 1, dimensions);

		if (next < pow)     return i; /* overflow */
		if (pow == entries) return i;
		if (next > entries) return i;
	}
	return 0;
}

static bool Codebook_CalcCodewords(struct Codebook* c, UInt8* len) {
	c->Codewords    = Mem_Alloc(c->NumCodewords, sizeof(UInt32), "codewords");
	c->CodewordLens = Mem_Alloc(c->NumCodewords, sizeof(UInt8), "raw codeword lens");
	c->Values       = Mem_Alloc(c->NumCodewords, sizeof(UInt32), "values");

	/* This is taken from stb_vorbis.c because I gave up trying */
	UInt32 i, j, depth;
	UInt32 next_codewords[33] = { 0 };

	/* add codeword 0 to tree */
	for (i = 0; i < c->Entries; i++) {
		if (len[i] == UInt8_MaxValue) continue;

		c->Codewords[0]    = 0;
		c->CodewordLens[0] = len[i];
		c->Values[0]       = i;
		break;
	}

	/* set codewords that new nodes can start from */
	for (depth = 1; depth <= len[i]; depth++) {
		next_codewords[depth] = 1U << (32 - depth);
	}

	i++; /* first codeword was already handled */
	for (j = 1; i < c->Entries; i++) {
		UInt32 root = len[i];
		if (root == UInt8_MaxValue) continue;

		/* per spec, find lowest possible value (leftmost) */
		while (root && next_codewords[root] == 0) root--;
		if (root == 0) return false;

		UInt32 codeword = next_codewords[root];
		next_codewords[root] = 0;

		c->Codewords[j]    = codeword;
		c->CodewordLens[j] = len[i];
		c->Values[j]       = i;

		for (depth = len[i]; depth > root; depth--) {
			next_codewords[depth] = codeword + (1U << (32 - depth));
		}
		j++;
	}
	return true;
}

static ReturnCode Codebook_DecodeSetup(struct VorbisState* ctx, struct Codebook* c) {
	UInt32 sync = Vorbis_ReadBits(ctx, 24);
	if (sync != CODEBOOK_SYNC) return VORBIS_ERR_CODEBOOK_SYNC;
	c->Dimensions = Vorbis_ReadBits(ctx, 16);
	c->Entries = Vorbis_ReadBits(ctx, 24);

	UInt8* codewordLens = Mem_Alloc(c->Entries, sizeof(UInt8), "raw codeword lens");
	Int32 i, ordered = Vorbis_ReadBits(ctx, 1), usedEntries = 0;

	if (!ordered) {
		Int32 sparse = Vorbis_ReadBits(ctx, 1);
		for (i = 0; i < c->Entries; i++) {
			if (sparse) {
				Int32 flag = Vorbis_ReadBits(ctx, 1);
				if (!flag) {
					codewordLens[i] = UInt8_MaxValue; 
					continue; /* unused entry */
				}
			}

			Int32 len = Vorbis_ReadBits(ctx, 5); len++;
			codewordLens[i] = len;
			usedEntries++;
		}
	} else {
		Int32 entry;
		Int32 curLength = Vorbis_ReadBits(ctx, 5); curLength++;
		for (entry = 0; entry < c->Entries;) {
			Int32 runBits = iLog(c->Entries - entry);
			Int32 runLen = Vorbis_ReadBits(ctx, runBits);

			for (i = entry; i < entry + runLen; i++) {
				codewordLens[i] = curLength;
			}

			entry += runLen;
			curLength++;
			if (entry > c->Entries) return VORBIS_ERR_CODEBOOK_ENTRY;
		}
		usedEntries = c->Entries;
	}

	c->NumCodewords = usedEntries;
	Codebook_CalcCodewords(c, codewordLens);
	Mem_Free(&codewordLens);

	c->LookupType = Vorbis_ReadBits(ctx, 4);
	c->Multiplicands = NULL;
	if (c->LookupType == 0) return 0;
	if (c->LookupType > 2)  return VORBIS_ERR_CODEBOOK_LOOKUP;

	c->MinValue   = float32_unpack(ctx);
	c->DeltaValue = float32_unpack(ctx);
	Int32 valueBits = Vorbis_ReadBits(ctx, 4); valueBits++;
	c->SequenceP  = Vorbis_ReadBits(ctx, 1);

	UInt32 lookupValues;
	if (c->LookupType == 1) {
		lookupValues = lookup1_values(c->Entries, c->Dimensions);
	} else {
		lookupValues = c->Entries * c->Dimensions;
	}
	c->LookupValues = lookupValues;

	c->Multiplicands = Mem_Alloc(lookupValues, sizeof(UInt16), "multiplicands");
	for (i = 0; i < lookupValues; i++) {
		c->Multiplicands[i] = Vorbis_ReadBits(ctx, valueBits);
	}
	return 0;
}

static UInt32 Codebook_DecodeScalar(struct VorbisState* ctx, struct Codebook* c) {
	UInt32 codeword = 0, shift = 31, depth, i;
	/* TODO: This is so massively slow */
	for (depth = 1; depth <= 32; depth++, shift--) {
		UInt32 bit = Vorbis_ReadBits(ctx, 1);
		codeword |= bit << shift;

		for (i = 0; i < c->NumCodewords; i++) {
			if (depth != c->CodewordLens[i]) continue;
			if (codeword != c->Codewords[i]) continue;

			UInt32 value = c->Values[i];
			//Platform_Log2("read value %i of len %i", &value, &depth);
			return value;
		}
	}
	ErrorHandler_Fail("????????????");
	return -1;
}

static void Codebook_DecodeVectors(struct VorbisState* ctx, struct Codebook* c, Real32* v, Int32 step) {
	UInt32 lookupOffset = Codebook_DecodeScalar(ctx, c);
	Real32 last = 0.0f, value;
	UInt32 i, offset;

	if (c->LookupType == 1) {		
		UInt32 indexDivisor = 1;
		for (i = 0; i < c->Dimensions; i++, v += step) {
			offset = (lookupOffset / indexDivisor) % c->LookupValues;
			value  = c->Multiplicands[offset] * c->DeltaValue + c->MinValue + last;

			*v += value;
			if (c->SequenceP) last = value;
			indexDivisor *= c->LookupValues;
		}
	} else if (c->LookupType == 2) {
		offset = lookupOffset * c->Dimensions;
		for (i = 0; i < c->Dimensions; i++, offset++, v += step) {
			value  = c->Multiplicands[offset] * c->DeltaValue + c->MinValue + last;

			*v += value;
			if (c->SequenceP) last = value;
		}
	} else {
		ErrorHandler_Fail("???????");
	}
}

/*########################################################################################################################*
*-----------------------------------------------------Vorbis floors-------------------------------------------------------*
*#########################################################################################################################*/
#define FLOOR_MAX_PARTITIONS 32
#define FLOOR_MAX_CLASSES 16
#define FLOOR_MAX_VALUES (FLOOR_MAX_PARTITIONS * 8 + 2)
struct Floor {
	UInt8 Partitions, Multiplier; Int32 Range, Values;
	UInt8 PartitionClasses[FLOOR_MAX_PARTITIONS];
	UInt8 ClassDimensions[FLOOR_MAX_CLASSES];
	UInt8 ClassSubClasses[FLOOR_MAX_CLASSES];
	UInt8 ClassMasterbooks[FLOOR_MAX_CLASSES];
	Int16 SubclassBooks[FLOOR_MAX_CLASSES][8];
	Int16 XList[FLOOR_MAX_VALUES];
	UInt16 ListOrder[FLOOR_MAX_VALUES];
	Int32 YList[VORBIS_MAX_CHANS][FLOOR_MAX_VALUES];
};

/* TODO: Make this thread safe */
Int16* tmp_xlist;
UInt16* tmp_order;
static void Floor_SortXList(Int32 left, Int32 right) {
	UInt16* values = tmp_order; UInt16 value;
	Int16* keys = tmp_xlist;    Int16 key;
	while (left < right) {
		Int32 i = left, j = right;
		Int16 pivot = keys[(i + j) / 2];

		/* partition the list */
		while (i <= j) {
			while (pivot > keys[i]) i++;
			while (pivot < keys[j]) j--;
			QuickSort_Swap_KV_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(Floor_SortXList)
	}
}

static ReturnCode Floor_DecodeSetup(struct VorbisState* ctx, struct Floor* f) {
	f->Partitions = Vorbis_ReadBits(ctx, 5);
	Int32 i, j, idx, maxClass = -1;

	for (i = 0; i < f->Partitions; i++) {
		f->PartitionClasses[i] = Vorbis_ReadBits(ctx, 4);
		maxClass = max(maxClass, f->PartitionClasses[i]);
	}

	for (i = 0; i <= maxClass; i++) {
		f->ClassDimensions[i] = Vorbis_ReadBits(ctx, 3); f->ClassDimensions[i]++;
		f->ClassSubClasses[i] = Vorbis_ReadBits(ctx, 2);
		if (f->ClassSubClasses[i]) {
			f->ClassMasterbooks[i] = Vorbis_ReadBits(ctx, 8);
		}
		for (j = 0; j < (1 << f->ClassSubClasses[i]); j++) {
			f->SubclassBooks[i][j] = Vorbis_ReadBits(ctx, 8); f->SubclassBooks[i][j]--;
		}
	}

	f->Multiplier = Vorbis_ReadBits(ctx, 2); f->Multiplier++;
	static Int16 ranges[4] = { 256, 128, 84, 64 };
	f->Range = ranges[f->Multiplier - 1];

	UInt32 rangeBits = Vorbis_ReadBits(ctx, 4);
	f->XList[0] = 0;
	f->XList[1] = 1 << rangeBits;

	for (i = 0, idx = 2; i < f->Partitions; i++) {
		Int32 classNum = f->PartitionClasses[i];
		for (j = 0; j < f->ClassDimensions[classNum]; j++) {
			f->XList[idx++] = Vorbis_ReadBits(ctx, rangeBits);
		}
	}
	f->Values = idx;

	/* sort X list for curve computation later */
	Int16 xlist_sorted[FLOOR_MAX_VALUES];
	Mem_Copy(xlist_sorted, f->XList, idx * sizeof(Int16));
	for (i = 0; i < idx; i++) { f->ListOrder[i] = i; }

	tmp_xlist = xlist_sorted; 
	tmp_order = f->ListOrder;
	Floor_SortXList(0, idx - 1);
	return 0;
}

static bool Floor_DecodeFrame(struct VorbisState* ctx, struct Floor* f, Int32 ch) {
	Int32 nonZero = Vorbis_ReadBits(ctx, 1);
	if (!nonZero) return false;
	Int32* yList = f->YList[ch];

	Int32 i, j, idx, rangeBits = iLog(f->Range - 1);
	yList[0] = Vorbis_ReadBits(ctx, rangeBits);
	yList[1] = Vorbis_ReadBits(ctx, rangeBits);

	for (i = 0, idx = 2; i < f->Partitions; i++) {
		UInt8 class = f->PartitionClasses[i];
		UInt8 cdim  = f->ClassDimensions[class];
		UInt8 cbits = f->ClassSubClasses[class];

		UInt32 csub = (1 << cbits) - 1;
		UInt32 cval = 0;
		if (cbits) {
			UInt8 bookNum = f->ClassMasterbooks[class];
			cval = Codebook_DecodeScalar(ctx, &ctx->Codebooks[bookNum]);
		}

		for (j = 0; j < cdim; j++) {
			Int16 bookNum = f->SubclassBooks[class][cval & csub];
			cval >>= cbits;
			if (bookNum >= 0) {
				yList[idx + j] = Codebook_DecodeScalar(ctx, &ctx->Codebooks[bookNum]);
			} else {
				yList[idx + j] = 0;
			}
		}
		idx += cdim;
	}
	return true;
}

static Int32 render_point(Int32 x0, Int32 y0, Int32 x1, Int32 y1, Int32 X) {
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

static Real32 floor1_inverse_dB_table[256];
static void render_line(Int32 x0, Int32 y0, Int32 x1, Int32 y1, Real32* data) {
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
	data[x] *= floor1_inverse_dB_table[y];

	for (x = x0 + 1; x < x1; x++) {
		err = err + ady;
		if (err >= adx) {
			err = err - adx;
			y = y + sy;
		} else {
			y = y + base;
		}
		data[x] *= floor1_inverse_dB_table[y];
	}
}

static Int32 low_neighbor(Int16* v, Int32 x) {
	Int32 n = 0, i, max = Int32_MinValue;
	for (i = 0; i < x; i++) {
		if (v[i] < v[x] && v[i] > max) { n = i; max = v[i]; }
	}
	return n;
}

static Int32 high_neighbor(Int16* v, Int32 x) {
	Int32 n = 0, i, min = Int32_MaxValue;
	for (i = 0; i < x; i++) {
		if (v[i] > v[x] && v[i] < min) { n = i; min = v[i]; }
	}
	return n;
}

static void Floor_Synthesis(struct VorbisState* ctx, struct Floor* f, Int32 ch) {
	/* amplitude value synthesis */
	Int32 YFinal[FLOOR_MAX_VALUES];
	bool Step2[FLOOR_MAX_VALUES];

	Real32* data = ctx->CurOutput[ch];
	Int32* yList = f->YList[ch];

	Step2[0] = true;
	Step2[1] = true;
	YFinal[0] = yList[0];
	YFinal[1] = yList[1];

	Int32 i;
	for (i = 2; i < f->Values; i++) {
		Int32 lo_offset = low_neighbor(f->XList, i);
		Int32 hi_offset = high_neighbor(f->XList, i);

		Int32 predicted = render_point(f->XList[lo_offset], YFinal[lo_offset],
			f->XList[hi_offset], YFinal[hi_offset], f->XList[i]);

		Int32 val = yList[i];
		Int32 highroom = f->Range - predicted;
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

	/* curve synthesis */
	Int32 hx = 0, lx = 0, rawI;
	Int32 ly = YFinal[f->ListOrder[0]] * f->Multiplier, hy = ly;

	for (rawI = 1; rawI < f->Values; rawI++) {
		i = f->ListOrder[rawI];
		if (!Step2[i]) continue;

		hx = f->XList[i]; hy = YFinal[i] * f->Multiplier;
		if (lx < hx) {
			render_line(lx, ly, min(hx, ctx->DataSize), hy, data);
		}
		lx = hx; ly = hy;
	}

	/* fill remainder of floor with a flat line */
	/* TODO: Is this right? should hy be 0, if Step2 is false for all */
	if (hx >= ctx->DataSize) return;
	lx = hx; hx = ctx->DataSize;
	Real32 value = floor1_inverse_dB_table[hy];
	for (; lx < hx; lx++) { data[lx] *= value; }
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

static ReturnCode Residue_DecodeSetup(struct VorbisState* ctx, struct Residue* r, UInt16 type) {
	r->Type  = type;
	r->Begin = Vorbis_ReadBits(ctx, 24);
	r->End   = Vorbis_ReadBits(ctx, 24);
	r->PartitionSize   = Vorbis_ReadBits(ctx, 24); r->PartitionSize++;
	r->Classifications = Vorbis_ReadBits(ctx, 6);  r->Classifications++;
	r->Classbook = Vorbis_ReadBits(ctx, 8);

	Int32 i;
	for (i = 0; i < r->Classifications; i++) {
		r->Cascade[i] = Vorbis_ReadBits(ctx, 3);
		Int32 moreBits = Vorbis_ReadBits(ctx, 1);
		if (!moreBits) continue;
		Int32 bits = Vorbis_ReadBits(ctx, 5);
		r->Cascade[i] |= bits << 3;
	}

	Int32 j;
	for (i = 0; i < r->Classifications; i++) {
		for (j = 0; j < 8; j++) {
			Int16 codebook = -1;
			if (r->Cascade[i] & (1 << j)) {
				codebook = Vorbis_ReadBits(ctx, 8);
			}
			r->Books[i][j] = codebook;
		}
	}
	return 0;
}

static void Residue_DecodeCore(struct VorbisState* ctx, struct Residue* r, UInt32 size, Int32 ch, bool* doNotDecode, Real32** data) {
	UInt32 residueBeg = min(r->Begin, size);
	UInt32 residueEnd = min(r->End,   size);
	Int32 pass, i, j, k;

	struct Codebook* classbook = &ctx->Codebooks[r->Classbook];
	UInt32 classwordsPerCodeword = classbook->Dimensions;
	UInt32 nToRead = residueEnd - residueBeg;
	UInt32 partitionsToRead = nToRead / r->PartitionSize;

	/* first half of temp array is used by residue type 2 for storing temp interleaved data */
	UInt8* classifications_raw = ((UInt8*)ctx->Temp) + (ctx->DataSize * ctx->Channels * 5);
	UInt8* classifications[VORBIS_MAX_CHANS];
	for (i = 0; i < ch; i++) {
		/* add a bit of space in case classwordsPerCodeword is > partitionsToRead*/
		classifications[i] = classifications_raw + i * (partitionsToRead + 64);
	}

	if (nToRead == 0) return;
	for (pass = 0; pass < 8; pass++) {
		UInt32 partitionCount = 0;
		while (partitionCount < partitionsToRead) {

			/* read classifications in pass 0 */
			if (pass == 0) {
				for (j = 0; j < ch; j++) {
					if (doNotDecode[j]) continue;

					UInt32 temp = Codebook_DecodeScalar(ctx, classbook);
					for (i = classwordsPerCodeword - 1; i >= 0; i--) {
						classifications[j][i + partitionCount] = temp % r->Classifications;
						temp /= r->Classifications;
					}
				}
			}

			for (i = 0; i < classwordsPerCodeword && partitionCount < partitionsToRead; i++) {
				for (j = 0; j < ch; j++) {
					if (doNotDecode[j]) continue;
					UInt8 class = classifications[j][partitionCount];
					Int16 book = r->Books[class][pass];
					if (book < 0) continue;

					UInt32 offset = residueBeg + partitionCount * r->PartitionSize;
					Real32* v = data[j] + offset;
					struct Codebook* c = &ctx->Codebooks[book];

					if (r->Type == 0) {
						Int32 step = r->PartitionSize / c->Dimensions;
						for (k = 0; k < step; k++) {
							Codebook_DecodeVectors(ctx, c, v, step); v++;
						}
					} else {
						for (k = 0; k < r->PartitionSize; k += c->Dimensions) {
							Codebook_DecodeVectors(ctx, c, v, 1); v += c->Dimensions;
						}
					}
				}
				partitionCount++;
			}
		}
	}
}

static void Residue_DecodeFrame(struct VorbisState* ctx, struct Residue* r, Int32 ch, bool* doNotDecode, Real32** data) {
	UInt32 size = ctx->DataSize;
	if (r->Type == 2) {
		bool decodeAny = false;
		Int32 i, j;

		/* type 2 decodes all channel vectors, if at least 1 channel to decode */
		for (i = 0; i < ch; i++) {
			if (!doNotDecode[i]) decodeAny = true;
		}
		if (!decodeAny) return;
		decodeAny = false; /* because DecodeCore expects this to be 'false' for 'do not decode' */

		Real32* interleaved = ctx->Temp;
		/* TODO: avoid using ctx->temp and deinterleaving at all */
		/* TODO: avoid setting memory to 0 here */
		Mem_Set(interleaved, 0, ctx->DataSize * ctx->Channels * sizeof(Real32));
		Residue_DecodeCore(ctx, r, size * ch, 1, &decodeAny, &interleaved);

		/* deinterleave type 2 output */	
		for (i = 0; i < size; i++) {
			for (j = 0; j < ch; j++) {
				data[j][i] = interleaved[i * ch + j];
			}
		}
	} else {
		Residue_DecodeCore(ctx, r, size, ch, doNotDecode, data);
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

static ReturnCode Mapping_DecodeSetup(struct VorbisState* ctx, struct Mapping* m) {
	Int32 i, submaps = 1, submapFlag = Vorbis_ReadBits(ctx, 1);
	if (submapFlag) {
		submaps = Vorbis_ReadBits(ctx, 4); submaps++;
	}

	Int32 couplingSteps = 0, couplingFlag = Vorbis_ReadBits(ctx, 1);
	if (couplingFlag) {
		couplingSteps = Vorbis_ReadBits(ctx, 8); couplingSteps++;
		/* TODO: How big can couplingSteps ever really get in practice? */
		Int32 couplingBits = iLog(ctx->Channels - 1);
		for (i = 0; i < couplingSteps; i++) {
			m->Magnitude[i] = Vorbis_ReadBits(ctx, couplingBits);
			m->Angle[i]     = Vorbis_ReadBits(ctx, couplingBits);
			if (m->Magnitude[i] == m->Angle[i]) return VORBIS_ERR_MAPPING_CHANS;
		}
	}

	Int32 reserved = Vorbis_ReadBits(ctx, 2);
	if (reserved != 0) return VORBIS_ERR_MAPPING_RESERVED;
	m->Submaps = submaps;
	m->CouplingSteps = couplingSteps;

	if (submaps > 1) {
		for (i = 0; i < ctx->Channels; i++) {
			m->Mux[i] = Vorbis_ReadBits(ctx, 4);
		}
	} else {
		for (i = 0; i < ctx->Channels; i++) {
			m->Mux[i] = 0;
		}
	}

	for (i = 0; i < submaps; i++) {
		Vorbis_ReadBits(ctx, 8); /* time value */
		m->FloorIdx[i]   = Vorbis_ReadBits(ctx, 8);
		m->ResidueIdx[i] = Vorbis_ReadBits(ctx, 8);
	}
	return 0;
}


/*########################################################################################################################*
*------------------------------------------------------imdct impl---------------------------------------------------------*
*#########################################################################################################################*/
#define PI MATH_PI
void imdct_slow(Real32* in, Real32* out, Int32 N) {
	Int32 i, k;

	for (i = 0; i < 2 * N; i++) {
		Real64 sum = 0;
		for (k = 0; k < N; k++) {
			sum += in[k] * Math_Cos((PI / N) * (i + 0.5 + N * 0.5) * (k + 0.5));
		}
		out[i] = sum;
	}
}

static UInt32 Vorbis_ReverseBits(UInt32 v) {
	v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
	v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
	v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
	v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
	v = (v >> 16) | (v << 16);
	return v;
}

static UInt32 Vorbis_Log2(UInt32 v) {
	UInt32 r = 0;
	while (v >>= 1) r++;
	return r;
}

void imdct_init(struct imdct_state* state, Int32 n) {
	Int32 k, k2, n4 = n >> 2, n8 = n >> 3, log2_n = Vorbis_Log2(n);
	Real32 *A = state->A, *B = state->B, *C = state->C;
	state->n = n; state->log2_n = log2_n;

	/* setup twiddle factors */
	for (k = 0, k2 = 0; k < n4; k++, k2 += 2) {
		A[k2]   =  Math_Cos((4*k * PI) / n);
		A[k2+1] = -Math_Sin((4*k * PI) / n);
		B[k2]   =  Math_Cos(((k2+1) * PI) / (2*n));
		B[k2+1] =  Math_Sin(((k2+1) * PI) / (2*n));
	}
	for (k = 0, k2 = 0; k < n8; k++, k2 += 2) {
		C[k2]   =  Math_Cos(((k2+1) * (2*PI)) / n);
		C[k2+1] = -Math_Sin(((k2+1) * (2*PI)) / n);
	}

	UInt32* reversed = state->Reversed;
	for (k = 0; k < n8; k++) {
		reversed[k] = Vorbis_ReverseBits(k) >> (32-log2_n+3);
	}
}

void imdct_calc(Real32* in, Real32* out, struct imdct_state* state) {
	Int32 k, k2, k4, k8, n = state->n;
	Int32 n2 = n >> 1, n4 = n >> 2, n8 = n >> 3, n3_4 = n - n4;
	
	/* Optimised algorithm from "The use of multirate filter banks for coding of high quality digital audio" */
	/* Uses a few fixes for the paper noted at http://www.nothings.org/stb_vorbis/mdct_01.txt */
	Real32 *A = state->A, *B = state->B, *C = state->C;

	Real32 u[VORBIS_MAX_BLOCK_SIZE];
	Real32 w[VORBIS_MAX_BLOCK_SIZE];

	/* spectral coefficients, step 1, step 2 */
	for (k = 0, k2 = 0, k4 = 0; k < n8; k++, k2 += 2, k4 += 4) {
		Real32 e_1 = -in[k4+3], e_2 = -in[k4+1];
		Real32 g_1 = e_1 * A[n2-1-k2] + e_2 * A[n2-2-k2];
		Real32 g_2 = e_1 * A[n2-2-k2] - e_2 * A[n2-1-k2];

		Real32 f_1 = in[n2-4-k4], f_2 = in[n2-2-k4];
		Real32 h_2 = f_1 * A[n4-2-k2] - f_2 * A[n4-1-k2];
		Real32 h_1 = f_1 * A[n4-1-k2] + f_2 * A[n4-2-k2];

		w[n2+3+k4] = h_2 + g_2;
		w[n2+1+k4] = h_1 + g_1;

		w[k4+3] = (h_2 - g_2) * A[n2-4-k4] - (h_1 - g_1) * A[n2-3-k4];
		w[k4+1] = (h_1 - g_1) * A[n2-4-k4] + (h_2 - g_2) * A[n2-3-k4];
	}

	/* step 3 */
	Int32 l, log2_n = state->log2_n;
	for (l = 0; l <= log2_n - 4; l++) {
		Int32 k0 = n >> (l+2), k1 = 1 << (l+3);
		Int32 r, r4, rMax = n >> (l+4), s2, s2Max = 1 << (l+2);

		for (r = 0, r4 = 0; r < rMax; r++, r4 += 4) {
			for (s2 = 0; s2 < s2Max; s2 += 2) {
				Real32 e_1 = w[n-1-k0*s2-r4],     e_2 = w[n-3-k0*s2-r4];
				Real32 f_1 = w[n-1-k0*(s2+1)-r4], f_2 = w[n-3-k0*(s2+1)-r4];

				u[n-1-k0*s2-r4] = e_1 + f_1;
				u[n-3-k0*s2-r4] = e_2 + f_2;

				u[n-1-k0*(s2+1)-r4] = (e_1 - f_1) * A[r*k1] - (e_2 - f_2) * A[r*k1+1];
				u[n-3-k0*(s2+1)-r4] = (e_2 - f_2) * A[r*k1] + (e_1 - f_1) * A[r*k1+1];
			}
		}

		/* TODO: eliminate this, do w/u in-place */
		/* TODO: dynamically allocate mem for imdct */
		if (l+1 <= log2_n - 4) {
			Mem_Copy(w, u, sizeof(u));
		}
	}

	/* step 4, step 5, step 6, step 7, step 8, output */
	UInt32* reversed = state->Reversed;
	for (k = 0, k2 = 0, k8 = 0; k < n8; k++, k2 += 2, k8 += 8) {
		UInt32 j = reversed[k], j8 = j << 3;
		Real32 e_1 = u[n-j8-1], e_2 = u[n-j8-3];
		Real32 f_1 = u[j8+3],   f_2 = u[j8+1];

		Real32 g_1 =  e_1 + f_1 + C[k2+1] * (e_1 - f_1) + C[k2] * (e_2 + f_2);
		Real32 h_1 =  e_1 + f_1 - C[k2+1] * (e_1 - f_1) - C[k2] * (e_2 + f_2);
		Real32 g_2 =  e_2 - f_2 + C[k2+1] * (e_2 + f_2) - C[k2] * (e_1 - f_1);
		Real32 h_2 = -e_2 + f_2 + C[k2+1] * (e_2 + f_2) - C[k2] * (e_1 - f_1);

		Real32 x_1 = -0.5f * (g_1 * B[k2]   + g_2 * B[k2+1]);
		Real32 x_2 = -0.5f * (g_1 * B[k2+1] - g_2 * B[k2]);
		out[n4-1-k]   = -x_2;
		out[n4+k]     =  x_2;
		out[n3_4-1-k] =  x_1;
		out[n3_4+k]   =  x_1;

		Real32 y_1 = -0.5f * (h_1 * B[n2-2-k2] + h_2 * B[n2-1-k2]);
		Real32 y_2 = -0.5f * (h_1 * B[n2-1-k2] - h_2 * B[n2-2-k2]);
		out[k]      = -y_2;
		out[n2-1-k] =  y_2;
		out[n2+k]   =  y_1;
		out[n-1-k]  =  y_1;
	}
}


/*########################################################################################################################*
*-----------------------------------------------------Vorbis setup--------------------------------------------------------*
*#########################################################################################################################*/
struct Mode { UInt8 BlockSizeFlag, MappingIdx; };
static ReturnCode Mode_DecodeSetup(struct VorbisState* ctx, struct Mode* m) {
	m->BlockSizeFlag  = Vorbis_ReadBits(ctx, 1);
	UInt16 windowType = Vorbis_ReadBits(ctx, 16);
	if (windowType != 0) return VORBIS_ERR_MODE_WINDOW;

	UInt16 transformType = Vorbis_ReadBits(ctx, 16);
	if (transformType != 0) return VORBIS_ERR_MODE_TRANSFORM;
	m->MappingIdx = Vorbis_ReadBits(ctx, 8);
	return 0;
}

static void Vorbis_CalcWindow(struct VorbisState* ctx, UInt32* offset, bool longBlock, bool prevLong, bool nextLong) {
	Int32 i, n = ctx->BlockSizes[longBlock];
	Int32 window_center = n / 2;
	Int32 left_window_beg,  left_window_end,  left_n;
	Int32 right_window_beg, right_window_end, right_n;

	if (longBlock && !prevLong) {
		left_window_beg = n / 4 - ctx->BlockSizes[0] / 4;
		left_window_end = n / 4 + ctx->BlockSizes[0] / 4;
		left_n = ctx->BlockSizes[0] / 2;
	} else {
		left_window_beg = 0;
		left_window_end = window_center;
		left_n = n / 2;
	}

	if (longBlock && !nextLong) {
		right_window_beg = (n*3) / 4 - ctx->BlockSizes[0] / 4;
		right_window_end = (n*3) / 4 + ctx->BlockSizes[0] / 4;
		right_n = ctx->BlockSizes[0] / 2;
	} else {
		right_window_beg = window_center;
		right_window_end = n;
		right_n = n / 2;
	}

	Real32* window = ctx->WindowShort + *offset; (*offset) += n;

	for (i = 0; i < left_window_beg; i++) window[i] = 0;
	for (i = left_window_beg; i < left_window_end; i++) {
		Real64 inner = Math_Sin((i - left_window_beg + 0.5) / left_n * (PI/2));
		window[i] = Math_Sin((PI/2) * inner * inner);
	}
	for (i = left_window_end; i < right_window_beg; i++) window[i] = 1;
	for (i = right_window_beg; i < right_window_end; i++) {
		Real64 inner = Math_Sin((i - right_window_beg + 0.5) / right_n * (PI/2) + (PI/2));
		window[i] = Math_Sin((PI/2) * inner * inner);
	}
	for (i = right_window_end; i < n; i++) window[i] = 0;
}

void Vorbis_Free(struct VorbisState* ctx) {
	Int32 i;
	for (i = 0; i < ctx->NumCodebooks; i++) {
		Codebook_Free(&ctx->Codebooks[i]);
	}

	Mem_Free(&ctx->Codebooks);
	Mem_Free(&ctx->Floors);
	Mem_Free(&ctx->Residues);
	Mem_Free(&ctx->Mappings);
	Mem_Free(&ctx->Modes);
	Mem_Free(&ctx->WindowShort);

	Mem_Free(&ctx->Temp);
	ctx->Values[0] = NULL;
	ctx->Values[1] = NULL;
}

static bool Vorbis_ValidBlockSize(UInt32 size) {
	return size >= 64 && size <= VORBIS_MAX_BLOCK_SIZE && Math_IsPowOf2(size);
}

static ReturnCode Vorbis_DecodeHeader(struct VorbisState* ctx, UInt8 type) {
	UInt8 header[7];
	ReturnCode result = Stream_TryRead(ctx->Source, header, sizeof(header));
	if (result) return result;
	if (header[0] != type) return VORBIS_ERR_WRONG_HEADER;

	bool OK = 
		header[1] == 'v' && header[2] == 'o' && header[3] == 'r' &&
		header[4] == 'b' && header[5] == 'i' && header[6] == 's';
	return OK ? 0 : ReturnCode_InvalidArg;
}

static ReturnCode Vorbis_DecodeIdentifier(struct VorbisState* ctx) {
	UInt8 header[23];
	ReturnCode result = Stream_TryRead(ctx->Source, header, sizeof(header));
	if (result) return result;

	UInt32 version  = Stream_GetU32_LE(&header[0]);
	if (version != 0) return VORBIS_ERR_VERSION;

	ctx->Channels   = header[4];
	ctx->SampleRate = Stream_GetU32_LE(&header[5]);
	/* (12) bitrate_maximum, nominal, minimum */
	ctx->BlockSizes[0] = 1 << (header[21] & 0xF);
	ctx->BlockSizes[1] = 1 << (header[21] >>  4);

	if (!Vorbis_ValidBlockSize(ctx->BlockSizes[0])) return VORBIS_ERR_BLOCKSIZE;
	if (!Vorbis_ValidBlockSize(ctx->BlockSizes[1])) return VORBIS_ERR_BLOCKSIZE;
	if (ctx->BlockSizes[0] > ctx->BlockSizes[1])    return VORBIS_ERR_BLOCKSIZE;

	if (ctx->Channels == 0 || ctx->Channels > VORBIS_MAX_CHANS) return VORBIS_ERR_CHANS;
	/* check framing flag */
	return (header[22] & 1) ? 0 : VORBIS_ERR_FRAMING;
}

static ReturnCode Vorbis_DecodeComments(struct VorbisState* ctx) {
	UInt32 vendorLen = Stream_ReadU32_LE(ctx->Source);
	Stream_Skip(ctx->Source, vendorLen);

	UInt32 i, comments = Stream_ReadU32_LE(ctx->Source);
	for (i = 0; i < comments; i++) {
		UInt32 commentLen = Stream_ReadU32_LE(ctx->Source);
		Stream_Skip(ctx->Source, commentLen);
	}

	/* check framing flag */
	return (Stream_ReadU8(ctx->Source) & 1) ? 0 : VORBIS_ERR_FRAMING;
}

static ReturnCode Vorbis_DecodeSetup(struct VorbisState* ctx) {
	Int32 i, count;
	ReturnCode result;

	count = Vorbis_ReadBits(ctx, 8); count++;
	ctx->Codebooks = Mem_Alloc(count, sizeof(struct Codebook), "vorbis codebooks");
	for (i = 0; i < count; i++) {
		result = Codebook_DecodeSetup(ctx, &ctx->Codebooks[i]);
		if (result) return result;
	}
	ctx->NumCodebooks = count;

	count = Vorbis_ReadBits(ctx, 6); count++;
	for (i = 0; i < count; i++) {
		UInt16 time = Vorbis_ReadBits(ctx, 16);
		if (time != 0) return VORBIS_ERR_TIME_TYPE;
	}

	count = Vorbis_ReadBits(ctx, 6); count++;
	ctx->Floors = Mem_Alloc(count, sizeof(struct Floor), "vorbis floors");
	for (i = 0; i < count; i++) {
		UInt16 floor = Vorbis_ReadBits(ctx, 16);
		if (floor != 1) return VORBIS_ERR_FLOOR_TYPE;
		result = Floor_DecodeSetup(ctx, &ctx->Floors[i]);
		if (result) return result;
	}

	count = Vorbis_ReadBits(ctx, 6); count++;
	ctx->Residues = Mem_Alloc(count, sizeof(struct Residue), "vorbis residues");
	for (i = 0; i < count; i++) {
		UInt16 residue = Vorbis_ReadBits(ctx, 16);
		if (residue > 2) return VORBIS_ERR_FLOOR_TYPE;
		result = Residue_DecodeSetup(ctx, &ctx->Residues[i], residue);
		if (result) return result;
	}

	count = Vorbis_ReadBits(ctx, 6); count++;
	ctx->Mappings = Mem_Alloc(count, sizeof(struct Mapping), "vorbis mappings");
	for (i = 0; i < count; i++) {
		UInt16 mapping = Vorbis_ReadBits(ctx, 16);
		if (mapping != 0) return VORBIS_ERR_MAPPING_TYPE;
		result = Mapping_DecodeSetup(ctx, &ctx->Mappings[i]);
		if (result) return result;
	}

	count = Vorbis_ReadBits(ctx, 6); count++;
	ctx->Modes = Mem_Alloc(count, sizeof(struct Mode), "vorbis modes");
	for (i = 0; i < count; i++) {
		result = Mode_DecodeSetup(ctx, &ctx->Modes[i]);
		if (result) return result;
	}
	
	ctx->ModeNumBits = iLog(count - 1); /* ilog([vorbis_mode_count]-1) bits */
	UInt8 framing = Vorbis_ReadBits(ctx, 1);
	Vorbis_AlignBits(ctx);
	/* check framing flag */
	return (framing & 1) ? 0 : VORBIS_ERR_FRAMING;
}

enum VORBIS_PACKET { VORBIS_IDENTIFIER = 1, VORBIS_COMMENTS = 3, VORBIS_SETUP = 5, };
ReturnCode Vorbis_DecodeHeaders(struct VorbisState* ctx) {
	ReturnCode result;
	
	result = Vorbis_DecodeHeader(ctx, VORBIS_IDENTIFIER);
	if (result) return result;
	result = Vorbis_DecodeIdentifier(ctx);
	if (result) return result;

	result = Vorbis_DecodeHeader(ctx, VORBIS_COMMENTS);
	if (result) return result;
	result = Vorbis_DecodeComments(ctx);
	if (result) return result;

	result = Vorbis_DecodeHeader(ctx, VORBIS_SETUP);
	if (result) return result;
	result = Vorbis_DecodeSetup(ctx);
	if (result) return result;

	/* window calculations can be pre-computed here */
	UInt32 count = ctx->BlockSizes[0] + ctx->BlockSizes[1] * 4, offset = 0;
	ctx->WindowShort = Mem_Alloc(count, sizeof(Real32), "Vorbis windows");
	Vorbis_CalcWindow(ctx, &offset, false, false, false);

	ctx->WindowLong[0][0] = ctx->WindowShort + offset;
	Vorbis_CalcWindow(ctx, &offset, true, false, false);
	ctx->WindowLong[1][0] = ctx->WindowShort + offset;
	Vorbis_CalcWindow(ctx, &offset, true, false, true);
	ctx->WindowLong[0][1] = ctx->WindowShort + offset;
	Vorbis_CalcWindow(ctx, &offset, true, true,  false);
	ctx->WindowLong[1][1] = ctx->WindowShort + offset;
	Vorbis_CalcWindow(ctx, &offset, true, true,  true);

	count = ctx->Channels * ctx->BlockSizes[1];
	ctx->Temp      = Mem_AllocCleared(count * 3, sizeof(Real32), "Vorbis values");
	ctx->Values[0] = ctx->Temp + count;
	ctx->Values[1] = ctx->Temp + count * 2;

	imdct_init(&ctx->imdct[0], ctx->BlockSizes[0]);
	imdct_init(&ctx->imdct[1], ctx->BlockSizes[1]);
	return 0;
}


/*########################################################################################################################*
*-----------------------------------------------------Vorbis frame--------------------------------------------------------*
*#########################################################################################################################*/
ReturnCode Vorbis_DecodeFrame(struct VorbisState* ctx) {
	Int32 i, j, packetType;
	ReturnCode result = Vorbis_TryReadBits(ctx, 1, &packetType);
	if (result) return result;
	if (packetType) return VORBIS_ERR_FRAME_TYPE;

	Int32 modeIdx = Vorbis_ReadBits(ctx, ctx->ModeNumBits);
	struct Mode* mode = &ctx->Modes[modeIdx];
	struct Mapping* mapping = &ctx->Mappings[mode->MappingIdx];

	/* decode window shape */
	ctx->CurBlockSize = ctx->BlockSizes[mode->BlockSizeFlag];
	ctx->DataSize = ctx->CurBlockSize / 2;
	Int32 prev_window = 0, next_window = 0;
	/* long window lapping*/
	if (mode->BlockSizeFlag) {
		prev_window = Vorbis_ReadBits(ctx, 1);
		next_window = Vorbis_ReadBits(ctx, 1);
	}	

	/* swap prev and cur outputs around */
	Real32* tmp = ctx->Values[1]; ctx->Values[1] = ctx->Values[0]; ctx->Values[0] = tmp;
	Mem_Set(ctx->Values[0], 0, ctx->Channels * ctx->CurBlockSize);

	for (i = 0; i < ctx->Channels; i++) {
		ctx->CurOutput[i]  = ctx->Values[0] + i * ctx->CurBlockSize;
		ctx->PrevOutput[i] = ctx->Values[1] + i * ctx->PrevBlockSize;
	}

	/* decode floor */
	bool hasFloor[VORBIS_MAX_CHANS], hasResidue[VORBIS_MAX_CHANS];
	for (i = 0; i < ctx->Channels; i++) {
		Int32 submap = mapping->Mux[i];
		Int32 floorIdx = mapping->FloorIdx[submap];
		hasFloor[i] = Floor_DecodeFrame(ctx, &ctx->Floors[floorIdx], i);
		hasResidue[i] = hasFloor[i];
	}

	/* non-zero vector propogate */
	for (i = 0; i < mapping->CouplingSteps; i++) {
		Int32 magChannel = mapping->Magnitude[i];
		Int32 angChannel = mapping->Angle[i];

		if (hasResidue[magChannel] || hasResidue[angChannel]) {
			hasResidue[magChannel] = true; hasResidue[angChannel] = true;
		}
	}

	/* decode residue */
	for (i = 0; i < mapping->Submaps; i++) {
		Int32 ch = 0;
		bool doNotDecode[VORBIS_MAX_CHANS];
		Real32* data[VORBIS_MAX_CHANS]; /* map residue data to actual channel data */
		for (j = 0; j < ctx->Channels; j++) {
			if (mapping->Mux[j] != i) continue;

			doNotDecode[ch] = !hasResidue[j];
			data[ch] = ctx->CurOutput[j];
			ch++;
		}

		Int32 residueIdx = mapping->FloorIdx[i];
		Residue_DecodeFrame(ctx, &ctx->Residues[residueIdx], ch, doNotDecode, data);
	}

	/* inverse coupling */
	for (i = mapping->CouplingSteps - 1; i >= 0; i--) {
		Real32* magValues = ctx->CurOutput[mapping->Magnitude[i]];
		Real32* angValues = ctx->CurOutput[mapping->Angle[i]];

		for (j = 0; j < ctx->DataSize; j++) {
			Real32 m = magValues[j], a = angValues[j];

			if (m > 0.0f) {
				if (a > 0.0f) { angValues[j] = m - a; }
				else {
					angValues[j] = m;
					magValues[j] = m + a;
				}
			} else {
				if (a > 0.0f) { angValues[j] = m + a; }
				else {
					angValues[j] = m;
					magValues[j] = m - a;
				}
			}
		}
	}

	/* compute dot product of floor and residue, producing audio spectrum vector */
	for (i = 0; i < ctx->Channels; i++) {
		if (!hasFloor[i]) continue;
		Int32 submap = mapping->Mux[i];
		Int32 floorIdx = mapping->FloorIdx[submap];
		Floor_Synthesis(ctx, &ctx->Floors[floorIdx], i);
	}

	Int32 n = ctx->CurBlockSize;
	Real32* window;
	if (mode->BlockSizeFlag) {
		window = ctx->WindowLong[prev_window][next_window];
	} else {
		window = ctx->WindowShort;
	}

	/* inverse monolithic transform of audio spectrum vector */
	for (i = 0; i < ctx->Channels; i++) {
		Real32* data = ctx->CurOutput[i];
		if (!hasFloor[i]) {
			/* TODO: Do we actually need to zero data here (residue type 2 maybe) */
			Mem_Set(data, 0, ctx->CurBlockSize * sizeof(Real32));
		} else {
			imdct_calc(data, data, &ctx->imdct[mode->BlockSizeFlag]);
			for (j = 0; j < n; j++) { data[j] *= window[j]; }
		}
	}

	/* discard remaining bits at end of packet */
	Vorbis_AlignBits(ctx);
	return 0;
}

Int32 Vorbis_OutputFrame(struct VorbisState* ctx, Int16* data) {
	Int32 i, ch;
	/* first frame decoded has no data */
	if (ctx->PrevBlockSize == 0) {
		ctx->PrevBlockSize = ctx->CurBlockSize;
		return 0;
	}

	Int32 prevQrtr = ctx->PrevBlockSize / 4, curQrtr = ctx->CurBlockSize / 4;
	Int32 prevHalf = prevQrtr * 2, curHalf = curQrtr * 2;
	
	Real32* prev[VORBIS_MAX_CHANS];
	Real32*  cur[VORBIS_MAX_CHANS];
	Int32 offset = curHalf - prevHalf; offset = max(offset, 0);

	for (i = 0; i < ctx->Channels; i++) {
		prev[i] = ctx->PrevOutput[i] + prevHalf; /* data starts at halfway in previous block */
		cur[i]   = ctx->CurOutput[i] + offset;   /* for when current block extends past prev block */
	}

	/* So for example, consider a short block overlapping with a long block
	   a) we need to chop off 'prev' before its halfway point
	   b) then need to chop off the 'cur' before the halfway point of 'prev'
	             |-   ********|*****                     |-   ********|
	            -| - *        |     ***                  | - *        |
	           - |  #         |        ***         ===>  |  #         |
	          -  | * -        |           ***            | * -        |
	   ******-***|*   -       |              ***         |*   -       |
	*/


	/* data returned is from centre of previous block to centre of current block */
	/* 3/4 point of previous block is aligned to 1/4 of current block */
	Int32 overlapBeg = prevQrtr - curQrtr; overlapBeg = max(overlapBeg, 0);
	Int32 overlapEnd = prevQrtr + curQrtr; overlapEnd = min(overlapEnd, curHalf);

	for (i = 0; i < overlapBeg; i++) {
		for (ch = 0; ch < ctx->Channels; ch++) {
			Real32 sample = prev[ch][i];
			Math_Clamp(sample, -1.0f, 1.0f);
			*data++ = (Int16)(sample * 32767);
		}
	}

	/* overlap and add data */
	for (i = overlapBeg; i < overlapEnd; i++) {
		for (ch = 0; ch < ctx->Channels; ch++) {
			Real32 sample = prev[ch][i] + cur[ch][i];
			Math_Clamp(sample, -1.0f, 1.0f);
			*data++ = (Int16)(sample * 32767);
		}
	}

	for (i = overlapEnd; i < curHalf; i++) {
		for (ch = 0; ch < ctx->Channels; ch++) {
			Real32 sample = cur[ch][i];
			Math_Clamp(sample, -1.0f, 1.0f);
			*data++ = (Int16)(sample * 32767);
		}
	}

	ctx->PrevBlockSize = ctx->CurBlockSize;
	return prevQrtr + curQrtr;
}

static Real32 floor1_inverse_dB_table[256] = {
	1.0649863e-07f, 1.1341951e-07f, 1.2079015e-07f, 1.2863978e-07f,
	1.3699951e-07f, 1.4590251e-07f, 1.5538408e-07f, 1.6548181e-07f,
	1.7623575e-07f, 1.8768855e-07f, 1.9988561e-07f, 2.1287530e-07f,
	2.2670913e-07f, 2.4144197e-07f, 2.5713223e-07f, 2.7384213e-07f,
	2.9163793e-07f, 3.1059021e-07f, 3.3077411e-07f, 3.5226968e-07f,
	3.7516214e-07f, 3.9954229e-07f, 4.2550680e-07f, 4.5315863e-07f,
	4.8260743e-07f, 5.1396998e-07f, 5.4737065e-07f, 5.8294187e-07f,
	6.2082472e-07f, 6.6116941e-07f, 7.0413592e-07f, 7.4989464e-07f,
	7.9862701e-07f, 8.5052630e-07f, 9.0579828e-07f, 9.6466216e-07f,
	1.0273513e-06f, 1.0941144e-06f, 1.1652161e-06f, 1.2409384e-06f,
	1.3215816e-06f, 1.4074654e-06f, 1.4989305e-06f, 1.5963394e-06f,
	1.7000785e-06f, 1.8105592e-06f, 1.9282195e-06f, 2.0535261e-06f,
	2.1869758e-06f, 2.3290978e-06f, 2.4804557e-06f, 2.6416497e-06f,
	2.8133190e-06f, 2.9961443e-06f, 3.1908506e-06f, 3.3982101e-06f,
	3.6190449e-06f, 3.8542308e-06f, 4.1047004e-06f, 4.3714470e-06f,
	4.6555282e-06f, 4.9580707e-06f, 5.2802740e-06f, 5.6234160e-06f,
	5.9888572e-06f, 6.3780469e-06f, 6.7925283e-06f, 7.2339451e-06f,
	7.7040476e-06f, 8.2047000e-06f, 8.7378876e-06f, 9.3057248e-06f,
	9.9104632e-06f, 1.0554501e-05f, 1.1240392e-05f, 1.1970856e-05f,
	1.2748789e-05f, 1.3577278e-05f, 1.4459606e-05f, 1.5399272e-05f,
	1.6400004e-05f, 1.7465768e-05f, 1.8600792e-05f, 1.9809576e-05f,
	2.1096914e-05f, 2.2467911e-05f, 2.3928002e-05f, 2.5482978e-05f,
	2.7139006e-05f, 2.8902651e-05f, 3.0780908e-05f, 3.2781225e-05f,
	3.4911534e-05f, 3.7180282e-05f, 3.9596466e-05f, 4.2169667e-05f,
	4.4910090e-05f, 4.7828601e-05f, 5.0936773e-05f, 5.4246931e-05f,
	5.7772202e-05f, 6.1526565e-05f, 6.5524908e-05f, 6.9783085e-05f,
	7.4317983e-05f, 7.9147585e-05f, 8.4291040e-05f, 8.9768747e-05f,
	9.5602426e-05f, 0.00010181521f, 0.00010843174f, 0.00011547824f,
	0.00012298267f, 0.00013097477f, 0.00013948625f, 0.00014855085f,
	0.00015820453f, 0.00016848555f, 0.00017943469f, 0.00019109536f,
	0.00020351382f, 0.00021673929f, 0.00023082423f, 0.00024582449f,
	0.00026179955f, 0.00027881276f, 0.00029693158f, 0.00031622787f,
	0.00033677814f, 0.00035866388f, 0.00038197188f, 0.00040679456f,
	0.00043323036f, 0.00046138411f, 0.00049136745f, 0.00052329927f,
	0.00055730621f, 0.00059352311f, 0.00063209358f, 0.00067317058f,
	0.00071691700f, 0.00076350630f, 0.00081312324f, 0.00086596457f,
	0.00092223983f, 0.00098217216f, 0.0010459992f,  0.0011139742f,
	0.0011863665f,  0.0012634633f,  0.0013455702f,  0.0014330129f,
	0.0015261382f,  0.0016253153f,  0.0017309374f,  0.0018434235f,
	0.0019632195f,  0.0020908006f,  0.0022266726f,  0.0023713743f,
	0.0025254795f,  0.0026895994f,  0.0028643847f,  0.0030505286f,
	0.0032487691f,  0.0034598925f,  0.0036847358f,  0.0039241906f,
	0.0041792066f,  0.0044507950f,  0.0047400328f,  0.0050480668f,
	0.0053761186f,  0.0057254891f,  0.0060975636f,  0.0064938176f,
	0.0069158225f,  0.0073652516f,  0.0078438871f,  0.0083536271f,
	0.0088964928f,  0.009474637f,   0.010090352f,   0.010746080f,
	0.011444421f,   0.012188144f,   0.012980198f,   0.013823725f,
	0.014722068f,   0.015678791f,   0.016697687f,   0.017782797f,
	0.018938423f,   0.020169149f,   0.021479854f,   0.022875735f,
	0.024362330f,   0.025945531f,   0.027631618f,   0.029427276f,
	0.031339626f,   0.033376252f,   0.035545228f,   0.037855157f,
	0.040315199f,   0.042935108f,   0.045725273f,   0.048696758f,
	0.051861348f,   0.055231591f,   0.058820850f,   0.062643361f,
	0.066714279f,   0.071049749f,   0.075666962f,   0.080584227f,
	0.085821044f,   0.091398179f,   0.097337747f,   0.10366330f,
	0.11039993f,    0.11757434f,    0.12521498f,    0.13335215f,
	0.14201813f,    0.15124727f,    0.16107617f,    0.17154380f,
	0.18269168f,    0.19456402f,    0.20720788f,    0.22067342f,
	0.23501402f,    0.25028656f,    0.26655159f,    0.28387361f,
	0.30232132f,    0.32196786f,    0.34289114f,    0.36517414f,
	0.38890521f,    0.41417847f,    0.44109412f,    0.46975890f,
	0.50028648f,    0.53279791f,    0.56742212f,    0.60429640f,
	0.64356699f,    0.68538959f,    0.72993007f,    0.77736504f,
	0.82788260f,    0.88168307f,    0.9389798f,     1.00000000f,
};
