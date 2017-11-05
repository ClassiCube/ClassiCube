#include "ZipArchive.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Deflate.h"

String Zip_ReadFixedString(Stream* stream, UInt8* buffer, UInt16 length) {
	String fileName;
	fileName.buffer = buffer;
	fileName.length = length;
	fileName.capacity = length;

	Stream_Read(stream, buffer, length);
	buffer[length] = NULL; /* Ensure null terminated */
	return fileName;
}

void Zip_ReadLocalFileHeader(ZipState* state, ZipEntry* entry) {
	Stream* stream = state->Input;
	UInt16 versionNeeded = Stream_ReadUInt16_LE(stream);
	UInt16 flags = Stream_ReadUInt16_LE(stream);
	UInt16 compressionMethod = Stream_ReadUInt16_LE(stream);
	Stream_ReadUInt32_LE(stream); /* last modified */
	Stream_ReadUInt32_LE(stream); /* CRC32 */

	Int32 compressedSize = Stream_ReadInt32_LE(stream);
	if (compressedSize == 0) compressedSize = entry->CompressedDataSize;
	Int32 uncompressedSize = Stream_ReadInt32_LE(stream);
	if (uncompressedSize == 0) uncompressedSize = entry->UncompressedDataSize;

	UInt16 fileNameLen = Stream_ReadUInt16_LE(stream);
	UInt16 extraFieldLen = Stream_ReadUInt16_LE(stream);
	UInt8 filenameBuffer[String_BufferSize(UInt16_MaxValue)];
	String filename = Zip_ReadFixedString(stream, filenameBuffer, fileNameLen);
	if (!state->SelectEntry(&filename)) return;

	stream->Seek(stream, extraFieldLen, STREAM_SEEKFROM_CURRENT);
	if (versionNeeded > 20) {
		String warnMsg = String_FromConst("May not be able to properly extract a .zip enty with a version later than 2.0");
		Platform_Log(&warnMsg);
	}

	Stream portion, compStream;
	if (compressionMethod == 0) {
		Stream_ReadonlyPortion(&portion, stream, uncompressedSize);
		state->ProcessEntry(&filename, &portion, entry);
	} else if (compressionMethod == 8) {
		DeflateState deflate;
		Stream_ReadonlyPortion(&portion, stream, compressedSize);
		Deflate_MakeStream(&compStream, &deflate, &portion);
		state->ProcessEntry(&filename, &compStream, entry);
	}
}

void Zip_ReadCentralDirectory(ZipState* state, ZipEntry* entry) {
	Stream* stream = state->Input;
	Stream_ReadUInt16_LE(stream); /* OS */
	UInt16 versionNeeded = Stream_ReadUInt16_LE(stream);
	UInt16 flags = Stream_ReadUInt16_LE(stream);
	UInt16 compressionMethod = Stream_ReadUInt16_LE(stream);
	Stream_ReadUInt32_LE(stream); /* last modified */
	entry->Crc32 = Stream_ReadUInt32_LE(stream);
	entry->CompressedDataSize = Stream_ReadInt32_LE(stream);
	entry->UncompressedDataSize = Stream_ReadInt32_LE(stream);

	UInt16 fileNameLen = Stream_ReadUInt16_LE(stream);
	UInt16 extraFieldLen = Stream_ReadUInt16_LE(stream);
	UInt16 fileCommentLen = Stream_ReadUInt16_LE(stream);
	UInt16 diskNum = Stream_ReadUInt16_LE(stream);
	UInt16 internalAttributes = Stream_ReadUInt16_LE(stream);
	UInt32 externalAttributes = Stream_ReadUInt32_LE(stream);
	entry->LocalHeaderOffset = Stream_ReadInt32_LE(stream);

	Int32 extraDataLen = fileNameLen + extraFieldLen + fileCommentLen;
	stream->Seek(stream, extraDataLen, STREAM_SEEKFROM_CURRENT);
}

void Zip_ReadEndOfCentralDirectory(ZipState* state, Int32* centralDirectoryOffset) {
	Stream* stream = state->Input;
	UInt16 diskNum = Stream_ReadUInt16_LE(stream);
	UInt16 diskNumStart = Stream_ReadUInt16_LE(stream);
	UInt16 diskEntries = Stream_ReadUInt16_LE(stream);
	state->EntriesCount = Stream_ReadUInt16_LE(stream);
	Int32 centralDirectorySize = Stream_ReadInt32_LE(stream);
	*centralDirectoryOffset = Stream_ReadInt32_LE(stream);
	UInt16 commentLength = Stream_ReadUInt16_LE(stream);
}


#define ZIP_ENDOFCENTRALDIR 0x06054b50UL
#define ZIP_CENTRALDIR      0x02014b50UL
#define ZIP_LOCALFILEHEADER 0x04034b50UL

void Zip_DefaultProcessor(STRING_TRANSIENT String* path, Stream* data, ZipEntry* entry) { }
bool Zip_DefaultSelector(STRING_TRANSIENT String* path) { return true; }
void Zip_Init(ZipState* state, Stream* input) {
	state->Input = input;
	state->EntriesCount = 0;
	state->ProcessEntry = Zip_DefaultProcessor;
	state->SelectEntry = Zip_DefaultSelector;
}

void Zip_Extract(ZipState* state) {
	state->EntriesCount = 0;
	Stream* stream = state->Input;
	ReturnCode result = stream->Seek(stream, -22, STREAM_SEEKFROM_END);
	ErrorHandler_CheckOrFail(result, "ZIP - Seek to end of central directory");

	UInt32 sig = Stream_ReadUInt32_LE(stream);
	if (sig != ZIP_ENDOFCENTRALDIR) {
		ErrorHandler_Fail("ZIP - Comment in .zip file not supported");
		return;
	}

	Int32 centralDirectoryOffset;
	Zip_ReadEndOfCentralDirectory(state, &centralDirectoryOffset);
	result = stream->Seek(stream, centralDirectoryOffset, STREAM_SEEKFROM_BEGIN);
	ErrorHandler_CheckOrFail(result, "ZIP - Seek to central directory");
	if (state->EntriesCount > ZIP_MAX_ENTRIES) {
		ErrorHandler_Fail("ZIP - Max of 2048 entries supported");
	}

	/* Read all the central directory entries */
	Int32 count = 0;
	while (count < state->EntriesCount) {
		sig = Stream_ReadUInt32_LE(stream);
		if (sig == ZIP_CENTRALDIR) {
			Zip_ReadCentralDirectory(state, &state->Entries[count]);
			count++;
		} else if (sig == ZIP_ENDOFCENTRALDIR) {
			break;
		} else {
			String sigMsg = String_FromConst("ZIP - Unsupported signature found, aborting");
			ErrorHandler_Log(&sigMsg);
			return;
		}
	}

	/* Now read the local file header entries */
	Int32 i;
	for (i = 0; i < count; i++) {
		ZipEntry* entry = &state->Entries[i];
		result = stream->Seek(stream, entry->LocalHeaderOffset, STREAM_SEEKFROM_BEGIN);
		ErrorHandler_CheckOrFail(result, "ZIP - Seek to local file header");

		sig = Stream_ReadUInt32_LE(stream);
		if (sig != ZIP_LOCALFILEHEADER) {
			String sigMsg = String_FromConst("ZIP - Invalid entry found, skipping");
			ErrorHandler_Log(&sigMsg);
			continue;
		}
		Zip_ReadLocalFileHeader(state, entry);
	}
}