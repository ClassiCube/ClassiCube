#include "Deflate.h"
#include "ErrorHandler.h"
#include "Funcs.h"
#include "Platform.h"
#include "Stream.h"
#include "Errors.h"
#include "Utils.h"

#define Header_ReadU8(value) if ((res = s->ReadU8(s, &value))) return res;
/*########################################################################################################################*
*-------------------------------------------------------GZip header-------------------------------------------------------*
*#########################################################################################################################*/
enum GZIP_STATE {
	GZIP_STATE_HEADER1, GZIP_STATE_HEADER2, GZIP_STATE_COMPRESSIONMETHOD, GZIP_STATE_FLAGS,
	GZIP_STATE_LASTMODIFIED, GZIP_STATE_COMPRESSIONFLAGS, GZIP_STATE_OPERATINGSYSTEM, 
	GZIP_STATE_HEADERCHECKSUM, GZIP_STATE_FILENAME, GZIP_STATE_COMMENT, GZIP_STATE_DONE
};

void GZipHeader_Init(struct GZipHeader* header) {
	header->State = GZIP_STATE_HEADER1;
	header->Done = false;
	header->Flags = 0;
	header->PartsRead = 0;
}

ReturnCode GZipHeader_Read(struct Stream* s, struct GZipHeader* header) {
	uint8_t tmp;
	ReturnCode res;
	switch (header->State) {

	case GZIP_STATE_HEADER1:
		Header_ReadU8(tmp);
		if (tmp != 0x1F) return GZIP_ERR_HEADER1;
		header->State++;

	case GZIP_STATE_HEADER2:
		Header_ReadU8(tmp);
		if (tmp != 0x8B) return GZIP_ERR_HEADER2;
		header->State++;

	case GZIP_STATE_COMPRESSIONMETHOD:
		Header_ReadU8(tmp);
		if (tmp != 0x08) return GZIP_ERR_METHOD;
		header->State++;

	case GZIP_STATE_FLAGS:
		Header_ReadU8(tmp);
		if (header->Flags & 0x04) return GZIP_ERR_FLAGS;
		header->State++;

	case GZIP_STATE_LASTMODIFIED:
		for (; header->PartsRead < 4; header->PartsRead++) {
			Header_ReadU8(tmp);
		}
		header->State++;
		header->PartsRead = 0;

	case GZIP_STATE_COMPRESSIONFLAGS:
		Header_ReadU8(tmp);
		header->State++;

	case GZIP_STATE_OPERATINGSYSTEM:
		Header_ReadU8(tmp);
		header->State++;

	case GZIP_STATE_FILENAME:
		if (header->Flags & 0x08) {
			for (; ;) {
				Header_ReadU8(tmp);
				if (tmp == '\0') break;
			}
		}
		header->State++;

	case GZIP_STATE_COMMENT:
		if (header->Flags & 0x10) {
			for (; ;) {
				Header_ReadU8(tmp);
				if (tmp == '\0') break;
			}
		}
		header->State++;

	case GZIP_STATE_HEADERCHECKSUM:
		if (header->Flags & 0x02) {
			for (; header->PartsRead < 2; header->PartsRead++) {
				Header_ReadU8(tmp);
			}
		}
		header->State++;
		header->PartsRead = 0;
		header->Done = true;
	}
	return 0;
}


/*########################################################################################################################*
*-------------------------------------------------------ZLib header-------------------------------------------------------*
*#########################################################################################################################*/
enum ZLIB_STATE { ZLIB_STATE_COMPRESSIONMETHOD, ZLIB_STATE_FLAGS, ZLIB_STATE_DONE };

void ZLibHeader_Init(struct ZLibHeader* header) {
	header->State = ZLIB_STATE_COMPRESSIONMETHOD;
	header->Done = false;
}

ReturnCode ZLibHeader_Read(struct Stream* s, struct ZLibHeader* header) {
	uint8_t tmp;
	ReturnCode res;
	switch (header->State) {

	case ZLIB_STATE_COMPRESSIONMETHOD:
		Header_ReadU8(tmp);
		if ((tmp & 0x0F) != 0x08) return ZLIB_ERR_METHOD;
		if ((tmp >> 4) > 7) return ZLIB_ERR_WINDOW_SIZE;
		/* 2^(size + 8) must be < 32768 for LZ77 window */
		header->State++;

	case ZLIB_STATE_FLAGS:
		Header_ReadU8(tmp);
		if (tmp & 0x20) return ZLIB_ERR_FLAGS;
		header->State++;
		header->Done = true;
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
#define Inflate_GetByte(state) state->AvailIn--; state->Bits |= (uint32_t)(*state->NextIn++) << state->NumBits; state->NumBits += 8;
/* Retrieves bits from the bit buffer */
#define Inflate_PeekBits(state, bits) (state->Bits & ((1UL << (bits)) - 1UL))
/* Consumes/eats up bits from the bit buffer */
#define Inflate_ConsumeBits(state, bits) state->Bits >>= (bits); state->NumBits -= (bits);
/* Aligns bit buffer to be on a byte boundary */
#define Inflate_AlignBits(state) uint32_t alignSkip = state->NumBits & 7; Inflate_ConsumeBits(state, alignSkip);
/* Ensures there are 'bitsCount' bits, or returns if not */
#define Inflate_EnsureBits(state, bitsCount) while (state->NumBits < bitsCount) { if (!state->AvailIn) return; Inflate_GetByte(state); }
/* Ensures there are 'bitsCount' bits */
#define Inflate_UNSAFE_EnsureBits(state, bitsCount) while (state->NumBits < bitsCount) { Inflate_GetByte(state); }
/* Peeks then consumes given bits */
#define Inflate_ReadBits(state, bitsCount) Inflate_PeekBits(state, bitsCount); Inflate_ConsumeBits(state, bitsCount);

/* Goes to the next state, after having read data of a block */
#define Inflate_NextBlockState(state) (state->LastBlock ? INFLATE_STATE_DONE : INFLATE_STATE_HEADER)
/* Goes to the next state, after having finished reading a compressed entry */
#define Inflate_NextCompressState(state) ((state->AvailIn >= INFLATE_FASTINF_IN && state->AvailOut >= INFLATE_FASTINF_OUT) ? INFLATE_STATE_FASTCOMPRESSED : INFLATE_STATE_COMPRESSED_LIT)
/* The maximum amount of bytes that can be output is 258 */
#define INFLATE_FASTINF_OUT 258
/* The most input bytes required for huffman codes and extra data is 16 + 5 + 16 + 13 bits. Add 3 extra bytes to account for putting data into the bit buffer. */
#define INFLATE_FASTINF_IN 10

static uint32_t Huffman_ReverseBits(uint32_t n, uint8_t bits) {
	n = ((n & 0xAAAA) >> 1) | ((n & 0x5555) << 1);
	n = ((n & 0xCCCC) >> 2) | ((n & 0x3333) << 2);
	n = ((n & 0xF0F0) >> 4) | ((n & 0x0F0F) << 4);
	n = ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
	return n >> (16 - bits);
}

/* Builds a huffman tree, based on input lengths of each codeword */
static void Huffman_Build(struct HuffmanTable* table, uint8_t* bitLens, int count) {
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
			ErrorHandler_Fail("Too many huffman codes for bit length");
		}
	}

	/* Compute the codewords for the huffman tree
	* Codewords are ordered, so consider this example tree:
	*    2 of length 2, 3 of length 3, 1 of length 4
	* Codewords produced would be: 00,01 100,101,110, 1110 
	*/
	code = 0; offset = 0;
	for (i = 1; i < INFLATE_MAX_BITS; i++) {
		code = (code + bl_count[i - 1]) << 1;
		bl_offsets[i] = offset;

		table->FirstCodewords[i] = code;
		table->FirstOffsets[i]   = offset;
		offset += bl_count[i];

		/* Last codeword is actually: code + (bl_count[i] - 1)
		* Hoever, when decoding we peform < against this value though, so need to add 1 here 
		* This way, don't need to special case bit lengths with 0 codewords when decoding 
		*/
		if (bl_count[i]) {
			table->EndCodewords[i] = code + bl_count[i];
		} else {
			table->EndCodewords[i] = 0;
		}
	}

	/* Assigns values to each codeword
	* Note that although codewords are ordered, values may not be
	* Some values may also not be assigned to any codeword
	*/
	value = 0;
	Mem_Set(table->Fast, UInt8_MaxValue, sizeof(table->Fast));
	for (i = 0; i < count; i++, value++) {
		int len = bitLens[i];
		if (!len) continue;
		table->Values[bl_offsets[len]] = value;

		/* Compute the accelerated lookup table values for this codeword
		* For example, assume len = 4 and codeword = 0100
		* - Shift it left to be 0100_00000
		* - Then, for all the indices from 0100_00000 to 0100_11111,
		*   - bit reverse index, as huffman codes are read backwards
		*   - set fast value to specify a 'value' value, and to skip 'len' bits
		*/
		if (len <= INFLATE_FAST_BITS) {
			int16_t packed = (int16_t)((len << INFLATE_FAST_BITS) | value);
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
	uint32_t i, j, codeword;
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

	ErrorHandler_Fail("DEFLATE - Invalid huffman code");
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
	uint32_t i, j, codeword;
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

	ErrorHandler_Fail("DEFLATE - Invalid huffman code");
	return -1;
}

void Inflate_Init(struct InflateState* state, struct Stream* source) {
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
}

static uint8_t fixed_lits[INFLATE_MAX_LITS] = {
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
static uint8_t fixed_dists[INFLATE_MAX_DISTS] = {
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
};

static uint16_t len_base[31] = { 
	3,4,5,6,7,8,9,10,11,13,
	15,17,19,23,27,31,35,43,51,59,
	67,83,99,115,131,163,195,227,258,0,0 
};
static uint8_t len_bits[31] = { 
	0,0,0,0,0,0,0,0,1,1,
	1,1,2,2,2,2,3,3,3,3,
	4,4,4,4,5,5,5,5,0,0,0 
};
static uint16_t dist_base[32] = {
	1,2,3,4,5,7,9,13,17,25,
	33,49,65,97,129,193,257,385,513,769,
	1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0 
};
static uint8_t dist_bits[32] = { 
	0,0,0,0,1,1,2,2,3,3,
	4,4,5,5,6,6,7,7,8,8,
	9,9,10,10,11,11,12,12,13,13,0,0 
};
static uint8_t codelens_order[INFLATE_MAX_CODELENS] = {
	16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 
};

static void Inflate_InflateFast(struct InflateState* state) {
	/* huffman variables */
	uint32_t lit, len, dist;
	uint32_t bits, lenIdx, distIdx;
	int packed, consumedBits;

	/* window variables */
	uint8_t* window;
	uint32_t i, curIdx, startIdx;
	uint32_t copyStart, copyLen, partLen;

	window = state->Window;
	curIdx = state->WindowIndex;
	copyStart = state->WindowIndex;
	copyLen   = 0;

#define INFLATE_FAST_COPY_MAX (INFLATE_WINDOW_SIZE - INFLATE_FASTINF_OUT)
	while (state->AvailOut >= INFLATE_FASTINF_OUT && state->AvailIn >= INFLATE_FASTINF_IN && copyLen < INFLATE_FAST_COPY_MAX) {
		Huffman_Unsafe_Decode(state, state->Table.Lits, lit);

		if (lit <= 256) {
			if (lit < 256) {
				window[curIdx] = (uint8_t)lit;
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
				uint8_t* src = &window[startIdx]; 
				uint8_t* dst = &window[curIdx];

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
	uint32_t len, dist, nlen;
	uint32_t i, bits;
	uint32_t blockHeader;

	/* len/dist table variables */
	uint32_t distIdx, lenIdx;
	int lit;
	/* code lens table variables */
	uint32_t count, repeatCount;
	uint8_t  repeatValue;
	/* window variables */
	uint32_t startIdx, curIdx;
	uint32_t copyLen, windowCopyLen;

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
				ErrorHandler_Fail("DEFLATE - Invalid block type");
			} break;

			}
			break;
		}

		case INFLATE_STATE_UNCOMPRESSED_HEADER: {
			Inflate_EnsureBits(state, 32);
			len  = Inflate_ReadBits(state, 16);
			nlen = Inflate_ReadBits(state, 16);

			if (len != (nlen ^ 0xFFFFUL)) {
				ErrorHandler_Fail("DEFLATE - Uncompressed block LEN check failed");
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
					state->Buffer[state->Index] = (uint8_t)bits;
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
				if (!state->Index) ErrorHandler_Fail("DEFLATE - Tried to repeat invalid byte");
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
				ErrorHandler_Fail("DEFLATE - Tried to repeat past end");
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
				*state->Output = (uint8_t)lit;
				state->Window[state->WindowIndex] = (uint8_t)lit;
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
				uint8_t value = state->Window[(startIdx + i) & INFLATE_WINDOW_MASK];
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

static ReturnCode Inflate_StreamRead(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified) {
	struct InflateState* state;
	uint8_t* inputEnd;
	uint32_t read, left;
	uint32_t startAvailOut;
	bool hasInput;
	ReturnCode res;

	*modified = 0;
	state = stream->Meta.Inflate;
	state->Output   = data;
	state->AvailOut = count;

	hasInput = true;
	while (state->AvailOut > 0 && hasInput) {
		if (state->State == INFLATE_STATE_DONE) break;

		if (!state->AvailIn) {
			/* Fully used up input buffer. Cycle back to start. */
			inputEnd = state->Input + INFLATE_MAX_INPUT;
			if (state->NextIn == inputEnd) state->NextIn = state->Input;

			left = (uint32_t)(inputEnd - state->NextIn);
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

void Inflate_MakeStream(struct Stream* stream, struct InflateState* state, struct Stream* underlying) {
	Stream_Init(stream);
	Inflate_Init(state, underlying);
	stream->Meta.Inflate = state;
	stream->Read = Inflate_StreamRead;
}


/*########################################################################################################################*
*---------------------------------------------------Deflate (compress)----------------------------------------------------*
*#########################################################################################################################*/
/* Pushes given bits, but does not write them */
#define Deflate_PushBits(state, value, bits) state->Bits |= (value) << state->NumBits; state->NumBits += (bits);
/* Pushes given bits (reversing for huffman code), but does not write them */
#define Deflate_PushHuff(state, value, bits) Deflate_PushBits(state, Huffman_ReverseBits(value, bits), bits)
/* Writes given byte to output */
#define Deflate_WriteByte(state) *state->NextOut++ = state->Bits; state->AvailOut--; state->Bits >>= 8; state->NumBits -= 8;
/* Flushes bits in buffer to output buffer */
#define Deflate_FlushBits(state) while (state->NumBits >= 8) { Deflate_WriteByte(state); }

#define DEFLATE_MAX_MATCH_LEN 258
static int Deflate_MatchLen(uint8_t* a, uint8_t* b, int maxLen) {
	int i = 0;
	while (i < maxLen && *a == *b) { i++; a++; b++; }
	return i;
}

static uint32_t Deflate_Hash(uint8_t* src) {
	return (uint32_t)((src[0] << 8) ^ (src[1] << 4) ^ (src[2])) & DEFLATE_HASH_MASK;
}

static void Deflate_Lit(struct DeflateState* state, int lit) {
	if (lit <= 143) { Deflate_PushHuff(state, lit + 48, 8); } 
	else { Deflate_PushHuff(state, lit + 256, 9); }
	Deflate_FlushBits(state);
}

static void Deflate_LenDist(struct DeflateState* state, int len, int dist) {
	int j;
	len_base[29]  = UInt16_MaxValue;
	dist_base[30] = UInt16_MaxValue;
	/* TODO: Remove this hack out into Deflate_FlushBlock */
	/* TODO: is that hack even thread-safe */
	/* TODO: Do we actually need the if (len_bits[j]) ????????? does writing 0 bits matter??? */

	for (j = 0; len >= len_base[j + 1]; j++);
	if (j <= 22) { Deflate_PushHuff(state, j + 1, 7); }
	else { Deflate_PushHuff(state, j + 169, 8); }
	if (len_bits[j]) { Deflate_PushBits(state, len - len_base[j], len_bits[j]); }
	Deflate_FlushBits(state);

	for (j = 0; dist >= dist_base[j + 1]; j++);
	Deflate_PushHuff(state, j, 5);
	if (dist_bits[j]) { Deflate_PushBits(state, dist - dist_base[j], dist_bits[j]); }
	Deflate_FlushBits(state);

	len_base[29]  = 0;
	dist_base[30] = 0;
}

static ReturnCode Deflate_FlushBlock(struct DeflateState* state, int len) {
	uint32_t hash, nextHash;
	int bestLen, maxLen, matchLen;
	int bestPos, pos, nextPos;
	uint16_t oldHead;
	uint8_t* src;
	uint8_t* cur;
	ReturnCode res;

	if (!state->WroteHeader) {
		state->WroteHeader = true;
		Deflate_PushBits(state, 3, 3); /* final block TRUE, block type FIXED */
	}

	/* TODO: Hash chains should persist past one block flush */
	Mem_Set(state->Head, 0, sizeof(state->Head));
	Mem_Set(state->Prev, 0, sizeof(state->Prev));

	/* Based off descriptions from http://www.gzip.org/algorithm.txt and
	https://github.com/nothings/stb/blob/master/stb_image_write.h */
	src = state->Input;
	cur = src;

	while (len > 3) {
		hash   = Deflate_Hash(cur);
		maxLen = min(len, DEFLATE_MAX_MATCH_LEN);

		bestLen = 3 - 1; /* Match must be at least 3 bytes */
		bestPos = 0;

		/* Find longest match starting at this byte */
		pos = state->Head[hash];
		while (pos != 0) { /* TODO: Need to limit chain length here */
			matchLen = Deflate_MatchLen(&src[pos], cur, maxLen);
			if (matchLen > bestLen) { bestLen = matchLen; bestPos = pos; }
			pos = state->Prev[pos];
		}

		/* Insert this entry into the hash chain */
		pos = (int)(cur - src);
		oldHead = state->Head[hash];
		state->Head[hash] = pos;
		state->Prev[pos]  = oldHead;

		/* Lazy evaluation: Find longest match starting at next byte */
		/* If that's longer than the longest match at current byte, throwaway this match */
		if (bestPos && len > 2) {
			nextHash = Deflate_Hash(cur + 1);
			nextPos  = state->Head[nextHash];
			maxLen   = min(len - 1, DEFLATE_MAX_MATCH_LEN);

			while (nextPos != 0) { /* TODO: Need to limit chain length here */
				matchLen = Deflate_MatchLen(&src[nextPos], cur + 1, maxLen);
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
	state->InputPosition = 0;

	res = Stream_Write(state->Dest, state->Output, DEFLATE_OUT_SIZE - state->AvailOut);
	state->NextOut  = state->Output;
	state->AvailOut = DEFLATE_OUT_SIZE;
	return res;
}

static ReturnCode Deflate_StreamWrite(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified) {
	struct DeflateState* state;
	ReturnCode res;

	state = stream->Meta.Inflate;
	*modified = 0;

	while (count > 0) {
		uint8_t* dst = &state->Input[state->InputPosition];
		uint32_t toWrite = count;
		if (state->InputPosition + toWrite >= DEFLATE_BUFFER_SIZE) {
			toWrite = DEFLATE_BUFFER_SIZE - state->InputPosition;
		}

		Mem_Copy(dst, data, toWrite);
		count -= toWrite;
		state->InputPosition += toWrite;
		*modified += toWrite;
		data += toWrite;

		if (state->InputPosition == DEFLATE_BUFFER_SIZE) {
			res = Deflate_FlushBlock(state, DEFLATE_BUFFER_SIZE);
			if (res) return res;
		}
	}
	return 0;
}

static ReturnCode Deflate_StreamClose(struct Stream* stream) {
	struct DeflateState* state;
	ReturnCode res;

	state = stream->Meta.Inflate;
	res   = Deflate_FlushBlock(state, state->InputPosition);
	if (res) return res;

	/* Write huffman encoded "literal 256" to terminate symbols */
	Deflate_PushBits(state, 512, 7); 
	Deflate_FlushBits(state);

	/* In case last byte still has a few extra bits */
	if (state->NumBits) {
		while (state->NumBits < 8) { Deflate_PushBits(state, 0, 1); }
		Deflate_FlushBits(state);
	}

	return Stream_Write(state->Dest, state->Output, DEFLATE_OUT_SIZE - state->AvailOut);
}

void Deflate_MakeStream(struct Stream* stream, struct DeflateState* state, struct Stream* underlying) {
	Stream_Init(stream);
	stream->Meta.Inflate = state;
	stream->Write = Deflate_StreamWrite;
	stream->Close = Deflate_StreamClose;

	state->InputPosition = 0;
	state->Bits    = 0;
	state->NumBits = 0;

	state->NextOut  = state->Output;
	state->AvailOut = DEFLATE_OUT_SIZE;
	state->Dest     = underlying;
	state->WroteHeader = false;

	Mem_Set(state->Head, 0, sizeof(state->Head));
	Mem_Set(state->Prev, 0, sizeof(state->Prev));	
}


/*########################################################################################################################*
*-----------------------------------------------------GZip (compress)-----------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode GZip_StreamClose(struct Stream* stream) {
	uint8_t data[8];
	struct GZipState* state = stream->Meta.Inflate;
	ReturnCode res;

	if ((res = Deflate_StreamClose(stream))) return res;
	Stream_SetU32_LE(&data[0], state->Crc32 ^ 0xFFFFFFFFUL);
	Stream_SetU32_LE(&data[4], state->Size);
	return Stream_Write(state->Base.Dest, data, sizeof(data));
}

static ReturnCode GZip_StreamWrite(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified) {
	struct GZipState* state = stream->Meta.Inflate;
	uint32_t i, crc32 = state->Crc32;
	state->Size += count;

	/* TODO: Optimise this calculation */
	for (i = 0; i < count; i++) {
		crc32 = Utils_Crc32Table[(crc32 ^ data[i]) & 0xFF] ^ (crc32 >> 8);
	}

	state->Crc32 = crc32;
	return Deflate_StreamWrite(stream, data, count, modified);
}

static ReturnCode GZip_StreamWriteFirst(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified) {
	static uint8_t header[10] = { 0x1F, 0x8B, 0x08 }; /* GZip header */
	struct GZipState* state = stream->Meta.Inflate;
	ReturnCode res;

	if ((res = Stream_Write(state->Base.Dest, header, sizeof(header)))) return res;
	stream->Write = GZip_StreamWrite;
	return GZip_StreamWrite(stream, data, count, modified);
}

void GZip_MakeStream(struct Stream* stream, struct GZipState* state, struct Stream* underlying) {
	Deflate_MakeStream(stream, &state->Base, underlying);
	state->Crc32 = 0xFFFFFFFFUL;
	state->Size  = 0;
	stream->Write = GZip_StreamWriteFirst;
	stream->Close = GZip_StreamClose;
}


/*########################################################################################################################*
*-----------------------------------------------------ZLib (compress)-----------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode ZLib_StreamClose(struct Stream* stream) {
	uint8_t data[4];
	struct ZLibState* state = stream->Meta.Inflate;
	ReturnCode res;

	if ((res = Deflate_StreamClose(stream))) return res;	
	Stream_SetU32_BE(&data[0], state->Adler32);
	return Stream_Write(state->Base.Dest, data, sizeof(data));
}

static ReturnCode ZLib_StreamWrite(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified) {
	struct ZLibState* state = stream->Meta.Inflate;
	uint32_t i, adler32 = state->Adler32;
	uint32_t s1 = adler32 & 0xFFFF, s2 = (adler32 >> 16) & 0xFFFF;

	/* TODO: Optimise this calculation */
	for (i = 0; i < count; i++) {
		#define ADLER32_BASE 65521
		s1 = (s1 + data[i]) % ADLER32_BASE;
		s2 = (s2 + s1)      % ADLER32_BASE;
	}

	state->Adler32 = (s2 << 16) | s1;
	return Deflate_StreamWrite(stream, data, count, modified);
}

static ReturnCode ZLib_StreamWriteFirst(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified) {
	static uint8_t header[2] = { 0x78, 0x9C }; /* ZLib header */
	struct ZLibState* state = stream->Meta.Inflate;
	ReturnCode res;

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
