#include "GZipHeader.h"
#include "ErrorHandler.h"

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
		if (!GZipHeader_ReadAndCheckByte(s, header, 0x1F)) return;

	case GZipState_Header2:
		if (!GZipHeader_ReadAndCheckByte(s, header, 0x8B)) return;

	case GZipState_CompressionMethod:
		if (!GZipHeader_ReadAndCheckByte(s, header, 0x08)) return;

	case GZipState_Flags:
		if (!GZipHeader_ReadByte(s, header, &header->Flags)) return;
		if ((header->Flags & 0x04) != 0) {
			ErrorHandler_Fail("Unsupported GZIP header flags");
		}

	case GZipState_LastModifiedTime:
		for (; header->PartsRead < 4; header->PartsRead++) {
			temp = s->TryReadByte();
			if (temp == -1) return;
		}
		header->PartsRead = 0;
		header->State++;

	case GZipState_CompressionFlags:
		if (!GZipHeader_ReadByte(s, header, &temp)) return;

	case GZipState_OperatingSystem:
		if (!GZipHeader_ReadByte(s, header, &temp)) return;

	case GZipState_Filename:
		if ((header->Flags & 0x08) != 0) {
			for (; ;) {
				temp = s->TryReadByte();
				if (temp == -1) return;
				if (temp == 0) break;
			}
		}
		header->State++;

	case GZipState_Comment:
		if ((header->Flags & 0x10) != 0) {
			for (; ;) {
				temp = s->TryReadByte();
				if (temp == -1) return;
				if (temp == 0) break;
			}
		}
		header->State++;

	case GZipState_HeaderChecksum:
		if ((header->Flags & 0x02) != 0) {
			for (; header->PartsRead < 2; header->PartsRead++) {
				temp = s->TryReadByte();
				if (temp == -1) return;
			}
		}
		header->PartsRead = 0;
		header->State++;

		header->Done = true;
	}
}

static bool GZipHeader_ReadAndCheckByte(Stream* s, GZipHeader* header, UInt8 expected) {
	Int32 value;
	if (!GZipHeader_ReadByte(s, header, &value)) return false;

	if (value != expected) {
		ErrorHandler_Fail("Unexpected constant in GZIP header");
	}
	return true;
}

static bool GZipHeader_ReadByte(Stream* s, GZipHeader* header, Int32* value) {
	*value = s->TryReadByte();
	if (*value == -1) return false;

	header->State++;
	return true;
}