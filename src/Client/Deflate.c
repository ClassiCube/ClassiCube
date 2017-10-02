#include "Deflate.h"
#include "ErrorHandler.h"
#include "Funcs.h"
#include "Platform.h"

Int32 Header_TryReadByte(Stream* s) {
	UInt8 buffer;
	UInt32 read;
	s->Read(s, &buffer, sizeof(buffer), &read);
	return read == 0 ? -1 : buffer;
}

bool Header_ReadByte(Stream* s, UInt8* state, Int32* value) {
	*value = Header_TryReadByte(s);
	if (*value == -1) return false;

	(*state)++;
	return true;
}


#define GZipState_Header1           0
#define GZipState_Header2           1
#define GZipState_CompressionMethod 2
#define GZipState_Flags             3
#define GZipState_LastModifiedTime  4
#define GZipState_CompressionFlags  5
#define GZipState_OperatingSystem   6
#define GZipState_HeaderChecksum    7
#define GZipState_Filename          8
#define GZipState_Comment           9
#define GZipState_Done              10

void GZipHeader_Init(GZipHeader* header) {
	header->State = GZipState_Header1;
	header->Done = false;
	header->Flags = 0;
	header->PartsRead = 0;
}

void GZipHeader_Read(Stream* s, GZipHeader* header) {
	Int32 temp;
	switch (header->State) {

	case GZipState_Header1:
		if (!Header_ReadByte(s, &header->State, &temp)) return;
		if (temp != 0x1F) {
			ErrorHandler_Fail("Byte 1 of GZIP header must be 1F");
		}

	case GZipState_Header2:
		if (!Header_ReadByte(s, &header->State, &temp)) return;
		if (temp != 0x8B) {
			ErrorHandler_Fail("Byte 2 of GZIP header must be 8B");
		}

	case GZipState_CompressionMethod:
		if (!Header_ReadByte(s, &header->State, &temp)) return;
		if (temp != 0x08) {
			ErrorHandler_Fail("Only DEFLATE compression supported");
		}

	case GZipState_Flags:
		if (!Header_ReadByte(s, &header->State, &header->Flags)) return;
		if ((header->Flags & 0x04) != 0) {
			ErrorHandler_Fail("Unsupported GZIP header flags");
		}

	case GZipState_LastModifiedTime:
		for (; header->PartsRead < 4; header->PartsRead++) {
			temp = Header_TryReadByte(s);
			if (temp == -1) return;
		}
		header->State++;
		header->PartsRead = 0;

	case GZipState_CompressionFlags:
		if (!Header_ReadByte(s, &header->State, &temp)) return;

	case GZipState_OperatingSystem:
		if (!Header_ReadByte(s, &header->State, &temp)) return;

	case GZipState_Filename:
		if ((header->Flags & 0x08) != 0) {
			for (; ;) {
				temp = Header_TryReadByte(s);
				if (temp == -1) return;
				if (temp == 0) break;
			}
		}
		header->State++;

	case GZipState_Comment:
		if ((header->Flags & 0x10) != 0) {
			for (; ;) {
				temp = Header_TryReadByte(s);
				if (temp == -1) return;
				if (temp == 0) break;
			}
		}
		header->State++;

	case GZipState_HeaderChecksum:
		if ((header->Flags & 0x02) != 0) {
			for (; header->PartsRead < 2; header->PartsRead++) {
				temp = Header_TryReadByte(s);
				if (temp == -1) return;
			}
		}

		header->State++;
		header->PartsRead = 0;
		header->Done = true;
	}
}


#define ZLibState_CompressionMethod 0
#define ZLibState_Flags             1
#define ZLibState_Done              2

void ZLibHeader_Init(ZLibHeader* header) {
	header->State = ZLibState_CompressionMethod;
	header->Done = false;
	header->LZ77WindowSize = 0;
}

void ZLibHeader_Read(Stream* s, ZLibHeader* header) {
	Int32 temp;
	switch (header->State) {

	case ZLibState_CompressionMethod:
		if (!Header_ReadByte(s, &header->State, &temp)) return;
		if ((temp & 0x0F) != 0x08) {
			ErrorHandler_Fail("Only DEFLATE compression supported");
		}

		Int32 log2Size = (temp >> 4) + 8;
		header->LZ77WindowSize = 1L << log2Size;
		if (header->LZ77WindowSize > 32768) {
			ErrorHandler_Fail("LZ77 window size must be 32KB or less");
		}

	case ZLibState_Flags:
		if (!Header_ReadByte(s, &header->State, &temp)) return;
		if ((temp & 0x20) != 0) {
			ErrorHandler_Fail("Unsupported ZLIB header flags");
		}
		header->Done = true;
	}
}


#define DeflateState_Header 0
#define DeflateState_UncompressedHeader 1
#define DeflateState_UncompressedData 2
#define DeflateState_DynamicHeader 3
#define DeflateState_DynamicCodeLens 4
#define DeflateState_DynamicLitsDists 5
#define DeflateState_CompressedData 6

#define DeflateState_Done 250

/* Insert this byte into the bit buffer */
#define DEFLATE_GET_BYTE(state)\
state->AvailIn--;\
state->Bits |= (UInt32)(state->Input[state->NextIn]) << state->NumBits;\
state->NextIn++;\
state->NumBits += 8;\

/* Retrieves bits from the bit buffer */
#define DEFLATE_PEEK_BITS(state, bits) (state->Bits & ((1UL << (bits)) - 1UL))

/* Consumes/eats up bits from the bit buffer */
#define DEFLATE_CONSUME_BITS(state, bits)\
state->Bits >>= (bits);\
state->NumBits -= (bits);

/* Aligns bit buffer to be on a byte boundary*/
#define DEFLATE_ALIGN_BITS(state)\
UInt32 alignSkip = state->NumBits & 7;\
DEFLATE_CONSUME_BITS(state, alignSkip);

/* Ensures there are 'bitsCount' bits, or returns false if not.  */
#define DEFLATE_ENSURE_BITS(state, bitsCount)\
while (state->NumBits < bitsCount) {\
	if (state->AvailIn == 0) return;\
	DEFLATE_GET_BYTE(state);\
}

/* Peeks then consumes given bits. */
#define DEFLATE_READ_BITS(state, bitsCount) DEFLATE_PEEK_BITS(state, bitsCount); DEFLATE_CONSUME_BITS(state, bitsCount);

#define DEFLATE_NEXTBLOCK_STATE(state) state->State = state->LastBlock ? DeflateState_Done : DeflateState_Header;

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

	Int32 bl_count[DEFLATE_MAX_BITS];
	Platform_MemSet(bl_count, 0, sizeof(bl_count));
	for (i = 0; i < count; i++) {
		bl_count[bitLens[i]]++;
	}
	bl_count[0] = 0;
	for (i = 1; i < DEFLATE_MAX_BITS; i++) {
		if (bl_count[i] > (1 << i)) {
			ErrorHandler_Fail("Too many huffman codes for bit length");
		}
	}

	Int32 code = 0, offset = 0;
	UInt16 bl_offsets[DEFLATE_MAX_BITS];
	for (i = 1; i < DEFLATE_MAX_BITS; i++) {
		code = (code + bl_count[i - 1]) << 1;
		bl_offsets[i] = (UInt16)offset;
		table->FirstCodewords[i] = (UInt16)code;
		table->FirstOffsets[i] = (UInt16)offset;

		offset += bl_count[i];
		if (bl_count[i]) {
			table->EndCodewords[i] = code + (bl_count[i] - 1);
		} else {
			table->EndCodewords[i] = -1;
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
		if (len <= DEFLATE_ZFAST_BITS) {
			Int16 packed = (Int16)((len << 9) | value), j;
			Int32 codeword = table->FirstCodewords[len] + (bl_offsets[len] - table->FirstOffsets[len]);
			codeword <<= (DEFLATE_ZFAST_BITS - len);

			for (j = 0; j < 1 << (DEFLATE_ZFAST_BITS - len); j++, codeword++) {
				Int32 index = Huffman_ReverseBits(codeword, DEFLATE_ZFAST_BITS);
				table->Fast[index] = packed;
			}
		}
		bl_offsets[len]++;
	}
}

Int32 Huffman_Decode(DeflateState* state, HuffmanTable* table) {
	/* Buffer as many bits as possible */
	while (state->NumBits <= DEFLATE_MAX_BITS) {
		if (state->AvailIn == 0) break;
		DEFLATE_GET_BYTE(state);
	}

	/* Try fast accelerated table lookup */
	Int32 i;
	if (state->NumBits >= 9) {
		i = table->Fast[DEFLATE_PEEK_BITS(state, DEFLATE_ZFAST_BITS)];
		if (i >= 0) {
			Int32 bits = i >> 9;
			DEFLATE_CONSUME_BITS(state, bits);
			return i & 0x1FFFF;
		}
	}

	/* Slow, bit by bit lookup */
	Int32 codeword = 0;
	for (i = 1; i < DEFLATE_MAX_BITS; i++) {
		if (state->NumBits < i) return -1;
		codeword = (codeword << 1) | ((state->Bits >> i) & 1);

		if (codeword >= table->FirstCodewords[i] && codeword <= table->EndCodewords[i]) {
			Int32 offset = table->FirstOffsets[i] + (codeword - table->FirstCodewords[i]);
			DEFLATE_CONSUME_BITS(state, i);
			return table->Values[offset];
		}
	}

	ErrorHandler_Fail("DEFLATE - Invalid huffman code");
	return -1;
}

void Deflate_Init(DeflateState* state, Stream* source) {
	state->State = DeflateState_Header;
	state->Source = source;
	state->Bits = 0;
	state->NumBits = 0;
	state->AvailIn = 0;
	state->NextIn = 0;
	state->LastBlock = false;
	state->AvailOut = 0;
	state->Output = NULL;
}

void Deflate_Step(DeflateState* state) {
	/* TODO: Read input here */
	for (;;) {
		switch (state->State) {
		case DeflateState_Header: {
			DEFLATE_ENSURE_BITS(state, 3);
			UInt32 blockHeader = DEFLATE_READ_BITS(state, 3);
			state->LastBlock = blockHeader & 1;

			switch (blockHeader >> 1) {
			case 0: { /* Uncompressed block*/
				DEFLATE_ALIGN_BITS(state);
				state->State = DeflateState_UncompressedHeader;
			} break;

			case 1: { /* Fixed/static huffman compressed */
				UInt8 fixed_lits[DEFLATE_MAX_LITS] = {
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
				UInt8 fixed_dists[DEFLATE_MAX_DISTS] = {
					5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
				};
				Huffman_Build(&state->LitsTable, fixed_lits, DEFLATE_MAX_LITS);
				Huffman_Build(&state->DistsTable, fixed_dists, DEFLATE_MAX_DISTS);
				state->State = DeflateState_CompressedData;
			} break;

			case 2: /* Dynamic huffman compressed */ {
				state->State = DeflateState_DynamicHeader;
			} break;

			case 3:
				ErrorHandler_Fail("DEFLATE - Invalid block type");
			} break;
		} break;

		case DeflateState_UncompressedHeader: {
			DEFLATE_ENSURE_BITS(state, 32);
			UInt32 len = DEFLATE_READ_BITS(state, 16);
			UInt32 nlen = DEFLATE_READ_BITS(state, 16);

			if (len != (nlen ^ 0xFFFFUL)) {
				ErrorHandler_Fail("DEFLATE - Uncompressed block LEN check failed");
			}
			state->Index = len; /* Reuse for 'uncompressed length' */
			state->State = DeflateState_UncompressedData;
		} break;

		case DeflateState_UncompressedData: {
			while (state->NumBits > 0 && state->AvailOut > 0 && state->Index > 0) {
				*state->Output = DEFLATE_READ_BITS(state, 8);
				state->AvailOut--;
				state->Index--;
			}
			if (state->AvailIn == 0 || state->AvailOut == 0) return;

			UInt32 copyLen = min(state->AvailIn, state->AvailOut);
			copyLen = min(copyLen, state->Index);
			if (copyLen > 0) {
				Platform_MemCpy(state->Output, state->Input, copyLen);
				state->Output += copyLen;
				state->AvailIn -= copyLen;
				state->AvailOut -= copyLen;
				state->Index -= copyLen;
			}

			if (state->Index == 0) {
				state->State = DEFLATE_NEXTBLOCK_STATE(state);
			}
		} break;

		case DeflateState_DynamicHeader: {
			DEFLATE_ENSURE_BITS(state, 14);
			state->NumLits = DEFLATE_READ_BITS(state, 5); state->NumLits += 257;
			state->NumDists = DEFLATE_READ_BITS(state, 5); state->NumDists += 1;
			state->NumCodeLens = DEFLATE_READ_BITS(state, 4); state->NumCodeLens += 4;
			state->Index = 0;
			state->State = DeflateState_DynamicCodeLens;
		} break;

		case DeflateState_DynamicCodeLens: {
			UInt8 order[DEFLATE_MAX_CODELENS] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
			Int32 i;

			while (state->Index < state->NumCodeLens) {
				DEFLATE_ENSURE_BITS(state, 3);
				i = order[state->Index];
				state->Buffer[i] = DEFLATE_READ_BITS(state, 3);
				state->Index++;
			}
			for (i = state->NumCodeLens; i < DEFLATE_MAX_CODELENS; i++) {
				state->Buffer[order[i]] = 0;
			}

			state->Index = 0;
			state->State = DeflateState_DynamicLitsDists;
			Huffman_Build(&state->CodeLensTable, state->Buffer, DEFLATE_MAX_CODELENS);
		} break;

		case DeflateState_DynamicLitsDists: {
			UInt32 count = state->NumLits + state->NumDists;
			while (state->Index < count) {
				Int32 bits = Huffman_Decode(state, &state->CodeLensTable);
				if (bits < 16) {
					if (bits == -1) return;
					state->Buffer[state->Index] = (UInt8)bits;
					state->Index++;
				}
				else {
					UInt32 repeatCount;
					UInt8 repeatValue;

					switch (bits) {
					case 16:
						DEFLATE_ENSURE_BITS(state, 2);
						repeatCount = DEFLATE_READ_BITS(state, 2);
						if (state->Index == 0) {
							ErrorHandler_Fail("DEFLATE - Tried to repeat invalid byte");
						}
						repeatCount += 3; repeatValue = state->Buffer[state->Index - 1];
						break;

					case 17:
						DEFLATE_ENSURE_BITS(state, 3);
						repeatCount = DEFLATE_READ_BITS(state, 3);
						repeatCount += 3; repeatValue = 0;
						break;

					case 18:
						DEFLATE_ENSURE_BITS(state, 7);
						repeatCount = DEFLATE_READ_BITS(state, 7);
						repeatCount += 11; repeatValue = 0;
						break;
					}

					if (state->Index + repeatCount > count) {
						ErrorHandler_Fail("DEFLATE - Tried to repeat past end");
					}
					Platform_MemSet(&state->Buffer[state->Index], repeatValue, repeatCount);
					state->Index += repeatCount;
				}
			}
			state->Index = 0;
			state->State = DeflateState_CompressedData;
			Huffman_Build(&state->LitsTable, state->Buffer, state->NumLits);
			Huffman_Build(&state->DistsTable, &state->Buffer[state->NumLits], state->NumDists);
		} break;

		case DeflateState_CompressedData: {
			if (state->AvailIn == 0 || state->AvailOut == 0) return;
			Int32 len = Huffman_Decode(state, &state->LitsTable);

			Int32 dist = Huffman_Decode(state, &state->DistsTable);
			/* TODO: Do stuff from here*/
		}

		case DeflateState_Done:
			return;
		}
	}
}