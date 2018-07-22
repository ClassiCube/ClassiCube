#include "Bitmap.h"
#include "Platform.h"
#include "PackedCol.h"
#include "ExtMath.h"
#include "Deflate.h"
#include "ErrorHandler.h"
#include "Stream.h"

void Bitmap_Create(struct Bitmap* bmp, Int32 width, Int32 height, UInt8* scan0) {
	bmp->Width = width; bmp->Height = height;
	bmp->Stride = width * BITMAP_SIZEOF_PIXEL;
	bmp->Scan0 = scan0;
}

void Bitmap_CopyBlock(Int32 srcX, Int32 srcY, Int32 dstX, Int32 dstY, struct Bitmap* src, struct Bitmap* dst, Int32 size) {
	Int32 x, y;
	for (y = 0; y < size; y++) {
		UInt32* srcRow = Bitmap_GetRow(src, srcY + y);
		UInt32* dstRow = Bitmap_GetRow(dst, dstY + y);
		for (x = 0; x < size; x++) {
			dstRow[dstX + x] = srcRow[srcX + x];
		}
	}
}

void Bitmap_Allocate(struct Bitmap* bmp, Int32 width, Int32 height) {
	bmp->Width = width; bmp->Height = height;
	bmp->Stride = width * BITMAP_SIZEOF_PIXEL;
	bmp->Scan0 = Platform_MemAlloc(width * height, BITMAP_SIZEOF_PIXEL);
	
	if (bmp->Scan0 == NULL) {
		ErrorHandler_Fail("Bitmap - failed to allocate memory");
	}
}

void Bitmap_AllocateClearedPow2(struct Bitmap* bmp, Int32 width, Int32 height) {
	width = Math_NextPowOf2(width);
	height = Math_NextPowOf2(height);
	Bitmap_Allocate(bmp, width, height);
	Platform_MemSet(bmp->Scan0, 0, Bitmap_DataSize(width, height));
}


/*########################################################################################################################*
*------------------------------------------------------PNG decoder--------------------------------------------------------*
*#########################################################################################################################*/
#define PNG_SIG_SIZE 8
#define PNG_IHDR_SIZE 13
#define PNG_RGB_MASK 0xFFFFFFUL
#define PNG_PALETTE 256
#define PNG_FourCC(a, b, c, d) ((UInt32)a << 24) | ((UInt32)b << 16) | ((UInt32)c << 8) | (UInt32)d

enum PNG_COL {
	PNG_COL_GRAYSCALE = 0, PNG_COL_RGB = 2, PNG_COL_INDEXED = 3,
	PNG_COL_GRAYSCALE_A = 4, PNG_COL_RGB_A = 6,
};

enum PNG_FILTER {
	PNG_FILTER_NONE, PNG_FILTER_SUB, PNG_FILTER_UP,
	PNG_FILTER_AVERAGE, PNG_FILTER_PAETH
};

typedef void(*Png_RowExpander)(UInt8 bpp, Int32 width, UInt32* palette, UInt8* src, UInt32* dst);
UInt8 png_sig[PNG_SIG_SIZE] = { 137, 80, 78, 71, 13, 10, 26, 10 };

bool Bitmap_DetectPng(UInt8* data, UInt32 len) {
	if (len < PNG_SIG_SIZE) return false;
	Int32 i;

	for (i = 0; i < PNG_SIG_SIZE; i++) {
		if (data[i] != png_sig[i]) return false;
	}
	return true;
}

static void Png_Reconstruct(UInt8 type, UInt8 bytesPerPixel, UInt8* line, UInt8* prior, UInt32 lineLen) {
	UInt32 i, j;
	switch (type) {
	case PNG_FILTER_NONE:
		return;

	case PNG_FILTER_SUB:
		for (i = bytesPerPixel, j = 0; i < lineLen; i++, j++) {
			line[i] += line[j];
		}
		return;

	case PNG_FILTER_UP:
		for (i = 0; i < lineLen; i++) {
			line[i] += prior[i];
		}
		return;

	case PNG_FILTER_AVERAGE:
		for (i = 0; i < bytesPerPixel; i++) {
			line[i] += (prior[i] >> 1);
		}
		for (j = 0; i < lineLen; i++, j++) {
			line[i] += ((prior[i] + line[j]) >> 1);
		}
		return;

	case PNG_FILTER_PAETH:
		/* TODO: verify this is right */
		for (i = 0; i < bytesPerPixel; i++) {
			line[i] += prior[i];
		}
		for (j = 0; i < lineLen; i++, j++) {
			UInt8 a = line[j], b = prior[i], c = prior[j];
			Int32 p = a + b - c;
			Int32 pa = Math_AbsI(p - a);
			Int32 pb = Math_AbsI(p - b);
			Int32 pc = Math_AbsI(p - c);

			if (pa <= pb && pa <= pc) { line[i] += a; } 
			else if (pb <= pc) {        line[i] += b; } 
			else {                      line[i] += c; }
		}
		return;

	default:
		ErrorHandler_Fail("PNG scanline has invalid filter type");
		return;
	}
}

static void Png_Expand_GRAYSCALE(UInt8 bitsPerSample, Int32 width, UInt32* palette, UInt8* src, UInt32* dst) {
	Int32 i, j, mask;
	UInt8 cur, rgb1, rgb2, rgb3, rgb4;
#define PNG_Do_Grayscale(tmp, dstI, srcI, scale) tmp = src[srcI] * scale; dst[dstI] = PackedCol_ARGB(tmp, tmp, tmp, 255);
#define PNG_Do_Grayscale_X(tmp, dstI, srcI) tmp = src[srcI]; dst[dstI] = PackedCol_ARGB(tmp, tmp, tmp, 255);

	switch (bitsPerSample) {
	case 1:
		for (i = 0, j = 0; i < (width & ~0x7); i += 8, j++) {
			cur = src[j];
			PNG_Do_Grayscale(rgb1, i    , (cur >> 7)    , 255); PNG_Do_Grayscale(rgb2, i + 1, (cur >> 6) & 1, 255);
			PNG_Do_Grayscale(rgb3, i + 2, (cur >> 5) & 1, 255); PNG_Do_Grayscale(rgb4, i + 3, (cur >> 4) & 1, 255);
			PNG_Do_Grayscale(rgb1, i + 4, (cur >> 3) & 1, 255); PNG_Do_Grayscale(rgb2, i + 5, (cur >> 2) & 1, 255);
			PNG_Do_Grayscale(rgb3, i + 6, (cur >> 1) & 1, 255); PNG_Do_Grayscale(rgb4, i + 7, (cur     ) & 1, 255);
		}
		for (; i < width; i++) {
			mask = (7 - (i & 7));
			PNG_Do_Grayscale(rgb1, i, (src[j] >> mask) & 1, 255);
		}
		return;
	case 2:
		for (i = 0, j = 0; i < (width & ~0x3); i += 4, j++) {
			cur = src[j];
			PNG_Do_Grayscale(rgb1, i    , (cur >> 6)    , 85); PNG_Do_Grayscale(rgb2, i + 1, (cur >> 4) & 3, 85);
			PNG_Do_Grayscale(rgb3, i + 2, (cur >> 2) & 3, 85); PNG_Do_Grayscale(rgb4, i + 3, (cur     ) & 3, 85);
		}
		for (; i < width; i++) {
			mask = (3 - (i & 3)) * 2;
			PNG_Do_Grayscale(rgb1, i, (src[j] >> mask) & 3, 85);
		}
		return;
	case 4:
		for (i = 0, j = 0; i < (width & ~0x1); i += 2, j++) {
			cur = src[j];
			PNG_Do_Grayscale(rgb1, i, cur >> 4, 17); PNG_Do_Grayscale(rgb1, i + 1, cur & 0x0F, 17);
		}
		for (; i < width; i++) {
			mask = (1 - (i & 1)) * 4;
			PNG_Do_Grayscale(rgb1, i, (src[j] >> mask) & 15, 17);
		}
		return;
	case 8:
		for (i = 0; i < (width & ~0x3); i += 4) {
			PNG_Do_Grayscale_X(rgb1, i    , i    ); PNG_Do_Grayscale_X(rgb2, i + 1, i + 1);
			PNG_Do_Grayscale_X(rgb3, i + 2, i + 2); PNG_Do_Grayscale_X(rgb4, i + 3, i + 3);
		}
		for (; i < width; i++) { PNG_Do_Grayscale_X(rgb1, i, i); }
		return;
	case 16:
		for (i = 0; i < width; i++) { PNG_Do_Grayscale_X(rgb1, i, i << 1); }
		return;
	}
}

static void Png_Expand_RGB(UInt8 bitsPerSample, Int32 width, UInt32* palette, UInt8* src, UInt32* dst) {
	Int32 i, j;
#define PNG_Do_RGB__8(dstI, srcI) dst[dstI] = PackedCol_ARGB(src[srcI], src[srcI + 1], src[srcI + 2], 255);
#define PNG_Do_RGB_16(dstI, srcI) dst[dstI] = PackedCol_ARGB(src[srcI], src[srcI + 3], src[srcI + 5], 255);

	if (bitsPerSample == 8) {
		for (i = 0, j = 0; i < (width & ~0x03); i += 4, j += 12) {
			PNG_Do_RGB__8(i    , j    ); PNG_Do_RGB__8(i + 1, j + 3);
			PNG_Do_RGB__8(i + 2, j + 6); PNG_Do_RGB__8(i + 3, j + 9);
		}
		for (; i < width; i++, j += 3) { PNG_Do_RGB__8(i, j); }
	} else {
		for (i = 0, j = 0; i < width; i++, j += 6) { PNG_Do_RGB_16(i, j); }
	}
}

static void Png_Expand_INDEXED(UInt8 bitsPerSample, Int32 width, UInt32* palette, UInt8* src, UInt32* dst) {
	Int32 i, j, mask;
	UInt8 cur;
#define PNG_Do_Indexed(dstI, srcI) dst[dstI] = palette[srcI];

	switch (bitsPerSample) {
	case 1:
		for (i = 0, j = 0; i < (width & ~0x7); i += 8, j++) {
			cur = src[j];
			PNG_Do_Indexed(i    , (cur >> 7)    ); PNG_Do_Indexed(i + 1, (cur >> 6) & 1);
			PNG_Do_Indexed(i + 2, (cur >> 5) & 1); PNG_Do_Indexed(i + 3, (cur >> 4) & 1);
			PNG_Do_Indexed(i + 4, (cur >> 3) & 1); PNG_Do_Indexed(i + 5, (cur >> 2) & 1);
			PNG_Do_Indexed(i + 6, (cur >> 1) & 1); PNG_Do_Indexed(i + 7, (cur     ) & 1);
		}
		for (; i < width; i++) {
			mask = (7 - (i & 7));
			PNG_Do_Indexed(i, (src[j] >> mask) & 1);
		}
		return;
	case 2:
		for (i = 0, j = 0; i < (width & ~0x3); i += 4, j++) {
			cur = src[j];
			PNG_Do_Indexed(i    , (cur >> 6)    ); PNG_Do_Indexed(i + 1, (cur >> 4) & 3);
			PNG_Do_Indexed(i + 2, (cur >> 2) & 3); PNG_Do_Indexed(i + 3, (cur     ) & 3);
		}
		for (; i < width; i++) {
			mask = (3 - (i & 3)) * 2;
			PNG_Do_Indexed(i, (src[j] >> mask) & 3);
		}
		return;
	case 4:
		for (i = 0, j = 0; i < (width & ~0x1); i += 2, j++) {
			cur = src[j];
			PNG_Do_Indexed(i, cur >> 4); PNG_Do_Indexed(i + 1, cur & 0x0F);
		}
		for (; i < width; i++) {
			mask = (1 - (i & 1)) * 4;
			PNG_Do_Indexed(i, (src[j] >> mask) & 15);
		}
		return;
	case 8:
		for (i = 0; i < (width & ~0x3); i += 4) {
			PNG_Do_Indexed(i    , src[i]    ); PNG_Do_Indexed(i + 1, src[i + 1]);
			PNG_Do_Indexed(i + 2, src[i + 2]); PNG_Do_Indexed(i + 3, src[i + 3]);
		}
		for (; i < width; i++) { PNG_Do_Indexed(i, src[i]); }
		return;
	}
}

static void Png_Expand_GRAYSCALE_A(UInt8 bitsPerSample, Int32 width, UInt32* palette, UInt8* src, UInt32* dst) {
	Int32 i, j;
	UInt8 rgb1, rgb2, rgb3, rgb4;
#define PNG_Do_Grayscale_A__8(tmp, dstI, srcI) tmp = src[srcI]; dst[dstI] = PackedCol_ARGB(tmp, tmp, tmp, src[srcI + 1]);
#define PNG_Do_Grayscale_A_16(tmp, dstI, srcI) tmp = src[srcI]; dst[dstI] = PackedCol_ARGB(tmp, tmp, tmp, src[srcI + 2]);

	if (bitsPerSample == 8) {
		for (i = 0, j = 0; i < (width & ~0x3); i += 4, j += 8) {
			PNG_Do_Grayscale_A__8(rgb1, i    , j    ); PNG_Do_Grayscale_A__8(rgb2, i + 1, j + 2);
			PNG_Do_Grayscale_A__8(rgb3, i + 2, j + 4); PNG_Do_Grayscale_A__8(rgb4, i + 3, j + 6);
		}
		for (; i < width; i++, j += 2) { PNG_Do_Grayscale_A__8(rgb1, i, j); }
	} else {
		for (i = 0, j = 0; i < width; i++, j += 4) { PNG_Do_Grayscale_A_16(rgb1, i, j); }
	}
}

static void Png_Expand_RGB_A(UInt8 bitsPerSample, Int32 width, UInt32* palette, UInt8* src, UInt32* dst) {
	Int32 i, j;
#define PNG_Do_RGB_A__8(dstI, srcI) dst[dstI] = PackedCol_ARGB(src[srcI], src[srcI + 1], src[srcI + 2], src[srcI + 3]);
#define PNG_Do_RGB_A_16(dstI, srcI) dst[dstI] = PackedCol_ARGB(src[srcI], src[srcI + 3], src[srcI + 5], src[srcI + 7]);

	if (bitsPerSample == 8) {
		for (i = 0, j = 0; i < (width & ~0x3); i += 4, j += 16) {
			PNG_Do_RGB_A__8(i    , j    ); PNG_Do_RGB_A__8(i + 1, j + 4 );
			PNG_Do_RGB_A__8(i + 2, j + 8); PNG_Do_RGB_A__8(i + 3, j + 12);
		}
		for (; i < width; i++, j += 4) { PNG_Do_RGB_A__8(i, j); }
	} else {
		for (i = 0, j = 0; i < width; i++, j += 8) { PNG_Do_RGB_A_16(i, j); }
	}
}

static void Png_ComputeTransparency(struct Bitmap* bmp, UInt32 transparentCol) {
	UInt32 trnsRGB = transparentCol & PNG_RGB_MASK;
	Int32 x, y, width = bmp->Width, height = bmp->Height;

	for (y = 0; y < height; y++) {
		UInt32* row = Bitmap_GetRow(bmp, y);
		for (x = 0; x < width; x++) {
			UInt32 rgb = row[x] & PNG_RGB_MASK;
			row[x] = (rgb == trnsRGB) ? trnsRGB : row[x];
		}
	}
}

/* Most bits per sample is 16. Most samples per pixel is 4. Add 1 for filter byte. */
#define PNG_BUFFER_SIZE ((PNG_MAX_DIMS * 2 * 4 + 1) * 2)
/* TODO: Test a lot of .png files and ensure output is right */
ReturnCode Bitmap_DecodePng(struct Bitmap* bmp, struct Stream* stream) {
	Bitmap_Create(bmp, 0, 0, NULL);

	UInt8 header[PNG_SIG_SIZE];
	Stream_Read(stream, header, PNG_SIG_SIZE);
	if (!Bitmap_DetectPng(header, PNG_SIG_SIZE)) return PNG_ERR_INVALID_SIG;

	UInt32 transparentCol = PackedCol_ARGB(0, 0, 0, 255);
	UInt8 col, bitsPerSample, bytesPerPixel;
	Png_RowExpander rowExpander;

	UInt32 palette[PNG_PALETTE];
	UInt32 i;
	for (i = 0; i < PNG_PALETTE; i++) {
		palette[i] = PackedCol_ARGB(0, 0, 0, 255);
	}
	bool gotHeader = false, readingChunks = true;

	struct InflateState inflate;
	struct Stream compStream;
	Inflate_MakeStream(&compStream, &inflate, stream);
	struct ZLibHeader zlibHeader;
	ZLibHeader_Init(&zlibHeader);

	UInt32 scanlineSize, scanlineBytes, curY = 0;
	UInt8 buffer[PNG_BUFFER_SIZE];
	UInt32 bufferIdx, bufferRows;

	while (readingChunks) {
		UInt32 dataSize = Stream_ReadU32_BE(stream);
		UInt32 fourCC   = Stream_ReadU32_BE(stream);

		switch (fourCC) {
		case PNG_FourCC('I', 'H', 'D', 'R'): {
			if (dataSize != PNG_IHDR_SIZE) return PNG_ERR_INVALID_HEADER_SIZE;
			gotHeader = true;

			bmp->Width  = Stream_ReadI32_BE(stream);
			bmp->Height = Stream_ReadI32_BE(stream);
			if (bmp->Width  < 0 || bmp->Width  > PNG_MAX_DIMS) return PNG_ERR_TOO_WIDE;
			if (bmp->Height < 0 || bmp->Height > PNG_MAX_DIMS) return PNG_ERR_TOO_TALL;

			bmp->Stride = bmp->Width * BITMAP_SIZEOF_PIXEL;
			bmp->Scan0 = Platform_MemAlloc(bmp->Width * bmp->Height, BITMAP_SIZEOF_PIXEL);
			if (bmp->Scan0 == NULL) ErrorHandler_Fail("Failed to allocate memory for PNG bitmap");

			bitsPerSample = Stream_ReadU8(stream);
			if (bitsPerSample > 16 || !Math_IsPowOf2(bitsPerSample)) return PNG_ERR_INVALID_BPP;
			col = Stream_ReadU8(stream);
			if (col == 1 || col == 5 || col > 6) return PNG_ERR_INVALID_COL;
			if (bitsPerSample < 8 && (col >= PNG_COL_RGB && col != PNG_COL_INDEXED)) return PNG_ERR_INVALID_COL_BPP;
			if (bitsPerSample == 16 && col == PNG_COL_INDEXED) return PNG_ERR_INVALID_COL_BPP;

			if (Stream_ReadU8(stream) != 0) return PNG_ERR_COMP_METHOD;
			if (Stream_ReadU8(stream) != 0) return PNG_ERR_FILTER;
			if (Stream_ReadU8(stream) != 0) return PNG_ERR_INTERLACED;

			static UInt32 samplesPerPixel[7] = { 1, 0, 3, 1, 2, 0, 4 };
			bytesPerPixel = ((samplesPerPixel[col] * bitsPerSample) + 7) >> 3;
			scanlineSize = ((samplesPerPixel[col] * bitsPerSample * bmp->Width) + 7) >> 3;
			scanlineBytes = scanlineSize + 1; /* Add 1 byte for filter byte of each scanline */			

			Platform_MemSet(buffer, 0, scanlineBytes); /* Prior row should be 0 per PNG spec */
			bufferIdx = scanlineBytes;
			bufferRows = PNG_BUFFER_SIZE / scanlineBytes;

			switch (col) {
			case PNG_COL_GRAYSCALE: rowExpander = Png_Expand_GRAYSCALE; break;
			case PNG_COL_RGB: rowExpander = Png_Expand_RGB; break;
			case PNG_COL_INDEXED: rowExpander = Png_Expand_INDEXED; break;
			case PNG_COL_GRAYSCALE_A: rowExpander = Png_Expand_GRAYSCALE_A; break;
			case PNG_COL_RGB_A: rowExpander = Png_Expand_RGB_A; break;
			}
		} break;

		case PNG_FourCC('P', 'L', 'T', 'E'): {
			if (dataSize > PNG_PALETTE * 3) return PNG_ERR_PAL_ENTRIES;
			if ((dataSize % 3) != 0) return PNG_ERR_PAL_SIZE;

			UInt8 palRGB[PNG_PALETTE * 3];
			Stream_Read(stream, palRGB, dataSize);
			for (i = 0; i < dataSize; i += 3) {
				palette[i / 3] = PackedCol_ARGB(palRGB[i], palRGB[i + 1], palRGB[i + 2], 255);
			}
		} break;

		case PNG_FourCC('t', 'R', 'N', 'S'): {
			if (col == PNG_COL_GRAYSCALE) {
				if (dataSize != 2) return PNG_ERR_TRANS_COUNT;
				UInt8 palRGB = (UInt8)Stream_ReadU16_BE(stream);
				transparentCol = PackedCol_ARGB(palRGB, palRGB, palRGB, 0);
			} else if (col == PNG_COL_INDEXED) {
				if (dataSize > PNG_PALETTE) return PNG_ERR_TRANS_COUNT;
				UInt8 palA[PNG_PALETTE * 3];
				Stream_Read(stream, palA, dataSize);

				for (i = 0; i < dataSize; i++) {
					palette[i] &= PNG_RGB_MASK;
					palette[i] |= (UInt32)palA[i] << 24;
				}
			} else if (col == PNG_COL_RGB) {
				if (dataSize != 6) return PNG_ERR_TRANS_COUNT;
				UInt8 palR = (UInt8)Stream_ReadU16_BE(stream);
				UInt8 palG = (UInt8)Stream_ReadU16_BE(stream);
				UInt8 palB = (UInt8)Stream_ReadU16_BE(stream);
				transparentCol = PackedCol_ARGB(palR, palG, palB, 0);
			} else {
				return PNG_ERR_TRANS_INVALID;
			}
		} break;

		case PNG_FourCC('I', 'D', 'A', 'T'): {
			struct Stream datStream;
			Stream_ReadonlyPortion(&datStream, stream, dataSize);
			inflate.Source = &datStream;
			/* TODO: This assumes zlib header will be in 1 IDAT chunk */
			while (!zlibHeader.Done) { ZLibHeader_Read(&datStream, &zlibHeader); }

			UInt32 bufferLen = bufferRows * scanlineBytes, bufferMax = bufferLen - scanlineBytes;
			while (curY < bmp->Height) {
				/* Need to leave one row in buffer untouched for storing prior scanline. Illustrated example of process:
				*          |=====|        #-----|        |-----|        #-----|        |-----|
				* initial  #-----| read 3 |-----| read 3 |-----| read 1 |-----| read 3 |-----| etc
				*  state   |-----| -----> |-----| -----> |=====| -----> |-----| -----> |=====|
				*          |-----|        |=====|        #-----|        |=====|        #-----|
				*
				* (==== is prior scanline, # is current index in buffer)
				* Having initial state this way allows doing two 'read 3' first time (assuming large idat chunks)
				*/
				UInt32 bufferLeft = bufferLen - bufferIdx, read;
				if (bufferLeft > bufferMax) bufferLeft = bufferMax;

				ReturnCode code = compStream.Read(&compStream, &buffer[bufferIdx], bufferLeft, &read);
				ErrorHandler_CheckOrFail(code, "PNG - reading image bulk data");
				if (read == 0) break;

				UInt32 startY = bufferIdx / scanlineBytes, rowY;
				bufferIdx += read;
				UInt32 endY = bufferIdx / scanlineBytes;
				/* reached end of buffer, cycle back to start */
				if (bufferIdx == bufferLen) bufferIdx = 0;

				for (rowY = startY; rowY < endY; rowY++, curY++) {
					UInt32 priorY = rowY == 0 ? bufferRows : rowY;
					UInt8* prior    = &buffer[(priorY - 1) * scanlineBytes];
					UInt8* scanline = &buffer[rowY         * scanlineBytes];

					Png_Reconstruct(scanline[0], bytesPerPixel, &scanline[1], &prior[1], scanlineSize);
					rowExpander(bitsPerSample, bmp->Width, palette, &scanline[1], Bitmap_GetRow(bmp, curY));
				}			
			}
		} break;

		case PNG_FourCC('I', 'E', 'N', 'D'): {
			readingChunks = false;
			if (dataSize != 0) return PNG_ERR_INVALID_END_SIZE;
		} break;

		default: {
			ReturnCode result = Stream_Skip(stream, dataSize);
			if (result != 0) return PNG_ERR_SKIPPING_CHUNK;
		} break;
		}

		Stream_ReadU32_BE(stream); /* Skip CRC32 */
	}

	if (transparentCol <= PNG_RGB_MASK) {
		Png_ComputeTransparency(bmp, transparentCol);
	}
	if (bmp->Scan0 == NULL) ErrorHandler_Fail("Invalid PNG image");
}


/*########################################################################################################################*
*------------------------------------------------------PNG encoder--------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Bitmap_Crc32StreamWrite(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	UInt32 i, crc32 = stream->Meta_CRC32;
	/* TODO: Optimise this calculation */
	for (i = 0; i < count; i++) {
		crc32 = Utils_Crc32Table[(crc32 ^ data[i]) & 0xFF] ^ (crc32 >> 8);
	}
	stream->Meta_CRC32 = crc32;

	struct Stream* underlying = stream->Meta_CRC32_Source;
	return underlying->Write(underlying, data, count, modified);
}

static void Bitmap_Crc32Stream(struct Stream* stream, struct Stream* underlying) {
	Stream_SetName(stream, &underlying->Name);
	stream->Meta_CRC32_Source = underlying;
	stream->Meta_CRC32 = 0xFFFFFFFFUL;

	Stream_SetDefaultOps(stream);
	stream->Write = Bitmap_Crc32StreamWrite;
}

static void Png_Filter(UInt8 filter, UInt8* cur, UInt8* prior, UInt8* best, Int32 lineLen) {
	/* 3 bytes per pixel constant */
	Int32 i;
	switch (filter) {
	case PNG_FILTER_NONE:
		Platform_MemCpy(best, cur, lineLen);
		break;

	case PNG_FILTER_SUB:
		best[0] = cur[0]; best[1] = cur[1]; best[2] = cur[2];

		for (i = 3; i < lineLen; i++) {
			best[i] = cur[i] - cur[i - 3];
		}
		break;

	case PNG_FILTER_UP:
		for (i = 0; i < lineLen; i++) {
			best[i] = cur[i] - prior[i];
		}
		break;

	case PNG_FILTER_AVERAGE:
		best[0] = cur[0] - (prior[0] >> 1);
		best[1] = cur[1] - (prior[1] >> 1);
		best[2] = cur[2] - (prior[2] >> 1);

		for (i = 3; i < lineLen; i++) {
			best[i] = cur[i] - ((prior[i] + cur[i - 3]) >> 1);
		}
		break;

	case PNG_FILTER_PAETH:
		best[0] = cur[0] - prior[0]; 
		best[1] = cur[1] - prior[1];
		best[2] = cur[2] - prior[2];

		for (i = 3; i < lineLen; i++) {
			UInt8 a = cur[i - 3], b = prior[i], c = prior[i - 3];
			Int32 p = a + b - c;
			Int32 pa = Math_AbsI(p - a);
			Int32 pb = Math_AbsI(p - b);
			Int32 pc = Math_AbsI(p - c);

			if (pa <= pb && pa <= pc) { best[i] = cur[i] - a; }
			else if (pb <= pc)        { best[i] = cur[i] - b; }
			else                      { best[i] = cur[i] - c; }
		}
		break;
	}
}

static void Png_EncodeRow(UInt8* src, UInt8* cur, UInt8* prior, UInt8* best, Int32 lineLen) {
	UInt8* dst = cur;
	Int32 x;
	for (x = 0; x < lineLen; x += 3) {
		dst[0] = src[2]; dst[1] = src[1]; dst[2] = src[0];
		src += 4; dst += 3;
	}

	/* Waste of time checking the PNG_NONE filter */
	Int32 filter, bestFilter = PNG_FILTER_SUB, bestEstimate = Int32_MaxValue;
	dst = best + 1;

	for (filter = PNG_FILTER_SUB; filter <= PNG_FILTER_PAETH; filter++) {
		Png_Filter(filter, cur, prior, dst, lineLen);

		/* Estimate how well this filtered line will compress, based on */
		/* smallest sum of magnitude of each byte (signed) in the line */
		/* (see note in PNG specification, 12.8 "Filter selection" ) */
		Int32 estimate = 0;
		for (x = 0; x < lineLen; x++) {
			estimate += Math_AbsI((Int8)dst[x]);
		}

		if (estimate > bestEstimate) continue;
		bestEstimate = estimate;
		bestFilter   = filter;
	}

	/* Since this filter is last checked, can avoid running it twice */
	if (bestFilter != PNG_FILTER_PAETH) {
		Png_Filter(bestFilter, cur, prior, dst, lineLen);
	}
	best[0] = bestFilter;
}

void Bitmap_EncodePng(struct Bitmap* bmp, struct Stream* stream) {
	Stream_Write(stream, png_sig, PNG_SIG_SIZE);
	struct Stream* underlying = stream;
	struct Stream crc32Stream;

	/* Write header chunk */
	Stream_WriteU32_BE(stream, PNG_IHDR_SIZE);
	Bitmap_Crc32Stream(&crc32Stream, underlying);
	stream = &crc32Stream;
	Stream_WriteU32_BE(stream, PNG_FourCC('I', 'H', 'D', 'R'));
	{
		Stream_WriteU32_BE(stream, bmp->Width);
		Stream_WriteU32_BE(stream, bmp->Height);
		Stream_WriteU8(stream, 8); /* bits per sample */
		Stream_WriteU8(stream, PNG_COL_RGB); /* TODO: RGBA but mask all alpha to 255? */
		Stream_WriteU8(stream, 0); /* DEFLATE compression method */
		Stream_WriteU8(stream, 0); /* ADAPTIVE filter method */
		Stream_WriteU8(stream, 0); /* Not using interlacing */
	}
	stream = underlying;
	Stream_WriteU32_BE(stream, crc32Stream.Meta_CRC32 ^ 0xFFFFFFFFUL);

	/* Write PNG body */
	UInt8 prevLine[PNG_MAX_DIMS * 3];
	UInt8 curLine[PNG_MAX_DIMS * 3];
	UInt8 bestLine[PNG_MAX_DIMS * 3 + 1];
	Platform_MemSet(prevLine, 0, bmp->Width * 3);

	Stream_WriteU32_BE(stream, 0);
	stream = &crc32Stream;
	Bitmap_Crc32Stream(&crc32Stream, underlying);
	Stream_WriteU32_BE(stream, PNG_FourCC('I', 'D', 'A', 'T'));
	{
		Int32 y, lineSize = bmp->Width * 3;
		struct ZLibState zlState;
		struct Stream zlStream;
		ZLib_MakeStream(&zlStream, &zlState, stream);

		for (y = 0; y < bmp->Height; y++) {
			UInt8* src  = (UInt8*)Bitmap_GetRow(bmp, y);
			UInt8* prev = (y & 1) == 0 ? prevLine : curLine;
			UInt8* cur  = (y & 1) == 0 ? curLine : prevLine;
			Png_EncodeRow(src, cur, prev, bestLine, lineSize);
			Stream_Write(&zlStream, bestLine, lineSize + 1); /* +1 for filter byte */
		}
		zlStream.Close(&zlStream);
	}
	UInt32 dataEnd; ReturnCode result = underlying->Position(underlying, &dataEnd);
	ErrorHandler_CheckOrFail(result, "PNG - getting position of data end");
	stream = underlying;
	Stream_WriteU32_BE(stream, crc32Stream.Meta_CRC32 ^ 0xFFFFFFFFUL);

	/* Write end chunk */
	Stream_WriteU32_BE(stream, 0);
	Stream_WriteU32_BE(stream, PNG_FourCC('I', 'E', 'N', 'D'));
	Stream_WriteU32_BE(stream, 0xAE426082UL); /* crc32 of iend */

	/* Come back to write size of data chunk */
	result = stream->Seek(stream, 33, STREAM_SEEKFROM_BEGIN);
	ErrorHandler_CheckOrFail(result, "PNG - seeking to write data size");
	Stream_WriteU32_BE(stream, dataEnd - 41);
}
