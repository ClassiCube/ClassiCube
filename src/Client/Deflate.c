#include "Deflate.h"
#include "ErrorHandler.h"
#include "Funcs.h"
#include "Platform.h"
#include "Stream.h"

bool Header_ReadByte(Stream* s, UInt8* state, Int32* value) {
	*value = Stream_TryReadByte(s);
	if (*value == -1) return false;

	(*state)++;
	return true;
}

enum GZIP_STATE_ {
	GZIP_STATE_HEADER1,
	GZIP_STATE_HEADER2,
	GZIP_STATE_COMPRESSIONMETHOD,
	GZIP_STATE_FLAGS,
	GZIP_STATE_LASTMODIFIEDTIME,
	GZIP_STATE_COMPRESSIONFLAGS,
	GZIP_STATE_OPERATINGSYSTEM,
	GZIP_STATE_HEADERCHECKSUM,
	GZIP_STATE_FILENAME,
	GZIP_STATE_COMMENT,
	GZIP_STATE_DONE,
};

void GZipHeader_Init(GZipHeader* header) {
	header->State = GZIP_STATE_HEADER1;
	header->Done = false;
	header->Flags = 0;
	header->PartsRead = 0;
}

void GZipHeader_Read(Stream* s, GZipHeader* header) {
	Int32 temp;
	switch (header->State) {

	case GZIP_STATE_HEADER1:
		if (!Header_ReadByte(s, &header->State, &temp)) return;
		if (temp != 0x1F) { ErrorHandler_Fail("Byte 1 of GZIP header must be 1F"); }

	case GZIP_STATE_HEADER2:
		if (!Header_ReadByte(s, &header->State, &temp)) return;
		if (temp != 0x8B) { ErrorHandler_Fail("Byte 2 of GZIP header must be 8B"); }

	case GZIP_STATE_COMPRESSIONMETHOD:
		if (!Header_ReadByte(s, &header->State, &temp)) return;
		if (temp != 0x08) { ErrorHandler_Fail("Only DEFLATE compression supported"); }

	case GZIP_STATE_FLAGS:
		if (!Header_ReadByte(s, &header->State, &header->Flags)) return;
		if (header->Flags & 0x04) { ErrorHandler_Fail("Unsupported GZIP header flags"); }

	case GZIP_STATE_LASTMODIFIEDTIME:
		for (; header->PartsRead < 4; header->PartsRead++) {
			temp = Stream_TryReadByte(s);
			if (temp == -1) return;
		}
		header->State++;
		header->PartsRead = 0;

	case GZIP_STATE_COMPRESSIONFLAGS:
		if (!Header_ReadByte(s, &header->State, &temp)) return;

	case GZIP_STATE_OPERATINGSYSTEM:
		if (!Header_ReadByte(s, &header->State, &temp)) return;

	case GZIP_STATE_FILENAME:
		if (header->Flags & 0x08) {
			for (; ;) {
				temp = Stream_TryReadByte(s);
				if (temp == -1) return;
				if (temp == NULL) break;
			}
		}
		header->State++;

	case GZIP_STATE_COMMENT:
		if (header->Flags & 0x10) {
			for (; ;) {
				temp = Stream_TryReadByte(s);
				if (temp == -1) return;
				if (temp == NULL) break;
			}
		}
		header->State++;

	case GZIP_STATE_HEADERCHECKSUM:
		if (header->Flags & 0x02) {
			for (; header->PartsRead < 2; header->PartsRead++) {
				temp = Stream_TryReadByte(s);
				if (temp == -1) return;
			}
		}

		header->State++;
		header->PartsRead = 0;
		header->Done = true;
	}
}


enum ZLIB_STATE_ {
	ZLIB_STATE_COMPRESSIONMETHOD,
	ZLIB_STATE_FLAGS,
	ZLIB_STATE_DONE,
};

void ZLibHeader_Init(ZLibHeader* header) {
	header->State = ZLIB_STATE_COMPRESSIONMETHOD;
	header->Done = false;
	header->LZ77WindowSize = 0;
}

void ZLibHeader_Read(Stream* s, ZLibHeader* header) {
	Int32 temp;
	switch (header->State) {

	case ZLIB_STATE_COMPRESSIONMETHOD:
		if (!Header_ReadByte(s, &header->State, &temp)) return;
		if ((temp & 0x0F) != 0x08) {
			ErrorHandler_Fail("Only DEFLATE compression supported");
		}

		Int32 log2Size = (temp >> 4) + 8;
		header->LZ77WindowSize = 1L << log2Size;
		if (header->LZ77WindowSize > 32768) {
			ErrorHandler_Fail("LZ77 window size must be 32KB or less");
		}

	case ZLIB_STATE_FLAGS:
		if (!Header_ReadByte(s, &header->State, &temp)) return;
		if ((temp & 0x20) != 0) {
			ErrorHandler_Fail("Unsupported ZLIB header flags");
		}
		header->Done = true;
	}
}


enum INFLATE_STATE_ {
INFLATE_STATE_HEADER,
INFLATE_STATE_UNCOMPRESSED_HEADER,
INFLATE_STATE_UNCOMPRESSED_DATA,
INFLATE_STATE_DYNAMIC_HEADER,
INFLATE_STATE_DYNAMIC_CODELENS,
INFLATE_STATE_DYNAMIC_LITSDISTS,
INFLATE_STATE_DYNAMIC_LITSDISTSREPEAT,
INFLATE_STATE_COMPRESSED_LIT,
INFLATE_STATE_COMPRESSED_LITREPEAT,
INFLATE_STATE_COMPRESSED_DIST,
INFLATE_STATE_COMPRESSED_DISTREPEAT,
INFLATE_STATE_COMPRESSED_DATA,
INFLATE_STATE_FASTCOMPRESSED,
INFLATE_STATE_DONE,
};

/* Insert this byte into the bit buffer */
#define Inflate_GetByte(state) state->AvailIn--; state->Bits |= (UInt32)(state->Input[state->NextIn]) << state->NumBits; state->NextIn++; state->NumBits += 8;
/* Retrieves bits from the bit buffer */
#define Inflate_PeekBits(state, bits) (state->Bits & ((1UL << (bits)) - 1UL))
/* Consumes/eats up bits from the bit buffer */
#define Inflate_ConsumeBits(state, bits) state->Bits >>= (bits); state->NumBits -= (bits);
/* Aligns bit buffer to be on a byte boundary*/
#define Inflate_AlignBits(state) UInt32 alignSkip = state->NumBits & 7; Inflate_ConsumeBits(state, alignSkip);
/* Ensures there are 'bitsCount' bits, or returns false if not.  */
#define Inflate_EnsureBits(state, bitsCount) while (state->NumBits < bitsCount) { if (state->AvailIn == 0) return; Inflate_GetByte(state); }
/* Ensures there are 'bitsCount' bits.  */
#define Inflate_UNSAFE_EnsureBits(state, bitsCount) while (state->NumBits < bitsCount) { Inflate_GetByte(state); }
/* Peeks then consumes given bits. */
#define Inflate_ReadBits(state, bitsCount) Inflate_PeekBits(state, bitsCount); Inflate_ConsumeBits(state, bitsCount);

/* Goes to the next state, after having read data of a block. */
#define Inflate_NextBlockState(state) (state->LastBlock ? INFLATE_STATE_DONE : INFLATE_STATE_HEADER)
/* Goes to the next state, after having finished reading a compressed entry. */
/* The maximum amount of bytes that can be output is 258. */
/* The most input bytes required for huffman codes and extra data is 16 + 5 + 16 + 13 bits. Add 3 extra bytes to account for putting data into the bit buffer. */
#define INFLATE_FASTINF_OUT 258
#define INFLATE_FASTINF_IN 10
#define Inflate_NextCompressState(state) ((state->AvailIn >= INFLATE_FASTINF_IN && state->AvailOut >= INFLATE_FASTINF_OUT) ? INFLATE_STATE_FASTCOMPRESSED : INFLATE_STATE_COMPRESSED_LIT)

UInt32 Huffman_ReverseBits(UInt32 n, UInt8 bits) {
	n = ((n & 0xAAAA) >> 1) | ((n & 0x5555) << 1);
	n = ((n & 0xCCCC) >> 2) | ((n & 0x3333) << 2);
	n = ((n & 0xF0F0) >> 4) | ((n & 0x0F0F) << 4);
	n = ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
	return n >> (16 - bits);
}

void Huffman_Build(HuffmanTable* table, UInt8* bitLens, Int32 count) {
	Int32 i;
	table->FirstCodewords[0] = 0;
	table->FirstOffsets[0] = 0;
	table->EndCodewords[0] = -1;

	Int32 bl_count[INFLATE_MAX_BITS] = { 0 };
	for (i = 0; i < count; i++) {
		bl_count[bitLens[i]]++;
	}
	bl_count[0] = 0;
	for (i = 1; i < INFLATE_MAX_BITS; i++) {
		if (bl_count[i] > (1 << i)) {
			ErrorHandler_Fail("Too many huffman codes for bit length");
		}
	}

	Int32 code = 0, offset = 0;
	UInt16 bl_offsets[INFLATE_MAX_BITS];
	for (i = 1; i < INFLATE_MAX_BITS; i++) {
		code = (code + bl_count[i - 1]) << 1;
		bl_offsets[i] = (UInt16)offset;
		table->FirstCodewords[i] = (UInt16)code;
		table->FirstOffsets[i] = (UInt16)offset;

		offset += bl_count[i];
		/* Last codeword is actually: code + (bl_count[i] - 1) */
		/* When decoding we peform < against this value though, so we need to add 1 here */
		if (bl_count[i]) {
			table->EndCodewords[i] = (UInt16)(code + bl_count[i]); 
		} else {
			table->EndCodewords[i] = 0;
		}
	}

	Int32 value = 0;
	Platform_MemSet(table->Fast, UInt8_MaxValue, sizeof(table->Fast));
	for (i = 0; i < count; i++, value++) {
		Int32 len = bitLens[i];
		if (len == 0) continue;
		table->Values[bl_offsets[len]] = (UInt16)value;

		/* Computes the accelerated lookup table values for this codeword
		* For example, assume len = 4 and codeword = 0100
		* - Shift it left to be 0100_00000
		* - Then, for all the indices from 0100_00000 to 0100_11111,
		*   - bit reverse index, as huffman codes are read backwards
		*   - set fast value to specify a 'value' value, and to skip 'len' bits
		*/
		if (len <= INFLATE_FAST_BITS) {
			Int16 packed = (Int16)((len << 9) | value), j;
			Int32 codeword = table->FirstCodewords[len] + (bl_offsets[len] - table->FirstOffsets[len]);
			codeword <<= (INFLATE_FAST_BITS - len);

			for (j = 0; j < 1 << (INFLATE_FAST_BITS - len); j++, codeword++) {
				Int32 index = Huffman_ReverseBits(codeword, INFLATE_FAST_BITS);
				table->Fast[index] = packed;
			}
		}
		bl_offsets[len]++;
	}
}

Int32 Huffman_Decode(InflateState* state, HuffmanTable* table) {
	/* Buffer as many bits as possible */
	while (state->NumBits <= INFLATE_MAX_BITS) {
		if (state->AvailIn == 0) break;
		Inflate_GetByte(state);
	}

	/* Try fast accelerated table lookup */
	if (state->NumBits >= 9) {
		Int32 packed = table->Fast[Inflate_PeekBits(state, INFLATE_FAST_BITS)];
		if (packed >= 0) {
			Int32 bits = packed >> 9;
			Inflate_ConsumeBits(state, bits);
			return packed & 0x1FF;
		}
	}

	/* Slow, bit by bit lookup */
	UInt32 codeword = 0;
	UInt32 i, j;
	for (i = 1, j = 0; i < INFLATE_MAX_BITS; i++, j++) {
		if (state->NumBits < i) return -1;
		codeword = (codeword << 1) | ((state->Bits >> j) & 1);

		if (codeword < table->EndCodewords[i]) {
			Int32 offset = table->FirstOffsets[i] + (codeword - table->FirstCodewords[i]);
			Inflate_ConsumeBits(state, i);
			return table->Values[offset];
		}
	}

	ErrorHandler_Fail("DEFLATE - Invalid huffman code");
	return -1;
}

Int32 Huffman_Unsafe_Decode(InflateState* state, HuffmanTable* table) {
	Inflate_UNSAFE_EnsureBits(state, INFLATE_MAX_BITS);
	UInt32 codeword = Inflate_PeekBits(state, INFLATE_FAST_BITS);
	Int32 packed = table->Fast[codeword];
	if (packed >= 0) {
		Int32 bits = packed >> 9;
		Inflate_ConsumeBits(state, bits);
		return packed & 0x1FF;
	}

	/* Slow, bit by bit lookup. Need to reverse order for huffman. */
	codeword = Huffman_ReverseBits(codeword, INFLATE_FAST_BITS);
	UInt32 i, j;
	for (i = INFLATE_FAST_BITS + 1, j = INFLATE_FAST_BITS; i < INFLATE_MAX_BITS; i++, j++) {
		codeword = (codeword << 1) | ((state->Bits >> j) & 1);

		if (codeword < table->EndCodewords[i]) {
			Int32 offset = table->FirstOffsets[i] + (codeword - table->FirstCodewords[i]);
			Inflate_ConsumeBits(state, i);
			return table->Values[offset];
		}
	}

	ErrorHandler_Fail("DEFLATE - Invalid huffman code");
	return -1;
}

void Inflate_Init(InflateState* state, Stream* source) {
	state->State = INFLATE_STATE_HEADER;
	state->Source = source;
	state->Bits = 0;
	state->NumBits = 0;
	state->AvailIn = 0;
	state->NextIn = 0;
	state->LastBlock = false;
	state->AvailOut = 0;
	state->Output = NULL;
	state->WindowIndex = 0;
}

UInt8 fixed_lits[INFLATE_MAX_LITS] = {
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
UInt8 fixed_dists[INFLATE_MAX_DISTS] = {
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
};

UInt16 len_base[31] = { 3,4,5,6,7,8,9,10,11,13,
15,17,19,23,27,31,35,43,51,59,
67,83,99,115,131,163,195,227,258,0,0 };
UInt8 len_bits[31] = { 0,0,0,0,0,0,0,0,1,1,
1,1,2,2,2,2,3,3,3,3,
4,4,4,4,5,5,5,5,0,0,0 };
UInt16 dist_base[32] = { 1,2,3,4,5,7,9,13,17,25,
33,49,65,97,129,193,257,385,513,769,
1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0 };
UInt8 dist_bits[32] = { 0,0,0,0,1,1,2,2,3,3,
4,4,5,5,6,6,7,7,8,8,
9,9,10,10,11,11,12,12,13,13,0,0 };
UInt8 codelens_order[INFLATE_MAX_CODELENS] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };

void Deflate_InflateFast(InflateState* state) {
	UInt32 copyStart = state->WindowIndex, copyLen = 0;
	UInt8* window = state->Window;
	UInt32 curIdx = state->WindowIndex;

#define INFLATE_FAST_COPY_MAX (INFLATE_WINDOW_SIZE - INFLATE_FASTINF_OUT)
	while (state->AvailOut >= INFLATE_FASTINF_OUT && state->AvailIn >= INFLATE_FASTINF_IN && copyLen < INFLATE_FAST_COPY_MAX) {
		UInt32 lit = Huffman_Unsafe_Decode(state, &state->LitsTable);
		if (lit <= 256) {
			if (lit < 256) {
				window[curIdx] = (UInt8)lit;
				state->AvailOut--; copyLen++;
				curIdx = (curIdx + 1) & INFLATE_WINDOW_MASK;
			} else {
				state->State = Inflate_NextBlockState(state);
				break;
			}
		} else {
			UInt32 lenIdx = lit - 257;
			UInt32 bits = len_bits[lenIdx];
			Inflate_UNSAFE_EnsureBits(state, bits);
			UInt32 len = len_base[lenIdx] + Inflate_ReadBits(state, bits);

			UInt32 distIdx = Huffman_Unsafe_Decode(state, &state->DistsTable);
			bits = dist_bits[distIdx];
			Inflate_UNSAFE_EnsureBits(state, bits);
			UInt32 dist = dist_base[distIdx] + Inflate_ReadBits(state, bits);

			UInt32 startIdx = (curIdx - dist) & INFLATE_WINDOW_MASK;
			UInt32 i;
			if (curIdx >= startIdx && (curIdx + len) < INFLATE_WINDOW_SIZE) {
				UInt8* src = &window[startIdx]; UInt8* dst = &window[curIdx];
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
	if (copyLen > 0) {
		if (copyStart + copyLen < INFLATE_WINDOW_SIZE) {
			Platform_MemCpy(state->Output, &state->Window[copyStart], copyLen);
			state->Output += copyLen;
		} else {
			UInt32 partLen = INFLATE_WINDOW_SIZE - copyStart;
			Platform_MemCpy(state->Output, &state->Window[copyStart], partLen);
			state->Output += partLen;
			Platform_MemCpy(state->Output, state->Window, copyLen - partLen);
			state->Output += (copyLen - partLen);
		}
	}
}

void Inflate_Process(InflateState* state) {
	for (;;) {
		switch (state->State) {
		case INFLATE_STATE_HEADER: {
			Inflate_EnsureBits(state, 3);
			UInt32 blockHeader = Inflate_ReadBits(state, 3);
			state->LastBlock = blockHeader & 1;

			switch (blockHeader >> 1) {
			case 0: { /* Uncompressed block */
				Inflate_AlignBits(state);
				state->State = INFLATE_STATE_UNCOMPRESSED_HEADER;
			} break;

			case 1: { /* Fixed/static huffman compressed */
				Huffman_Build(&state->LitsTable, fixed_lits, INFLATE_MAX_LITS);
				Huffman_Build(&state->DistsTable, fixed_dists, INFLATE_MAX_DISTS);
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
			UInt32 len = Inflate_ReadBits(state, 16);
			UInt32 nlen = Inflate_ReadBits(state, 16);

			if (len != (nlen ^ 0xFFFFUL)) {
				ErrorHandler_Fail("DEFLATE - Uncompressed block LEN check failed");
			}
			state->Index = len; /* Reuse for 'uncompressed length' */
			state->State = INFLATE_STATE_UNCOMPRESSED_DATA;
		}

		case INFLATE_STATE_UNCOMPRESSED_DATA: {
			while (state->NumBits > 0 && state->AvailOut > 0 && state->Index > 0) {
				*state->Output = Inflate_ReadBits(state, 8);
				state->AvailOut--;
				state->Index--;
			}
			if (state->AvailIn == 0 || state->AvailOut == 0) return;

			UInt32 copyLen = min(state->AvailIn, state->AvailOut);
			copyLen = min(copyLen, state->Index);
			if (copyLen > 0) {
				Platform_MemCpy(state->Output, state->Input, copyLen);
				/* TODO: Copy output to window!!! */
				state->Output += copyLen; state->AvailOut -= copyLen;
				state->AvailIn -= copyLen;
				state->Index -= copyLen;
			}

			if (state->Index == 0) {
				state->State = Inflate_NextBlockState(state);
			}
			break;
		}

		case INFLATE_STATE_DYNAMIC_HEADER: {
			Inflate_EnsureBits(state, 14);
			state->NumLits = 257 + Inflate_ReadBits(state, 5);
			state->NumDists = 1 + Inflate_ReadBits(state, 5);
			state->NumCodeLens = 4 + Inflate_ReadBits(state, 4);
			state->Index = 0;
			state->State = INFLATE_STATE_DYNAMIC_CODELENS;
		}

		case INFLATE_STATE_DYNAMIC_CODELENS: {
			Int32 i;
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
			Huffman_Build(&state->CodeLensTable, state->Buffer, INFLATE_MAX_CODELENS);
		}

		case INFLATE_STATE_DYNAMIC_LITSDISTS: {
			UInt32 count = state->NumLits + state->NumDists;
			while (state->Index < count) {
				Int32 bits = Huffman_Decode(state, &state->CodeLensTable);
				if (bits < 16) {
					if (bits == -1) return;
					state->Buffer[state->Index] = (UInt8)bits;
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
				Huffman_Build(&state->LitsTable, state->Buffer, state->NumLits);
				Huffman_Build(&state->DistsTable, &state->Buffer[state->NumLits], state->NumDists);
			}
			break;
		}

		case INFLATE_STATE_DYNAMIC_LITSDISTSREPEAT: {
			UInt32 repeatCount;
			UInt8 repeatValue;

			switch (state->TmpCodeLens) {
			case 16:
				Inflate_EnsureBits(state, 2);
				repeatCount = Inflate_ReadBits(state, 2);
				if (state->Index == 0) {
					ErrorHandler_Fail("DEFLATE - Tried to repeat invalid byte");
				}
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

			UInt32 count = state->NumLits + state->NumDists;
			if (state->Index + repeatCount > count) {
				ErrorHandler_Fail("DEFLATE - Tried to repeat past end");
			}

			Platform_MemSet(&state->Buffer[state->Index], repeatValue, repeatCount);
			state->Index += repeatCount;
			state->State = INFLATE_STATE_DYNAMIC_LITSDISTS;
			break;
		}

		case INFLATE_STATE_COMPRESSED_LIT: {
			if (state->AvailOut == 0) return;

			Int32 lit = Huffman_Decode(state, &state->LitsTable);
			if (lit < 256) {
				if (lit == -1) return;
				*state->Output = (UInt8)lit;
				state->Window[state->WindowIndex] = (UInt8)lit;
				state->Output++; state->AvailOut--;
				state->WindowIndex = (state->WindowIndex + 1) & INFLATE_WINDOW_MASK;
				break;
			} else if (lit == 256) {
				state->State = Inflate_NextBlockState(state);
				break;
			} else {
				state->TmpLit = lit - 257;
				state->State = INFLATE_STATE_COMPRESSED_LITREPEAT;
			}
		}

		case INFLATE_STATE_COMPRESSED_LITREPEAT: {
			UInt32 lenIdx = (UInt32)state->TmpLit;
			UInt32 bits = len_bits[lenIdx];
			Inflate_EnsureBits(state, bits);
			state->TmpLit = len_base[lenIdx] + Inflate_ReadBits(state, bits);
			state->State = INFLATE_STATE_COMPRESSED_DIST;
		}

		case INFLATE_STATE_COMPRESSED_DIST: {
			state->TmpDist = Huffman_Decode(state, &state->DistsTable);
			if (state->TmpDist == -1) return;
			state->State = INFLATE_STATE_COMPRESSED_DISTREPEAT;
		}

		case INFLATE_STATE_COMPRESSED_DISTREPEAT: {
			UInt32 distIdx = (UInt32)state->TmpDist;
			UInt32 bits = dist_bits[distIdx];
			Inflate_EnsureBits(state, bits);
			state->TmpDist = dist_base[distIdx] + Inflate_ReadBits(state, bits);
			state->State = INFLATE_STATE_COMPRESSED_DATA;
		}

		case INFLATE_STATE_COMPRESSED_DATA: {
			if (state->AvailOut == 0) return;
			UInt32 len = (UInt32)state->TmpLit, dist = (UInt32)state->TmpDist;
			len = min(len, state->AvailOut);

			/* TODO: Should we test outside of the loop, whether a masking will be required or not? */
			UInt32 startIdx = (state->WindowIndex - dist) & INFLATE_WINDOW_MASK, curIdx = state->WindowIndex;
			UInt32 i;
			for (i = 0; i < len; i++) {
				UInt8 value = state->Window[(startIdx + i) & INFLATE_WINDOW_MASK];
				*state->Output = value;
				state->Window[(curIdx + i) & INFLATE_WINDOW_MASK] = value;
				state->Output++;
			}

			state->WindowIndex = (curIdx + len) & INFLATE_WINDOW_MASK;
			state->TmpLit -= len;
			state->AvailOut -= len;
			if (state->TmpLit == 0) { state->State = Inflate_NextCompressState(state); }
			break;
		}

		case INFLATE_STATE_FASTCOMPRESSED: {
			Deflate_InflateFast(state);
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

ReturnCode Inflate_StreamRead(Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	InflateState* state = (InflateState*)stream->Meta_Inflate;
	*modified = 0;
	state->Output = data;
	state->AvailOut = count;

	while (state->AvailOut > 0) {
		if (state->State == INFLATE_STATE_DONE) break;
		if (state->AvailIn == 0) {
			/* Fully used up input buffer. Cycle back to start. */
			if (state->NextIn == INFLATE_MAX_INPUT) state->NextIn = 0;

			UInt8* ptr = &state->Input[state->NextIn];
			UInt32 read, remaining = INFLATE_MAX_INPUT - state->NextIn;
			ReturnCode code = state->Source->Read(state->Source, ptr, remaining, &read);

			/* Did we fail to read in more input data? */
			/* If there's a few bits of data in the bit buffer it doesn't matter since Inflate_Process
			/* would have already processed as much as it possibly could already */
			/* TODO: Is this assumption about bit buffer right */
			if (code != 0 || read == 0) return code;
			state->AvailIn += read;
		}
		
		/* Reading data reduces available out */
		UInt32 preAvailOut = state->AvailOut;
		Inflate_Process(state);
		*modified += (preAvailOut - state->AvailOut);
	}
	return 0;
}

ReturnCode Inflate_StreamWrite(Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	*modified = 0; return 1;
}
ReturnCode Inflate_StreamClose(Stream* stream) { return 0; }
ReturnCode Inflate_StreamSeek(Stream* stream, Int32 offset, Int32 seekType) { return 1; }

void Inflate_MakeStream(Stream* stream, InflateState* state, Stream* underlying) {
	Inflate_Init(state, underlying);
	Stream_SetName(stream, &underlying->Name);
	stream->Meta_Inflate = state;

	stream->Read  = Inflate_StreamRead;
	stream->Write = Inflate_StreamWrite;
	stream->Close = Inflate_StreamClose;
	stream->Seek  = Inflate_StreamSeek;
}