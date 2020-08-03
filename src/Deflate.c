#include "Deflate.h"
#include "Logger.h"
#include "Funcs.h"
#include "Platform.h"
#include "Stream.h"
#include "Errors.h"
#include "Utils.h"

#define Header_ReadU8(value) if ((res = s->ReadU8(s, &value))) return res;
/*########################################################################################################################*
*-------------------------------------------------------GZip header-------------------------------------------------------*
*#########################################################################################################################*/
enum GzipState {
	GZIP_STATE_HEADER1, GZIP_STATE_HEADER2, GZIP_STATE_COMPRESSIONMETHOD, GZIP_STATE_FLAGS,
	GZIP_STATE_LASTMODIFIED, GZIP_STATE_COMPRESSIONFLAGS, GZIP_STATE_OPERATINGSYSTEM, 
	GZIP_STATE_HEADERCHECKSUM, GZIP_STATE_FILENAME, GZIP_STATE_COMMENT, GZIP_STATE_DONE
};

void GZipHeader_Init(struct GZipHeader* header) {
	header->state = GZIP_STATE_HEADER1;
	header->done  = false;
	header->flags = 0;
	header->partsRead = 0;
}

cc_result GZipHeader_Read(struct Stream* s, struct GZipHeader* header) {
	cc_uint8 tmp;
	cc_result res;
	switch (header->state) {

	case GZIP_STATE_HEADER1:
		Header_ReadU8(tmp);
		if (tmp != 0x1F) return GZIP_ERR_HEADER1;
		header->state++;

	case GZIP_STATE_HEADER2:
		Header_ReadU8(tmp);
		if (tmp != 0x8B) return GZIP_ERR_HEADER2;
		header->state++;

	case GZIP_STATE_COMPRESSIONMETHOD:
		Header_ReadU8(tmp);
		if (tmp != 0x08) return GZIP_ERR_METHOD;
		header->state++;

	case GZIP_STATE_FLAGS:
		Header_ReadU8(tmp);
		header->flags = tmp;
		if (header->flags & 0x04) return GZIP_ERR_FLAGS;
		header->state++;

	case GZIP_STATE_LASTMODIFIED:
		for (; header->partsRead < 4; header->partsRead++) {
			Header_ReadU8(tmp);
		}
		header->state++;
		header->partsRead = 0;

	case GZIP_STATE_COMPRESSIONFLAGS:
		Header_ReadU8(tmp);
		header->state++;

	case GZIP_STATE_OPERATINGSYSTEM:
		Header_ReadU8(tmp);
		header->state++;

	case GZIP_STATE_FILENAME:
		if (header->flags & 0x08) {
			for (; ;) {
				Header_ReadU8(tmp);
				if (tmp == '\0') break;
			}
		}
		header->state++;

	case GZIP_STATE_COMMENT:
		if (header->flags & 0x10) {
			for (; ;) {
				Header_ReadU8(tmp);
				if (tmp == '\0') break;
			}
		}
		header->state++;

	case GZIP_STATE_HEADERCHECKSUM:
		if (header->flags & 0x02) {
			for (; header->partsRead < 2; header->partsRead++) {
				Header_ReadU8(tmp);
			}
		}
		header->state++;
		header->partsRead = 0;
		header->done = true;
	}
	return 0;
}


/*########################################################################################################################*
*-------------------------------------------------------ZLib header-------------------------------------------------------*
*#########################################################################################################################*/
enum ZlibState { ZLIB_STATE_COMPRESSIONMETHOD, ZLIB_STATE_FLAGS, ZLIB_STATE_DONE };

void ZLibHeader_Init(struct ZLibHeader* header) {
	header->state = ZLIB_STATE_COMPRESSIONMETHOD;
	header->done  = false;
}

cc_result ZLibHeader_Read(struct Stream* s, struct ZLibHeader* header) {
	cc_uint8 tmp;
	cc_result res;
	switch (header->state) {

	case ZLIB_STATE_COMPRESSIONMETHOD:
		Header_ReadU8(tmp);
		if ((tmp & 0x0F) != 0x08) return ZLIB_ERR_METHOD;
		/* Upper 4 bits are window size (ignored) */
		header->state++;

	case ZLIB_STATE_FLAGS:
		Header_ReadU8(tmp);
		if (tmp & 0x20) return ZLIB_ERR_FLAGS;
		header->state++;
		header->done = true;
	}
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------Inflate (decompress)---------------------------------------------------*
*#########################################################################################################################*/
enum INFLATE_STATE_ {
	INFLATE_STATE_HEADER, INFLATE_STATE_UNCOMPRESSED_HEADER,
	INFLATE_STATE_UNCOMPRESSED_DATA, INFLATE_STATE_DYNAMIC_HEADER,
	INFLATE_STATE_DYNAMIC_CODELENS, INFLATE_STATE_DYNAMIC_LITSDISTS,
	INFLATE_STATE_DYNAMIC_LITSDISTSREPEAT, INFLATE_STATE_COMPRESSED_LIT,
	INFLATE_STATE_COMPRESSED_LITREPEAT, INFLATE_STATE_COMPRESSED_DIST,
	INFLATE_STATE_COMPRESSED_DISTREPEAT, INFLATE_STATE_COMPRESSED_DATA,
	INFLATE_STATE_FASTCOMPRESSED, INFLATE_STATE_DONE
};

/* Insert next byte into the bit buffer */
#define Inflate_GetByte(state) state->AvailIn--; state->Bits |= (cc_uint32)(*state->NextIn++) << state->NumBits; state->NumBits += 8;
/* Retrieves bits from the bit buffer */
#define Inflate_PeekBits(state, bits) (state->Bits & ((1UL << (bits)) - 1UL))
/* Consumes/eats up bits from the bit buffer */
#define Inflate_ConsumeBits(state, bits) state->Bits >>= (bits); state->NumBits -= (bits);
/* Aligns bit buffer to be on a byte boundary */
#define Inflate_AlignBits(state) cc_uint32 alignSkip = state->NumBits & 7; Inflate_ConsumeBits(state, alignSkip);
/* Ensures there are 'bitsCount' bits, or returns if not */
#define Inflate_EnsureBits(state, bitsCount) while (state->NumBits < bitsCount) { if (!state->AvailIn) return; Inflate_GetByte(state); }
/* Ensures there are 'bitsCount' bits */
#define Inflate_UNSAFE_EnsureBits(state, bitsCount) while (state->NumBits < bitsCount) { Inflate_GetByte(state); }
/* Peeks then consumes given bits */
#define Inflate_ReadBits(state, bitsCount) Inflate_PeekBits(state, bitsCount); Inflate_ConsumeBits(state, bitsCount);
/* Sets to given result and sets state to DONE */
#define Inflate_Fail(state, res) state->result = res; state->State = INFLATE_STATE_DONE;

/* Goes to the next state, after having read data of a block */
#define Inflate_NextBlockState(state) (state->LastBlock ? INFLATE_STATE_DONE : INFLATE_STATE_HEADER)
/* Goes to the next state, after having finished reading a compressed entry */
#define Inflate_NextCompressState(state) ((state->AvailIn >= INFLATE_FASTINF_IN && state->AvailOut >= INFLATE_FASTINF_OUT) ? INFLATE_STATE_FASTCOMPRESSED : INFLATE_STATE_COMPRESSED_LIT)
/* The maximum amount of bytes that can be output is 258 */
#define INFLATE_FASTINF_OUT 258
/* The most input bytes required for huffman codes and extra data is 16 + 5 + 16 + 13 bits. Add 3 extra bytes to account for putting data into the bit buffer. */
#define INFLATE_FASTINF_IN 10

static cc_uint32 Huffman_ReverseBits(cc_uint32 n, cc_uint8 bits) {
	n = ((n & 0xAAAA) >> 1) | ((n & 0x5555) << 1);
	n = ((n & 0xCCCC) >> 2) | ((n & 0x3333) << 2);
	n = ((n & 0xF0F0) >> 4) | ((n & 0x0F0F) << 4);
	n = ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
	return n >> (16 - bits);
}

/* Builds a huffman tree, based on input lengths of each codeword */
static void Huffman_Build(struct HuffmanTable* table, const cc_uint8* bitLens, int count) {
	int bl_count[INFLATE_MAX_BITS], bl_offsets[INFLATE_MAX_BITS];
	int code, offset, value;
	int i, j;

	/* Initialise 'zero bit length' codewords */
	table->FirstCodewords[0] = 0;
	table->FirstOffsets[0]   = 0;
	table->EndCodewords[0]   = 0;

	/* Count number of codewords assigned to each bit length */
	for (i = 0; i < INFLATE_MAX_BITS; i++) bl_count[i] = 0;
	for (i = 0; i < count; i++) {
		bl_count[bitLens[i]]++;
	}

	/* Ensure huffman tree actually makes sense */
	bl_count[0] = 0;
	for (i = 1; i < INFLATE_MAX_BITS; i++) {
		if (bl_count[i] > (1 << i)) {
			Logger_Abort("Too many huffman codes for bit length");
		}
	}

	/* Compute the codewords for the huffman tree.
	*  Codewords are ordered, so consider this example tree:
	*     2 of length 2, 3 of length 3, 1 of length 4
	*  Codewords produced would be: 00,01 100,101,110, 1110 
	*/
	code = 0; offset = 0;
	for (i = 1; i < INFLATE_MAX_BITS; i++) {
		code = (code + bl_count[i - 1]) << 1;
		bl_offsets[i] = offset;

		table->FirstCodewords[i] = code;
		table->FirstOffsets[i]   = offset;
		offset += bl_count[i];

		/* Last codeword is actually: code + (bl_count[i] - 1)
		*  However, when decoding we peform < against this value though, so need to add 1 here.
		*  This way, don't need to special case bit lengths with 0 codewords when decoding.
		*/
		if (bl_count[i]) {
			table->EndCodewords[i] = code + bl_count[i];
		} else {
			table->EndCodewords[i] = 0;
		}
	}

	/* Assigns values to each codeword.
	*  Note that although codewords are ordered, values may not be.
	*  Some values may also not be assigned to any codeword.
	*/
	value = 0;
	Mem_Set(table->Fast, UInt8_MaxValue, sizeof(table->Fast));
	for (i = 0; i < count; i++, value++) {
		int len = bitLens[i];
		if (!len) continue;
		table->Values[bl_offsets[len]] = value;

		/* Compute the accelerated lookup table values for this codeword.
		* For example, assume len = 4 and codeword = 0100
		* - Shift it left to be 0100_00000
		* - Then, for all the indices from 0100_00000 to 0100_11111,
		*   - bit reverse index, as huffman codes are read backwards
		*   - set fast value to specify a 'value' value, and to skip 'len' bits
		*/
		if (len <= INFLATE_FAST_BITS) {
			cc_int16 packed = (cc_int16)((len << INFLATE_FAST_BITS) | value);
			int codeword = table->FirstCodewords[len] + (bl_offsets[len] - table->FirstOffsets[len]);
			codeword <<= (INFLATE_FAST_BITS - len);

			for (j = 0; j < 1 << (INFLATE_FAST_BITS - len); j++, codeword++) {
				int index = Huffman_ReverseBits(codeword, INFLATE_FAST_BITS);
				table->Fast[index] = packed;
			}
		}
		bl_offsets[len]++;
	}
}

/* Attempts to read the next huffman encoded value from the bitstream, using given table */
/* Returns -1 if there are insufficient bits to read the value */
static int Huffman_Decode(struct InflateState* state, struct HuffmanTable* table) {
	cc_uint32 i, j, codeword;
	int packed, bits, offset;

	/* Buffer as many bits as possible */
	while (state->NumBits <= INFLATE_MAX_BITS) {
		if (!state->AvailIn) break;
		Inflate_GetByte(state);
	}

	/* Try fast accelerated table lookup */
	if (state->NumBits >= INFLATE_FAST_BITS) {
		packed = table->Fast[Inflate_PeekBits(state, INFLATE_FAST_BITS)];
		if (packed >= 0) {
			bits = packed >> INFLATE_FAST_BITS;
			Inflate_ConsumeBits(state, bits);
			return packed & 0x1FF;
		}
	}

	/* Slow, bit by bit lookup */
	codeword = 0;
	for (i = 1, j = 0; i < INFLATE_MAX_BITS; i++, j++) {
		if (state->NumBits < i) return -1;
		codeword = (codeword << 1) | ((state->Bits >> j) & 1);

		if (codeword < table->EndCodewords[i]) {
			offset = table->FirstOffsets[i] + (codeword - table->FirstCodewords[i]);
			Inflate_ConsumeBits(state, i);
			return table->Values[offset];
		}
	}

	Inflate_Fail(state, INF_ERR_INVALID_CODE);
	return -1;
}

/* Inline the common <= 9 bits case */
#define Huffman_Unsafe_Decode(state, table, result) \
{\
	Inflate_UNSAFE_EnsureBits(state, INFLATE_MAX_BITS);\
	packed = table.Fast[Inflate_PeekBits(state, INFLATE_FAST_BITS)];\
	if (packed >= 0) {\
		consumedBits = packed >> INFLATE_FAST_BITS;\
		Inflate_ConsumeBits(state, consumedBits);\
		result = packed & 0x1FF;\
	} else {\
		result = Huffman_Unsafe_Decode_Slow(state, &table);\
	}\
}

static int Huffman_Unsafe_Decode_Slow(struct InflateState* state, struct HuffmanTable* table) {
	cc_uint32 i, j, codeword;
	int offset;

	/* Slow, bit by bit lookup. Need to reverse order for huffman. */
	codeword = Inflate_PeekBits(state,       INFLATE_FAST_BITS);
	codeword = Huffman_ReverseBits(codeword, INFLATE_FAST_BITS);

	for (i = INFLATE_FAST_BITS + 1, j = INFLATE_FAST_BITS; i < INFLATE_MAX_BITS; i++, j++) {
		codeword = (codeword << 1) | ((state->Bits >> j) & 1);

		if (codeword < table->EndCodewords[i]) {
			offset = table->FirstOffsets[i] + (codeword - table->FirstCodewords[i]);
			Inflate_ConsumeBits(state, i);
			return table->Values[offset];
		}
	}

	Logger_Abort("DEFLATE - Invalid huffman code");
	return -1;
}

void Inflate_Init2(struct InflateState* state, struct Stream* source) {
	state->State = INFLATE_STATE_HEADER;
	state->LastBlock = false;
	state->Bits = 0;
	state->NumBits = 0;
	state->NextIn  = state->Input;
	state->AvailIn = 0;
	state->Output = NULL;
	state->AvailOut = 0;
	state->Source = source;
	state->WindowIndex = 0;
	state->result = 0;
}

static const cc_uint8 fixed_lits[INFLATE_MAX_LITS] = {
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8
};
static const cc_uint8 fixed_dists[INFLATE_MAX_DISTS] = {
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
};

static const cc_uint16 len_base[31] = { 
	3,4,5,6,7,8,9,10,11,13,
	15,17,19,23,27,31,35,43,51,59,
	67,83,99,115,131,163,195,227,258,0,0 
};
static const cc_uint8 len_bits[31] = { 
	0,0,0,0,0,0,0,0,1,1,
	1,1,2,2,2,2,3,3,3,3,
	4,4,4,4,5,5,5,5,0,0,0 
};
static const cc_uint16 dist_base[32] = {
	1,2,3,4,5,7,9,13,17,25,
	33,49,65,97,129,193,257,385,513,769,
	1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0 
};
static const cc_uint8 dist_bits[32] = {
	0,0,0,0,1,1,2,2,3,3,
	4,4,5,5,6,6,7,7,8,8,
	9,9,10,10,11,11,12,12,13,13,0,0 
};
static const cc_uint8 codelens_order[INFLATE_MAX_CODELENS] = {
	16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 
};

static void Inflate_InflateFast(struct InflateState* state) {
	/* huffman variables */
	cc_uint32 lit, len, dist;
	cc_uint32 bits, lenIdx, distIdx;
	int packed, consumedBits;

	/* window variables */
	cc_uint8* window;
	cc_uint32 i, curIdx, startIdx;
	cc_uint32 copyStart, copyLen, partLen;

	window = state->Window;
	curIdx = state->WindowIndex;
	copyStart = state->WindowIndex;
	copyLen   = 0;

#define INFLATE_FAST_COPY_MAX (INFLATE_WINDOW_SIZE - INFLATE_FASTINF_OUT)
	while (state->AvailOut >= INFLATE_FASTINF_OUT && state->AvailIn >= INFLATE_FASTINF_IN && copyLen < INFLATE_FAST_COPY_MAX) {
		Huffman_Unsafe_Decode(state, state->Table.Lits, lit);

		if (lit <= 256) {
			if (lit < 256) {
				window[curIdx] = (cc_uint8)lit;
				state->AvailOut--; copyLen++;
				curIdx = (curIdx + 1) & INFLATE_WINDOW_MASK;
			} else {
				state->State = Inflate_NextBlockState(state);
				break;
			}
		} else {
			lenIdx = lit - 257;
			bits = len_bits[lenIdx];
			Inflate_UNSAFE_EnsureBits(state, bits);
			len  = len_base[lenIdx] + Inflate_ReadBits(state, bits);

			Huffman_Unsafe_Decode(state, state->TableDists, distIdx);
			bits = dist_bits[distIdx];
			Inflate_UNSAFE_EnsureBits(state, bits);
			dist = dist_base[distIdx] + Inflate_ReadBits(state, bits);
	
			/* Window is infinitely repeating like ... [xyz][xyz][xyz] ... */
			/* If start and end don't cross a boundary, can avoid masking index */
			startIdx = (curIdx - dist) & INFLATE_WINDOW_MASK;
			if (curIdx >= startIdx && (curIdx + len) < INFLATE_WINDOW_SIZE) {
				cc_uint8* src = &window[startIdx]; 
				cc_uint8* dst = &window[curIdx];

				for (i = 0; i < (len & ~0x3); i += 4) {
					*dst++ = *src++; *dst++ = *src++; *dst++ = *src++; *dst++ = *src++;
				}
				for (; i < len; i++) { *dst++ = *src++; }
			} else {
				for (i = 0; i < len; i++) {
					window[(curIdx + i) & INFLATE_WINDOW_MASK] = window[(startIdx + i) & INFLATE_WINDOW_MASK];
				}
			}
			curIdx = (curIdx + len) & INFLATE_WINDOW_MASK;
			state->AvailOut -= len; copyLen += len;
		}
	}

	state->WindowIndex = curIdx;
	if (!copyLen) return;

	if (copyStart + copyLen < INFLATE_WINDOW_SIZE) {
		Mem_Copy(state->Output, &state->Window[copyStart], copyLen);
		state->Output += copyLen;
	} else {
		partLen = INFLATE_WINDOW_SIZE - copyStart;
		Mem_Copy(state->Output, &state->Window[copyStart], partLen);
		state->Output += partLen;
		Mem_Copy(state->Output, state->Window, copyLen - partLen);
		state->Output += (copyLen - partLen);
	}
}

void Inflate_Process(struct InflateState* state) {
	cc_uint32 len, dist, nlen;
	cc_uint32 i, bits;
	cc_uint32 blockHeader;

	/* len/dist table variables */
	cc_uint32 distIdx, lenIdx;
	int lit;
	/* code lens table variables */
	cc_uint32 count, repeatCount;
	cc_uint8  repeatValue;
	/* window variables */
	cc_uint32 startIdx, curIdx;
	cc_uint32 copyLen, windowCopyLen;

	for (;;) {
		switch (state->State) {
		case INFLATE_STATE_HEADER: {
			Inflate_EnsureBits(state, 3);
			blockHeader      = Inflate_ReadBits(state, 3);
			state->LastBlock = blockHeader & 1;

			switch (blockHeader >> 1) {
			case 0: { /* Uncompressed block */
				Inflate_AlignBits(state);
				state->State = INFLATE_STATE_UNCOMPRESSED_HEADER;
			} break;

			case 1: { /* Fixed/static huffman compressed */
				Huffman_Build(&state->Table.Lits, fixed_lits,  INFLATE_MAX_LITS);
				Huffman_Build(&state->TableDists, fixed_dists, INFLATE_MAX_DISTS);
				state->State = Inflate_NextCompressState(state);
			} break;

			case 2: { /* Dynamic huffman compressed */
				state->State = INFLATE_STATE_DYNAMIC_HEADER;
			} break;

			case 3: {
				Inflate_Fail(state, INF_ERR_BLOCKTYPE);
			} break;

			}
			break;
		}

		case INFLATE_STATE_UNCOMPRESSED_HEADER: {
			Inflate_EnsureBits(state, 32);
			len  = Inflate_ReadBits(state, 16);
			nlen = Inflate_ReadBits(state, 16);

			if (len != (nlen ^ 0xFFFFUL)) {
				Inflate_Fail(state, INF_ERR_BLOCKTYPE); return;
			}
			state->Index = len; /* Reuse for 'uncompressed length' */
			state->State = INFLATE_STATE_UNCOMPRESSED_DATA;
		}

		case INFLATE_STATE_UNCOMPRESSED_DATA: {
			/* read bits left in bit buffer (slow way) */
			while (state->NumBits && state->AvailOut && state->Index) {
				*state->Output = Inflate_ReadBits(state, 8);
				state->Window[state->WindowIndex] = *state->Output;

				state->WindowIndex = (state->WindowIndex + 1) & INFLATE_WINDOW_MASK;
				state->Output++; state->AvailOut--;	state->Index--;
			}
			if (!state->AvailIn || !state->AvailOut) return;

			copyLen = min(state->AvailIn, state->AvailOut);
			copyLen = min(copyLen, state->Index);
			if (copyLen > 0) {
				Mem_Copy(state->Output, state->NextIn, copyLen);
				windowCopyLen = INFLATE_WINDOW_SIZE - state->WindowIndex;
				windowCopyLen = min(windowCopyLen, copyLen);

				Mem_Copy(&state->Window[state->WindowIndex], state->Output, windowCopyLen);
				/* Wrap around remainder of copy to start from beginning of window */
				if (windowCopyLen < copyLen) {
					Mem_Copy(state->Window, &state->Output[windowCopyLen], copyLen - windowCopyLen);
				}

				state->WindowIndex = (state->WindowIndex + copyLen) & INFLATE_WINDOW_MASK;
				state->Output += copyLen; state->AvailOut -= copyLen; state->Index -= copyLen;
				state->NextIn += copyLen; state->AvailIn  -= copyLen;		
			}

			if (!state->Index) { state->State = Inflate_NextBlockState(state); }
			break;
		}

		case INFLATE_STATE_DYNAMIC_HEADER: {
			Inflate_EnsureBits(state, 14);
			state->NumLits   = 257 + Inflate_ReadBits(state, 5);
			state->NumDists    = 1 + Inflate_ReadBits(state, 5);
			state->NumCodeLens = 4 + Inflate_ReadBits(state, 4);
			state->Index = 0;
			state->State = INFLATE_STATE_DYNAMIC_CODELENS;
		}

		case INFLATE_STATE_DYNAMIC_CODELENS: {
			while (state->Index < state->NumCodeLens) {
				Inflate_EnsureBits(state, 3);
				i = codelens_order[state->Index];
				state->Buffer[i] = Inflate_ReadBits(state, 3);
				state->Index++;
			}
			for (i = state->NumCodeLens; i < INFLATE_MAX_CODELENS; i++) {
				state->Buffer[codelens_order[i]] = 0;
			}

			state->Index = 0;
			state->State = INFLATE_STATE_DYNAMIC_LITSDISTS;
			Huffman_Build(&state->Table.CodeLens, state->Buffer, INFLATE_MAX_CODELENS);
		}

		case INFLATE_STATE_DYNAMIC_LITSDISTS: {
			count = state->NumLits + state->NumDists;
			while (state->Index < count) {
				int bits = Huffman_Decode(state, &state->Table.CodeLens);
				if (bits < 16) {
					if (bits == -1) return;
					state->Buffer[state->Index] = (cc_uint8)bits;
					state->Index++;
				} else {
					state->TmpCodeLens = bits;
					state->State = INFLATE_STATE_DYNAMIC_LITSDISTSREPEAT;
					break;
				}
			}

			if (state->Index == count) {
				state->Index = 0;
				state->State = Inflate_NextCompressState(state);
				Huffman_Build(&state->Table.Lits, state->Buffer, state->NumLits);
				Huffman_Build(&state->TableDists, &state->Buffer[state->NumLits], state->NumDists);
			}
			break;
		}

		case INFLATE_STATE_DYNAMIC_LITSDISTSREPEAT: {
			switch (state->TmpCodeLens) {
			case 16:
				Inflate_EnsureBits(state, 2);
				repeatCount = Inflate_ReadBits(state, 2);
				if (!state->Index) { Inflate_Fail(state, INF_ERR_REPEAT_BEG); return; }
				repeatCount += 3; repeatValue = state->Buffer[state->Index - 1];
				break;

			case 17:
				Inflate_EnsureBits(state, 3);
				repeatCount = Inflate_ReadBits(state, 3);
				repeatCount += 3; repeatValue = 0;
				break;

			case 18:
				Inflate_EnsureBits(state, 7);
				repeatCount = Inflate_ReadBits(state, 7);
				repeatCount += 11; repeatValue = 0;
				break;
			}

			count = state->NumLits + state->NumDists;
			if (state->Index + repeatCount > count) {
				Inflate_Fail(state, INF_ERR_REPEAT_END); return;
			}

			Mem_Set(&state->Buffer[state->Index], repeatValue, repeatCount);
			state->Index += repeatCount;
			state->State = INFLATE_STATE_DYNAMIC_LITSDISTS;
			break;
		}

		case INFLATE_STATE_COMPRESSED_LIT: {
			if (!state->AvailOut) return;
			lit = Huffman_Decode(state, &state->Table.Lits);

			if (lit < 256) {
				if (lit == -1) return;
				*state->Output = (cc_uint8)lit;
				state->Window[state->WindowIndex] = (cc_uint8)lit;
				state->Output++; state->AvailOut--;
				state->WindowIndex = (state->WindowIndex + 1) & INFLATE_WINDOW_MASK;
				break;
			} else if (lit == 256) {
				state->State = Inflate_NextBlockState(state);
				break;
			} else {
				state->TmpLit = lit - 257;
				state->State  = INFLATE_STATE_COMPRESSED_LITREPEAT;
			}
		}

		case INFLATE_STATE_COMPRESSED_LITREPEAT: {
			lenIdx = state->TmpLit;
			bits   = len_bits[lenIdx];
			Inflate_EnsureBits(state, bits);
			state->TmpLit = len_base[lenIdx] + Inflate_ReadBits(state, bits);
			state->State  = INFLATE_STATE_COMPRESSED_DIST;
		}

		case INFLATE_STATE_COMPRESSED_DIST: {
			state->TmpDist = Huffman_Decode(state, &state->TableDists);
			if (state->TmpDist == -1) return;
			state->State = INFLATE_STATE_COMPRESSED_DISTREPEAT;
		}

		case INFLATE_STATE_COMPRESSED_DISTREPEAT: {
			distIdx = state->TmpDist;
			bits    = dist_bits[distIdx];
			Inflate_EnsureBits(state, bits);
			state->TmpDist = dist_base[distIdx] + Inflate_ReadBits(state, bits);
			state->State   = INFLATE_STATE_COMPRESSED_DATA;
		}

		case INFLATE_STATE_COMPRESSED_DATA: {
			if (!state->AvailOut) return;
			len = state->TmpLit; dist = state->TmpDist;
			len = min(len, state->AvailOut);

			/* TODO: Should we test outside of the loop, whether a masking will be required or not? */		
			startIdx = (state->WindowIndex - dist) & INFLATE_WINDOW_MASK;
			curIdx   = state->WindowIndex;
			for (i = 0; i < len; i++) {
				cc_uint8 value = state->Window[(startIdx + i) & INFLATE_WINDOW_MASK];
				*state->Output = value;
				state->Window[(curIdx + i) & INFLATE_WINDOW_MASK] = value;
				state->Output++;
			}

			state->WindowIndex = (curIdx + len) & INFLATE_WINDOW_MASK;
			state->TmpLit   -= len;
			state->AvailOut -= len;
			if (!state->TmpLit) { state->State = Inflate_NextCompressState(state); }
			break;
		}

		case INFLATE_STATE_FASTCOMPRESSED: {
			Inflate_InflateFast(state);
			if (state->State == INFLATE_STATE_FASTCOMPRESSED) {
				state->State = Inflate_NextCompressState(state);
			}
			break;
		}

		case INFLATE_STATE_DONE:
			return;
		}
	}
}

static cc_result Inflate_StreamRead(struct Stream* stream, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	struct InflateState* state;
	cc_uint8* inputEnd;
	cc_uint32 read, left;
	cc_uint32 startAvailOut;
	cc_bool hasInput;
	cc_result res;

	*modified = 0;
	state = (struct InflateState*)stream->Meta.Inflate;
	state->Output   = data;
	state->AvailOut = count;

	hasInput = true;
	while (state->AvailOut > 0 && hasInput) {
		if (state->State == INFLATE_STATE_DONE) return state->result;

		if (!state->AvailIn) {
			/* Fully used up input buffer. Cycle back to start. */
			inputEnd = state->Input + INFLATE_MAX_INPUT;
			if (state->NextIn == inputEnd) state->NextIn = state->Input;

			left = (cc_uint32)(inputEnd - state->NextIn);
			res  = state->Source->Read(state->Source, state->NextIn, left, &read);
			if (res) return res;

			/* Did we fail to read in more input data? Can't immediately return here, */
			/* because there might be a few bits of data left in the bit buffer */
			hasInput = read > 0;
			state->AvailIn += read;
		}
		
		/* Reading data reduces available out */
		startAvailOut = state->AvailOut;
		Inflate_Process(state);
		*modified += (startAvailOut - state->AvailOut);
	}
	return 0;
}

void Inflate_MakeStream2(struct Stream* stream, struct InflateState* state, struct Stream* underlying) {
	Stream_Init(stream);
	Inflate_Init2(state, underlying);
	stream->Meta.Inflate = state;
	stream->Read = Inflate_StreamRead;
}


/*########################################################################################################################*
*---------------------------------------------------Deflate (compress)----------------------------------------------------*
*#########################################################################################################################*/
/* these are copies of len_base and dist_base, with UINT16_MAX instead of 0 for sentinel cutoff */
static const cc_uint16 deflate_len[30] = {
	3,4,5,6,7,8,9,10,11,13,
	15,17,19,23,27,31,35,43,51,59,
	67,83,99,115,131,163,195,227,258,UInt16_MaxValue
};
static const cc_uint16 deflate_dist[31] = {
	1,2,3,4,5,7,9,13,17,25,
	33,49,65,97,129,193,257,385,513,769,
	1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,UInt16_MaxValue
};

/* Pushes given bits, but does not write them */
#define Deflate_PushBits(state, value, bits) state->Bits |= (value) << state->NumBits; state->NumBits += (bits);
/* Pushes bits of the huffman codeword bits for the given literal, but does not write them */
#define Deflate_PushLit(state, value) Deflate_PushBits(state, state->LitsCodewords[value], state->LitsLens[value])
/* Pushes given bits (reversing for huffman code), but does not write them */
#define Deflate_PushHuff(state, value, bits) Deflate_PushBits(state, Huffman_ReverseBits(value, bits), bits)
/* Writes given byte to output */
#define Deflate_WriteByte(state) *state->NextOut++ = state->Bits; state->AvailOut--; state->Bits >>= 8; state->NumBits -= 8;
/* Flushes bits in buffer to output buffer */
#define Deflate_FlushBits(state) while (state->NumBits >= 8) { Deflate_WriteByte(state); }

#define MIN_MATCH_LEN 3
#define MAX_MATCH_LEN 258

/* Number of bytes that match (are the same) from a and b */
static int Deflate_MatchLen(cc_uint8* a, cc_uint8* b, int maxLen) {
	int i = 0;
	while (i < maxLen && *a == *b) { i++; a++; b++; }
	return i;
}

/* Hashes 3 bytes of data */
static cc_uint32 Deflate_Hash(cc_uint8* src) {
	return (cc_uint32)((src[0] << 8) ^ (src[1] << 4) ^ (src[2])) & DEFLATE_HASH_MASK;
}

/* Writes a literal to state->Output */
static void Deflate_Lit(struct DeflateState* state, int lit) {
	Deflate_PushLit(state, lit);
	Deflate_FlushBits(state);
}

/* Writes a length-distance pair to state->Output */
static void Deflate_LenDist(struct DeflateState* state, int len, int dist) {
	int j;
	/* TODO: Do we actually need the if (len_bits[j]) ????????? does writing 0 bits matter??? */

	for (j = 0; len >= deflate_len[j + 1]; j++);
	Deflate_PushLit(state, j + 257);
	if (len_bits[j]) { Deflate_PushBits(state, len - deflate_len[j], len_bits[j]); }
	Deflate_FlushBits(state);

	for (j = 0; dist >= deflate_dist[j + 1]; j++);
	Deflate_PushHuff(state, j, 5);
	if (dist_bits[j]) { Deflate_PushBits(state, dist - deflate_dist[j], dist_bits[j]); }
	Deflate_FlushBits(state);
}

/* Moves "current block" to "previous block", adjusting state if needed. */
static void Deflate_MoveBlock(struct DeflateState* state) {
	int i;
	Mem_Copy(state->Input, state->Input + DEFLATE_BLOCK_SIZE, DEFLATE_BLOCK_SIZE);
	state->InputPosition = DEFLATE_BLOCK_SIZE;

	/* adjust hash table offsets, removing offsets that are no longer in data at all */
	for (i = 0; i < Array_Elems(state->Head); i++) {
		state->Head[i] = state->Head[i] < DEFLATE_BLOCK_SIZE ? 0 : (state->Head[i] - DEFLATE_BLOCK_SIZE);
	}
	for (i = 0; i < Array_Elems(state->Prev); i++) {
		state->Prev[i] = state->Prev[i] < DEFLATE_BLOCK_SIZE ? 0 : (state->Prev[i] - DEFLATE_BLOCK_SIZE);
	}
}

/* Compresses current block of data */
static cc_result Deflate_FlushBlock(struct DeflateState* state, int len) {
	cc_uint32 hash, nextHash;
	int bestLen, maxLen, matchLen, depth;
	int bestPos, pos, nextPos;
	cc_uint16 oldHead;
	cc_uint8* input;
	cc_uint8* cur;
	cc_result res;

	if (!state->WroteHeader) {
		state->WroteHeader = true;
		Deflate_PushBits(state, 3, 3); /* final block TRUE, block type FIXED */
	}

	/* Based off descriptions from http://www.gzip.org/algorithm.txt and
	https://github.com/nothings/stb/blob/master/stb_image_write.h */
	input = state->Input;
	cur   = input + DEFLATE_BLOCK_SIZE;

	/* Compress current block of data */
	/* Use > instead of >=, because also try match at one byte after current */
	while (len > MIN_MATCH_LEN) {
		hash   = Deflate_Hash(cur);
		maxLen = min(len, MAX_MATCH_LEN);

		bestLen = MIN_MATCH_LEN - 1; /* Match must be at least 3 bytes */
		bestPos = 0;

		/* Find longest match starting at this byte */
		/* Only explore up to 5 previous matches, to avoid slow performance */
		/* (i.e prefer quickly saving maps/screenshots to completely optimal filesize) */
		pos = state->Head[hash];
		for (depth = 0; pos != 0 && depth < 5; depth++) {
			matchLen = Deflate_MatchLen(&input[pos], cur, maxLen);
			if (matchLen > bestLen) { bestLen = matchLen; bestPos = pos; }
			pos = state->Prev[pos];
		}

		/* Insert this entry into the hash chain */
		pos = (int)(cur - input);
		oldHead = state->Head[hash];
		state->Head[hash] = pos;
		state->Prev[pos]  = oldHead;

		/* Lazy evaluation: Find longest match starting at next byte */
		/* If that's longer than the longest match at current byte, throwaway this match */
		if (bestPos) {
			nextHash = Deflate_Hash(cur + 1);
			nextPos  = state->Head[nextHash];
			maxLen   = min(len - 1, MAX_MATCH_LEN);

			for (depth = 0; nextPos != 0 && depth < 5; depth++) {
				matchLen = Deflate_MatchLen(&input[nextPos], cur + 1, maxLen);
				if (matchLen > bestLen) { bestPos = 0; break; }
				nextPos = state->Prev[nextPos];
			}
		}

		if (bestPos) {
			Deflate_LenDist(state, bestLen, pos - bestPos);
			len -= bestLen; cur += bestLen;
		} else {
			Deflate_Lit(state, *cur);
			len--; cur++;
		}

		/* leave room for a few bytes and literals at end */
		if (state->AvailOut >= 20) continue;
		res = Stream_Write(state->Dest, state->Output, DEFLATE_OUT_SIZE - state->AvailOut);
		state->NextOut  = state->Output;
		state->AvailOut = DEFLATE_OUT_SIZE;
		if (res) return res;
	}

	/* literals for last few bytes */
	while (len > 0) {
		Deflate_Lit(state, *cur);
		len--; cur++;
	}

	res = Stream_Write(state->Dest, state->Output, DEFLATE_OUT_SIZE - state->AvailOut);
	state->NextOut  = state->Output;
	state->AvailOut = DEFLATE_OUT_SIZE;

	Deflate_MoveBlock(state);
	return res;
}

/* Adds data to buffered output data, flushing if needed */
static cc_result Deflate_StreamWrite(struct Stream* stream, const cc_uint8* data, cc_uint32 total, cc_uint32* modified) {
	struct DeflateState* state;
	cc_result res;

	state = (struct DeflateState*)stream->Meta.Inflate;
	*modified = 0;

	while (total > 0) {
		cc_uint8* dst = &state->Input[state->InputPosition];
		cc_uint32 len = total;
		if (state->InputPosition + len >= DEFLATE_BUFFER_SIZE) {
			len = DEFLATE_BUFFER_SIZE - state->InputPosition;
		}

		Mem_Copy(dst, data, len);
		total -= len;
		state->InputPosition += len;
		*modified += len;
		data += len;

		if (state->InputPosition == DEFLATE_BUFFER_SIZE) {
			res = Deflate_FlushBlock(state, DEFLATE_BLOCK_SIZE);
			if (res) return res;
		}
	}
	return 0;
}

/* Flushes any buffered data, then writes terminating symbol */
static cc_result Deflate_StreamClose(struct Stream* stream) {
	struct DeflateState* state;
	cc_result res;

	state = (struct DeflateState*)stream->Meta.Inflate;
	res   = Deflate_FlushBlock(state, state->InputPosition - DEFLATE_BLOCK_SIZE);
	if (res) return res;

	/* Write huffman encoded "literal 256" to terminate symbols */
	Deflate_PushLit(state, 256);
	Deflate_FlushBits(state);

	/* In case last byte still has a few extra bits */
	if (state->NumBits) {
		while (state->NumBits < 8) { Deflate_PushBits(state, 0, 1); }
		Deflate_FlushBits(state);
	}

	return Stream_Write(state->Dest, state->Output, DEFLATE_OUT_SIZE - state->AvailOut);
}

/* Constructs a huffman encoding table (for values to codewords) */
static void Deflate_BuildTable(const cc_uint8* lens, int count, cc_uint16* codewords, cc_uint8* bitlens) {
	int i, j, offset, codeword;
	struct HuffmanTable table;

	Huffman_Build(&table, lens, count);
	for (i = 0; i < INFLATE_MAX_BITS; i++) {
		if (!table.EndCodewords[i]) continue;
		count = table.EndCodewords[i] - table.FirstCodewords[i];

		for (j = 0; j < count; j++) {
			offset   = table.Values[table.FirstOffsets[i] + j];
			codeword = table.FirstCodewords[i] + j;
			bitlens[offset]   = i;
			codewords[offset] = Huffman_ReverseBits(codeword, i);
		}
	}
}

void Deflate_MakeStream(struct Stream* stream, struct DeflateState* state, struct Stream* underlying) {
	Stream_Init(stream);
	stream->Meta.Inflate = state;
	stream->Write = Deflate_StreamWrite;
	stream->Close = Deflate_StreamClose;

	/* First half of buffer is "previous block" */
	state->InputPosition = DEFLATE_BLOCK_SIZE;
	state->Bits    = 0;
	state->NumBits = 0;

	state->NextOut  = state->Output;
	state->AvailOut = DEFLATE_OUT_SIZE;
	state->Dest     = underlying;
	state->WroteHeader = false;

	Mem_Set(state->Head, 0, sizeof(state->Head));
	Mem_Set(state->Prev, 0, sizeof(state->Prev));
	Deflate_BuildTable(fixed_lits, INFLATE_MAX_LITS, state->LitsCodewords, state->LitsLens);
}


/*########################################################################################################################*
*-----------------------------------------------------GZip (compress)-----------------------------------------------------*
*#########################################################################################################################*/
static cc_result GZip_StreamClose(struct Stream* stream) {
	struct GZipState* state = (struct GZipState*)stream->Meta.Inflate;
	cc_uint8 data[8];
	cc_result res;

	if ((res = Deflate_StreamClose(stream))) return res;
	Stream_SetU32_LE(&data[0], state->Crc32 ^ 0xFFFFFFFFUL);
	Stream_SetU32_LE(&data[4], state->Size);
	return Stream_Write(state->Base.Dest, data, sizeof(data));
}

static cc_result GZip_StreamWrite(struct Stream* stream, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	struct GZipState* state = (struct GZipState*)stream->Meta.Inflate;
	cc_uint32 i, crc32 = state->Crc32;
	state->Size += count;

	/* TODO: Optimise this calculation */
	for (i = 0; i < count; i++) {
		crc32 = Utils_Crc32Table[(crc32 ^ data[i]) & 0xFF] ^ (crc32 >> 8);
	}

	state->Crc32 = crc32;
	return Deflate_StreamWrite(stream, data, count, modified);
}

static cc_result GZip_StreamWriteFirst(struct Stream* stream, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	static cc_uint8 header[10] = { 0x1F, 0x8B, 0x08 }; /* GZip header */
	struct GZipState* state = (struct GZipState*)stream->Meta.Inflate;
	cc_result res;

	if ((res = Stream_Write(state->Base.Dest, header, sizeof(header)))) return res;
	stream->Write = GZip_StreamWrite;
	return GZip_StreamWrite(stream, data, count, modified);
}

void GZip_MakeStream(struct Stream* stream, struct GZipState* state, struct Stream* underlying) {
	Deflate_MakeStream(stream, &state->Base, underlying);
	state->Crc32  = 0xFFFFFFFFUL;
	state->Size   = 0;
	stream->Write = GZip_StreamWriteFirst;
	stream->Close = GZip_StreamClose;
}


/*########################################################################################################################*
*-----------------------------------------------------ZLib (compress)-----------------------------------------------------*
*#########################################################################################################################*/
static cc_result ZLib_StreamClose(struct Stream* stream) {
	struct ZLibState* state = (struct ZLibState*)stream->Meta.Inflate;
	cc_uint8 data[4];
	cc_result res;

	if ((res = Deflate_StreamClose(stream))) return res;	
	Stream_SetU32_BE(&data[0], state->Adler32);
	return Stream_Write(state->Base.Dest, data, sizeof(data));
}

static cc_result ZLib_StreamWrite(struct Stream* stream, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	struct ZLibState* state = (struct ZLibState*)stream->Meta.Inflate;
	cc_uint32 i, adler32 = state->Adler32;
	cc_uint32 s1 = adler32 & 0xFFFF, s2 = (adler32 >> 16) & 0xFFFF;

	/* TODO: Optimise this calculation */
	for (i = 0; i < count; i++) {
		#define ADLER32_BASE 65521
		s1 = (s1 + data[i]) % ADLER32_BASE;
		s2 = (s2 + s1)      % ADLER32_BASE;
	}

	state->Adler32 = (s2 << 16) | s1;
	return Deflate_StreamWrite(stream, data, count, modified);
}

static cc_result ZLib_StreamWriteFirst(struct Stream* stream, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	static cc_uint8 header[2] = { 0x78, 0x9C }; /* ZLib header */
	struct ZLibState* state = (struct ZLibState*)stream->Meta.Inflate;
	cc_result res;

	if ((res = Stream_Write(state->Base.Dest, header, sizeof(header)))) return res;
	stream->Write = ZLib_StreamWrite;
	return ZLib_StreamWrite(stream, data, count, modified);
}

void ZLib_MakeStream(struct Stream* stream, struct ZLibState* state, struct Stream* underlying) {
	Deflate_MakeStream(stream, &state->Base, underlying);
	state->Adler32 = 1;
	stream->Write = ZLib_StreamWriteFirst;
	stream->Close = ZLib_StreamClose;
}


/*########################################################################################################################*
*--------------------------------------------------------ZipEntry---------------------------------------------------------*
*#########################################################################################################################*/
#define ZIP_MAXNAMELEN 512
static cc_result Zip_ReadLocalFileHeader(struct ZipState* state, struct ZipEntry* entry) {
	struct Stream* stream = state->input;
	cc_uint8 header[26];
	cc_uint32 compressedSize, uncompressedSize;
	int method, pathLen, extraLen;

	String path; char pathBuffer[ZIP_MAXNAMELEN];
	struct Stream portion, compStream;
	struct InflateState inflate;
	cc_result res;
	if ((res = Stream_Read(stream, header, sizeof(header)))) return res;

	method           = Stream_GetU16_LE(&header[4]);
	compressedSize   = Stream_GetU32_LE(&header[14]);
	uncompressedSize = Stream_GetU32_LE(&header[18]);

	/* Some .zip files don't set these in local file header */
	if (!compressedSize)   compressedSize   = entry->CompressedSize;
	if (!uncompressedSize) uncompressedSize = entry->UncompressedSize;

	pathLen  = Stream_GetU16_LE(&header[22]);
	extraLen = Stream_GetU16_LE(&header[24]);
	if (pathLen > ZIP_MAXNAMELEN) return ZIP_ERR_FILENAME_LEN;

	/* NOTE: ZIP spec says path uses code page 437 for encoding */
	path = String_Init(pathBuffer, pathLen, pathLen);	
	if ((res = Stream_Read(stream, (cc_uint8*)pathBuffer, pathLen))) return res;
	state->_curEntry = entry;

	if (!state->SelectEntry(&path)) return 0;
	/* local file may have extra data before actual data (e.g. ZIP64) */
	if ((res = stream->Skip(stream, extraLen))) return res;

	if (method == 0) {
		Stream_ReadonlyPortion(&portion, stream, uncompressedSize);
		return state->ProcessEntry(&path, &portion, state);
	} else if (method == 8) {
		Stream_ReadonlyPortion(&portion, stream, compressedSize);
		Inflate_MakeStream2(&compStream, &inflate, &portion);
		return state->ProcessEntry(&path, &compStream, state);
	} else {
		Platform_Log1("Unsupported.zip entry compression method: %i", &method);
		/* TODO: Should this be an error */
	}
	return 0;
}

static cc_result Zip_ReadCentralDirectory(struct ZipState* state) {
	struct Stream* stream = state->input;
	struct ZipEntry* entry;
	cc_uint8 header[42];

	String path; char pathBuffer[ZIP_MAXNAMELEN];
	int pathLen, extraLen, commentLen;
	cc_result res;
	if ((res = Stream_Read(stream, header, sizeof(header)))) return res;

	pathLen = Stream_GetU16_LE(&header[24]);
	if (pathLen > ZIP_MAXNAMELEN) return ZIP_ERR_FILENAME_LEN;

	/* NOTE: ZIP spec says path uses code page 437 for encoding */
	path = String_Init(pathBuffer, pathLen, pathLen);
	if ((res = Stream_Read(stream, (cc_uint8*)pathBuffer, pathLen))) return res;

	/* skip data following central directory entry header */
	extraLen   = Stream_GetU16_LE(&header[26]);
	commentLen = Stream_GetU16_LE(&header[28]);
	if ((res = stream->Skip(stream, extraLen + commentLen))) return res;

	if (!state->SelectEntry(&path)) return 0;
	if (state->_usedEntries >= ZIP_MAX_ENTRIES) return ZIP_ERR_TOO_MANY_ENTRIES;
	entry = &state->entries[state->_usedEntries++];

	entry->CRC32             = Stream_GetU32_LE(&header[12]);
	entry->CompressedSize    = Stream_GetU32_LE(&header[16]);
	entry->UncompressedSize  = Stream_GetU32_LE(&header[20]);
	entry->LocalHeaderOffset = Stream_GetU32_LE(&header[38]);
	return 0;
}

static cc_result Zip_ReadEndOfCentralDirectory(struct ZipState* state) {
	struct Stream* stream = state->input;
	cc_uint8 header[18];

	cc_result res;
	if ((res = Stream_Read(stream, header, sizeof(header)))) return res;

	state->_totalEntries  = Stream_GetU16_LE(&header[6]);
	state->_centralDirBeg = Stream_GetU32_LE(&header[12]);
	return 0;
}

enum ZipSig {
	ZIP_SIG_ENDOFCENTRALDIR = 0x06054b50,
	ZIP_SIG_CENTRALDIR      = 0x02014b50,
	ZIP_SIG_LOCALFILEHEADER = 0x04034b50
};

static cc_result Zip_DefaultProcessor(const String* path, struct Stream* data, struct ZipState* s) { return 0; }
static cc_bool Zip_DefaultSelector(const String* path) { return true; }
void Zip_Init(struct ZipState* state, struct Stream* input) {
	state->input = input;
	state->obj   = NULL;
	state->ProcessEntry = Zip_DefaultProcessor;
	state->SelectEntry  = Zip_DefaultSelector;
}

cc_result Zip_Extract(struct ZipState* state) {
	struct Stream* stream = state->input;
	cc_uint32 stream_len;
	cc_uint32 sig = 0;
	int i, count;

	cc_result res;
	if ((res = stream->Length(stream, &stream_len))) return res;

	/* At -22 for nearly all zips, but try a bit further back in case of comment */
	count = min(257, stream_len);
	for (i = 22; i < count; i++) {
		res = stream->Seek(stream, stream_len - i);
		if (res) return ZIP_ERR_SEEK_END_OF_CENTRAL_DIR;

		if ((res = Stream_ReadU32_LE(stream, &sig))) return res;
		if (sig == ZIP_SIG_ENDOFCENTRALDIR) break;
	}

	if (sig != ZIP_SIG_ENDOFCENTRALDIR) return ZIP_ERR_NO_END_OF_CENTRAL_DIR;
	res = Zip_ReadEndOfCentralDirectory(state);
	if (res) return res;

	res = stream->Seek(stream, state->_centralDirBeg);
	if (res) return ZIP_ERR_SEEK_CENTRAL_DIR;
	state->_usedEntries = 0;

	/* Read all the central directory entries */
	for (i = 0; i < state->_totalEntries; i++) {
		if ((res = Stream_ReadU32_LE(stream, &sig))) return res;

		if (sig == ZIP_SIG_CENTRALDIR) {
			res = Zip_ReadCentralDirectory(state);
			if (res) return res;
		} else if (sig == ZIP_SIG_ENDOFCENTRALDIR) {
			break;
		} else {
			return ZIP_ERR_INVALID_CENTRAL_DIR;
		}
	}

	/* Now read the local file header entries */
	for (i = 0; i < state->_usedEntries; i++) {
		struct ZipEntry* entry = &state->entries[i];
		res = stream->Seek(stream, entry->LocalHeaderOffset);
		if (res) return ZIP_ERR_SEEK_LOCAL_DIR;

		if ((res = Stream_ReadU32_LE(stream, &sig))) return res;
		if (sig != ZIP_SIG_LOCALFILEHEADER) return ZIP_ERR_INVALID_LOCAL_DIR;

		res = Zip_ReadLocalFileHeader(state, entry);
		if (res) return res;
	}
	return 0;
}
