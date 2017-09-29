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
#define DeflateState_DynamicLits 5
#define DeflateState_DynamicDists 6
#define DeflateState_CompressedData 7

#define DeflateState_Done 250


/* TODO: This is probably completely broken. just tryna get something. */
void Huffman_Build(HuffmanTable* table, UInt8* bitLens, Int32 count) {
	Int32 i;
	table->FirstCodewords[0] = 0;
	table->FirstOffsets[0] = 0;

	Int32 bl_count[DEFLATE_MAX_BITS];
	Platform_MemSet(bl_count, 0, sizeof(bl_count));
	for (i = 0; i < count; i++) {
		bl_count[bitLens[i]]++;
	}
	for (i = 1; i < DEFLATE_MAX_BITS; i++) {
		if (bl_count[i] > (1 << i)) {
			ErrorHandler_Fail("Too many huffman codes for bit length");
		}
	}

	Int32 code = 0, value = 0;
	Int32 next_code[DEFLATE_MAX_BITS];
	for (i = 1; i < DEFLATE_MAX_BITS; i++) {
		code = (code + bl_count[i - 1]) << 1;
		next_code[i] = code;
		table->FirstCodewords[i] = (UInt16)code;
		table->FirstOffsets[i] = (UInt16)value;
		value += bl_count[i - 1];

		if (bl_count[i] == 0) {
			table->EndCodewords[i] = -1;
		} else {
			table->EndCodewords[i] = code + (bl_count[i] - 1);
		}
	}
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

/* Insert this byte into the bit buffer */
#define DEFLATE_GET_BYTE(state)\
state->AvailIn--;\
state->Bits |= (UInt32)(state->Input[state->NextIn]) << state->NumBits;\
state->NextIn++;\
state->NumBits += 8;\

/* Gets bytes from the bit buffer */
#define DEFLATE_CONSUME_BITS(state, bits, result)\
result = state->Bits & ((1UL << (bits)) - 1UL);\
state->Bits >>= bits;\
state->NumBits -= bits;

/* Aligns bit buffer to be on a byte boundary*/
#define DEFLATE_ALIGN_BITS(state, tmp)\
tmp = state->NumBits & 7;\
state->Bits >>= tmp;\
state->NumBits -= tmp;

#define DEFLATE_NEXTBLOCK_STATE(state) state->State = state->LastBlock ? DeflateState_Done : DeflateState_Header;

bool Deflate_Step(DeflateState* state) {
	switch (state->State) {
	case DeflateState_Header: {
		while (state->NumBits < 3) {
			if (state->AvailIn == 0) return false;
			DEFLATE_GET_BYTE(state);
		}

		UInt32 blockHeader;
		DEFLATE_CONSUME_BITS(state, 3, blockHeader);
		state->LastBlock = blockHeader & 1;

		switch (blockHeader >> 1) {
		case 0: { /* Uncompressed block*/
			UInt32 tmp;
			DEFLATE_ALIGN_BITS(state, tmp);
			state->State = DeflateState_UncompressedHeader;
		} break;

		case 1: { /* TODO: Compressed with FIXED huffman table*/
		} break;

		case 2: { /* TODO: Compressed with dynamic huffman table */
			state->State = DeflateState_DynamicHeader;
		} break;
		case 3:
			ErrorHandler_Fail("DEFLATE - Invalid block type");
			return false;
		}
	} break;

	case DeflateState_UncompressedHeader: {
		while (state->NumBits < 32) {
			if (state->AvailIn == 0) return false;
			DEFLATE_GET_BYTE(state);
		}

		UInt32 len, nlen;
		DEFLATE_CONSUME_BITS(state, 16, len);
		DEFLATE_CONSUME_BITS(state, 16, nlen);
		if (len != (nlen ^ 0xFFFFUL)) {
			ErrorHandler_Fail("DEFLATE - Uncompressed block LEN check failed");
		}
		state->Index = len; /* Reuse for 'uncompressed length' */
		state->State = DeflateState_UncompressedData;
	} break;

	case DeflateState_UncompressedData: {
		if (state->AvailIn > 0 || state->AvailOut > 0) return false;
		UInt32 copyLen = min(state->AvailIn, state->AvailOut);
		copyLen = min(copyLen, state->Index);

		Platform_MemCpy(state->Output, state->Input, copyLen);
		state->Output += copyLen;
		state->AvailIn -= copyLen;
		state->AvailOut -= copyLen;
		state->Index -= copyLen;

		if (state->Index == 0) {
			state->State = DEFLATE_NEXTBLOCK_STATE(state);
		}
	} break;

	case DeflateState_DynamicHeader: {
		while (state->NumBits < 14) {
			if (state->AvailIn == 0) return false;
			DEFLATE_GET_BYTE(state);
		}

		DEFLATE_CONSUME_BITS(state, 5, state->NumLits);     state->NumLits += 257;
		DEFLATE_CONSUME_BITS(state, 5, state->NumDists);    state->NumDists += 1;
		DEFLATE_CONSUME_BITS(state, 4, state->NumCodeLens); state->NumCodeLens += 4;
		state->Index = 0;
		state->State = DeflateState_DynamicCodeLens;		
	} break;

	case DeflateState_DynamicCodeLens: {
		UInt8 order[DEFLATE_MAX_CODELENS] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
		Int32 i;

		while (state->Index < state->NumCodeLens) {
			while (state->NumBits < 3) {
				if (state->AvailIn == 0) return false;
				DEFLATE_GET_BYTE(state);
			}

			i = order[state->Index];
			DEFLATE_CONSUME_BITS(state, 3, state->Buffer[i]);
			state->Index++;
		}
		for (i = state->NumCodeLens; i < DEFLATE_MAX_CODELENS; i++) {
			state->Buffer[order[i]] = 0;
		}

		state->Index = 0;
		state->State = DeflateState_DynamicLits;
	} break;

	case DeflateState_DynamicLits: {
		while (state->Index < state->NumLits) {
			/* TODO ???????? */
		}
		state->Index = 0;
		state->State = DeflateState_DynamicDists;
	} break;

	case DeflateState_DynamicDists: {
		while (state->Index < state->NumDists) {
			/* TODO ???????? */
		}
		state->Index = 0;
		state->State = DeflateState_CompressedData;
	} break;

	case DeflateState_CompressedData: {
		/* TODO ????? */
	}

	case DeflateState_Done:
		return false;
	}
	return true;
}

void Deflate_Process(DeflateState* state) {
	while (state->AvailIn > 0 || state->AvailOut > 0) {
		if (!Deflate_Step(state)) return;
	}
}