#include "Bitmap.h"
#include "Platform.h"
#include "PackedCol.h"
#include "ExtMath.h"
#include "Deflate.h"
#include "ErrorHandler.h"
#include "Stream.h"
#include "Errors.h"

void Bitmap_Create(Bitmap* bmp, int width, int height, uint8_t* scan0) {
	bmp->Width = width; bmp->Height = height; bmp->Scan0 = scan0;
}

void Bitmap_CopyBlock(int srcX, int srcY, int dstX, int dstY, Bitmap* src, Bitmap* dst, int size) {
	int x, y;
	for (y = 0; y < size; y++) {
		uint32_t* srcRow = Bitmap_GetRow(src, srcY + y);
		uint32_t* dstRow = Bitmap_GetRow(dst, dstY + y);
		for (x = 0; x < size; x++) {
			dstRow[dstX + x] = srcRow[srcX + x];
		}
	}
}

void Bitmap_Allocate(Bitmap* bmp, int width, int height) {
	bmp->Width = width; bmp->Height = height;
	bmp->Scan0 = Mem_Alloc(width * height, BITMAP_SIZEOF_PIXEL, "bitmap data");
}

void Bitmap_AllocateClearedPow2(Bitmap* bmp, int width, int height) {
	width  = Math_NextPowOf2(width);
	height = Math_NextPowOf2(height);

	bmp->Width = width; bmp->Height = height;
	bmp->Scan0 = Mem_AllocCleared(width * height, BITMAP_SIZEOF_PIXEL, "bitmap data");
}


/*########################################################################################################################*
*------------------------------------------------------PNG decoder--------------------------------------------------------*
*#########################################################################################################################*/
#define PNG_SIG_SIZE 8
#define PNG_IHDR_SIZE 13
#define PNG_RGB_MASK 0xFFFFFFUL
#define PNG_PALETTE 256
#define PNG_FourCC(a, b, c, d) (((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | (uint32_t)d)

enum PngCol {
	PNG_COL_GRAYSCALE = 0, PNG_COL_RGB = 2, PNG_COL_INDEXED = 3,
	PNG_COL_GRAYSCALE_A = 4, PNG_COL_RGB_A = 6
};

enum PngFilter {
	PNG_FILTER_NONE, PNG_FILTER_SUB, PNG_FILTER_UP, PNG_FILTER_AVERAGE, PNG_FILTER_PAETH
};

typedef void (*Png_RowExpander)(int width, uint32_t* palette, uint8_t* src, uint32_t* dst);
static uint8_t png_sig[PNG_SIG_SIZE] = { 137, 80, 78, 71, 13, 10, 26, 10 };

bool Png_Detect(const uint8_t* data, uint32_t len) {
	int i;
	if (len < PNG_SIG_SIZE) return false;

	for (i = 0; i < PNG_SIG_SIZE; i++) {
		if (data[i] != png_sig[i]) return false;
	}
	return true;
}

static void Png_Reconstruct(uint8_t type, uint8_t bytesPerPixel, uint8_t* line, uint8_t* prior, uint32_t lineLen) {
	uint32_t i, j;
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
			uint8_t a = line[j], b = prior[i], c = prior[j];
			int p = a + b - c;
			int pa = Math_AbsI(p - a);
			int pb = Math_AbsI(p - b);
			int pc = Math_AbsI(p - c);

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

static void Png_Expand_GRAYSCALE_1(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i; /* NOTE: not optimised*/
#define PNG_Do_Grayscale(tmp, dstI, srcI, scale) tmp = src[srcI] * scale; dst[dstI] = PackedCol_ARGB(tmp, tmp, tmp, 255);
	for (i = 0; i < width; i++) {
		int mask = (7 - (i & 7)); uint8_t rgb;
		PNG_Do_Grayscale(rgb, i, (src[i >> 3] >> mask) & 1, 255);
	}
}

static void Png_Expand_GRAYSCALE_2(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i; /* NOTE: not optimised */
	for (i = 0; i < width; i++) {
		int mask = (3 - (i & 3)) * 2; uint8_t rgb;
		PNG_Do_Grayscale(rgb, i, (src[i >> 3] >> mask) & 3, 85);
	}
}

static void Png_Expand_GRAYSCALE_4(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i, j, mask; uint8_t cur, rgb1, rgb2;

	for (i = 0, j = 0; i < (width & ~0x1); i += 2, j++) {
		cur = src[j];
		PNG_Do_Grayscale(rgb1, i, cur >> 4, 17); PNG_Do_Grayscale(rgb2, i + 1, cur & 0x0F, 17);
	}
	for (; i < width; i++) {
		mask = (1 - (i & 1)) * 4;
		PNG_Do_Grayscale(rgb1, i, (src[j] >> mask) & 15, 17);
	}
}

static void Png_Expand_GRAYSCALE_8(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i; uint8_t rgb1, rgb2, rgb3, rgb4;
#define PNG_Do_Grayscale_8(tmp, dstI, srcI) tmp = src[srcI]; dst[dstI] = PackedCol_ARGB(tmp, tmp, tmp, 255);

	for (i = 0; i < (width & ~0x3); i += 4) {
		PNG_Do_Grayscale_8(rgb1, i    , i    ); PNG_Do_Grayscale_8(rgb2, i + 1, i + 1);
		PNG_Do_Grayscale_8(rgb3, i + 2, i + 2); PNG_Do_Grayscale_8(rgb4, i + 3, i + 3);
	}
	for (; i < width; i++) { PNG_Do_Grayscale_8(rgb1, i, i); }
}

static void Png_Expand_GRAYSCALE_16(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i;
	for (i = 0; i < width; i++) {
		uint8_t rgb = src[i * 2];
		dst[i] = PackedCol_ARGB(rgb, rgb, rgb, 255);
	}
}

static void Png_Expand_RGB_8(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i, j;
#define PNG_Do_RGB__8(dstI, srcI) dst[dstI] = PackedCol_ARGB(src[srcI], src[srcI + 1], src[srcI + 2], 255);

	for (i = 0, j = 0; i < (width & ~0x03); i += 4, j += 12) {
		PNG_Do_RGB__8(i    , j    ); PNG_Do_RGB__8(i + 1, j + 3);
		PNG_Do_RGB__8(i + 2, j + 6); PNG_Do_RGB__8(i + 3, j + 9);
	}
	for (; i < width; i++, j += 3) { PNG_Do_RGB__8(i, j); }
}

static void Png_Expand_RGB_16(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i, j;
	for (i = 0, j = 0; i < width; i++, j += 6) { 
		dst[i] = PackedCol_ARGB(src[j], src[j + 2], src[j + 4], 255);
	}
}

static void Png_Expand_INDEXED_1(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i; /* NOTE: not optimised*/
	for (i = 0; i < width; i++) {
		int mask = (7 - (i & 7));
		dst[i] = palette[(src[i >> 3] >> mask) & 1];
	}
}

static void Png_Expand_INDEXED_2(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i; /* NOTE: not optimised*/
	for (i = 0; i < width; i++) {
		int mask = (3 - (i & 3)) * 2;
		dst[i] = palette[(src[i >> 3] >> mask) & 3];
	}
}

static void Png_Expand_INDEXED_4(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i, j, mask; uint8_t cur;
#define PNG_Do_Indexed(dstI, srcI) dst[dstI] = palette[srcI];

	for (i = 0, j = 0; i < (width & ~0x1); i += 2, j++) {
		cur = src[j];
		PNG_Do_Indexed(i, cur >> 4); PNG_Do_Indexed(i + 1, cur & 0x0F);
	}
	for (; i < width; i++) {
		mask = (1 - (i & 1)) * 4;
		PNG_Do_Indexed(i, (src[j] >> mask) & 15);
	}
}

static void Png_Expand_INDEXED_8(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i;

	for (i = 0; i < (width & ~0x3); i += 4) {
		PNG_Do_Indexed(i    , src[i]    ); PNG_Do_Indexed(i + 1, src[i + 1]);
		PNG_Do_Indexed(i + 2, src[i + 2]); PNG_Do_Indexed(i + 3, src[i + 3]);
	}
	for (; i < width; i++) { PNG_Do_Indexed(i, src[i]); }
}

static void Png_Expand_GRAYSCALE_A_8(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i, j; uint8_t rgb1, rgb2, rgb3, rgb4;
#define PNG_Do_Grayscale_A__8(tmp, dstI, srcI) tmp = src[srcI]; dst[dstI] = PackedCol_ARGB(tmp, tmp, tmp, src[srcI + 1]);

	for (i = 0, j = 0; i < (width & ~0x3); i += 4, j += 8) {
		PNG_Do_Grayscale_A__8(rgb1, i    , j    ); PNG_Do_Grayscale_A__8(rgb2, i + 1, j + 2);
		PNG_Do_Grayscale_A__8(rgb3, i + 2, j + 4); PNG_Do_Grayscale_A__8(rgb4, i + 3, j + 6);
	}
	for (; i < width; i++, j += 2) { PNG_Do_Grayscale_A__8(rgb1, i, j); }
}

static void Png_Expand_GRAYSCALE_A_16(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i, j; /* NOTE: not optimised*/
	for (i = 0, j = 0; i < width; i++, j += 4) {
		uint8_t rgb = src[j];
		dst[i] = PackedCol_ARGB(rgb, rgb, rgb, src[j + 2]);
	}
}

static void Png_Expand_RGB_A_8(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i, j;
#define PNG_Do_RGB_A__8(dstI, srcI) dst[dstI] = PackedCol_ARGB(src[srcI], src[srcI + 1], src[srcI + 2], src[srcI + 3]);

	for (i = 0, j = 0; i < (width & ~0x3); i += 4, j += 16) {
		PNG_Do_RGB_A__8(i    , j    ); PNG_Do_RGB_A__8(i + 1, j + 4 );
		PNG_Do_RGB_A__8(i + 2, j + 8); PNG_Do_RGB_A__8(i + 3, j + 12);
	}
	for (; i < width; i++, j += 4) { PNG_Do_RGB_A__8(i, j); }
}

static void Png_Expand_RGB_A_16(int width, uint32_t* palette, uint8_t* src, uint32_t* dst) {
	int i, j; /* NOTE: not optimised*/
	for (i = 0, j = 0; i < width; i++, j += 8) { 
		dst[i] = PackedCol_ARGB(src[j], src[j + 2], src[j + 4], src[j + 6]);
	}
}

Png_RowExpander Png_GetExpander(uint8_t col, uint8_t bitsPerSample) {
	switch (col) {
	case PNG_COL_GRAYSCALE:
		switch (bitsPerSample) {
		case 1:  return Png_Expand_GRAYSCALE_1;
		case 2:  return Png_Expand_GRAYSCALE_2;
		case 4:  return Png_Expand_GRAYSCALE_4;
		case 8:  return Png_Expand_GRAYSCALE_8;
		case 16: return Png_Expand_GRAYSCALE_16;
		}
		return NULL;

	case PNG_COL_RGB:
		switch (bitsPerSample) {
		case 8:  return Png_Expand_RGB_8;
		case 16: return Png_Expand_RGB_16;
		}
		return NULL;

	case PNG_COL_INDEXED:
		switch (bitsPerSample) {
		case 1: return Png_Expand_INDEXED_1;
		case 2: return Png_Expand_INDEXED_2;
		case 4: return Png_Expand_INDEXED_4;
		case 8: return Png_Expand_INDEXED_8;
		}
		return NULL;

	case PNG_COL_GRAYSCALE_A:
		switch (bitsPerSample) {
		case 8:  return Png_Expand_GRAYSCALE_A_8;
		case 16: return Png_Expand_GRAYSCALE_A_16;
		}
		return NULL;

	case PNG_COL_RGB_A:
		switch (bitsPerSample) {
		case 8:  return Png_Expand_RGB_A_8;
		case 16: return Png_Expand_RGB_A_16;
		}
		return NULL;
	}
	return NULL;
}

static void Png_ComputeTransparency(Bitmap* bmp, uint32_t transparentCol) {
	uint32_t trnsRGB = transparentCol & PNG_RGB_MASK;
	int x, y, width = bmp->Width, height = bmp->Height;

	for (y = 0; y < height; y++) {
		uint32_t* row = Bitmap_GetRow(bmp, y);
		for (x = 0; x < width; x++) {
			uint32_t rgb = row[x] & PNG_RGB_MASK;
			row[x] = (rgb == trnsRGB) ? trnsRGB : row[x];
		}
	}
}

/* Most bits per sample is 16. Most samples per pixel is 4. Add 1 for filter byte. */
/* Need to store both current and prior row, per PNG specification. */
#define PNG_BUFFER_SIZE ((PNG_MAX_DIMS * 2 * 4 + 1) * 2)
/* TODO: Test a lot of .png files and ensure output is right */
ReturnCode Png_Decode(Bitmap* bmp, struct Stream* stream) {
	uint8_t tmp[PNG_PALETTE * 3];
	uint32_t dataSize, fourCC;
	ReturnCode res;

	/* header variables */
	uint8_t col, bitsPerSample, bytesPerPixel;
	Png_RowExpander rowExpander;
	uint32_t scanlineSize, scanlineBytes;

	/* palette data */
	uint32_t transparentCol = PackedCol_ARGB(0, 0, 0, 255);
	uint32_t palette[PNG_PALETTE];
	uint32_t i;

	/* idat state */
	uint32_t curY = 0, begY, rowY, endY;
	uint8_t buffer[PNG_BUFFER_SIZE];
	uint32_t bufferRows, bufferLen;
	uint32_t bufferIdx, read, left;

	/* idat decompressor */
	struct InflateState inflate;
	struct Stream compStream, datStream;
	struct ZLibHeader zlibHeader;

	Bitmap_Create(bmp, 0, 0, NULL);
	res = Stream_Read(stream, tmp, PNG_SIG_SIZE);
	if (res) return res;
	if (!Png_Detect(tmp, PNG_SIG_SIZE)) return PNG_ERR_INVALID_SIG;

	for (i = 0; i < PNG_PALETTE; i++) {
		palette[i] = PackedCol_ARGB(0, 0, 0, 255);
	}
	bool readingChunks = true;

	Inflate_MakeStream(&compStream, &inflate, stream);
	ZLibHeader_Init(&zlibHeader);

	while (readingChunks) {
		res = Stream_Read(stream, tmp, 8);
		if (res) return res;
		dataSize = Stream_GetU32_BE(&tmp[0]);
		fourCC   = Stream_GetU32_BE(&tmp[4]);

		switch (fourCC) {
		case PNG_FourCC('I','H','D','R'): {
			if (dataSize != PNG_IHDR_SIZE) return PNG_ERR_INVALID_HEADER_SIZE;
			res = Stream_Read(stream, tmp, PNG_IHDR_SIZE);
			if (res) return res;

			bmp->Width  = (int)Stream_GetU32_BE(&tmp[0]);
			bmp->Height = (int)Stream_GetU32_BE(&tmp[4]);
			if (bmp->Width  < 0 || bmp->Width  > PNG_MAX_DIMS) return PNG_ERR_TOO_WIDE;
			if (bmp->Height < 0 || bmp->Height > PNG_MAX_DIMS) return PNG_ERR_TOO_TALL;

			bmp->Scan0 = Mem_Alloc(bmp->Width * bmp->Height, BITMAP_SIZEOF_PIXEL, "PNG bitmap data");
			bitsPerSample = tmp[8]; col = tmp[9];
			rowExpander = Png_GetExpander(col, bitsPerSample);
			if (rowExpander == NULL) return PNG_ERR_INVALID_COL_BPP;

			if (tmp[10] != 0) return PNG_ERR_COMP_METHOD;
			if (tmp[11] != 0) return PNG_ERR_FILTER;
			if (tmp[12] != 0) return PNG_ERR_INTERLACED;

			static uint32_t samplesPerPixel[7] = { 1, 0, 3, 1, 2, 0, 4 };
			bytesPerPixel = ((samplesPerPixel[col] * bitsPerSample) + 7) >> 3;
			scanlineSize  = ((samplesPerPixel[col] * bitsPerSample * bmp->Width) + 7) >> 3;
			scanlineBytes = scanlineSize + 1; /* Add 1 byte for filter byte of each scanline */

			Mem_Set(buffer, 0, scanlineBytes); /* Prior row should be 0 per PNG spec */
			bufferIdx  = scanlineBytes;
			bufferRows = PNG_BUFFER_SIZE / scanlineBytes;
			bufferLen  = bufferRows * scanlineBytes;
		} break;

		case PNG_FourCC('P','L','T','E'): {
			if (dataSize > PNG_PALETTE * 3) return PNG_ERR_PAL_ENTRIES;
			if ((dataSize % 3) != 0) return PNG_ERR_PAL_SIZE;
			res = Stream_Read(stream, tmp, dataSize);
			if (res) return res;

			for (i = 0; i < dataSize; i += 3) {
				palette[i / 3] = PackedCol_ARGB(tmp[i], tmp[i + 1], tmp[i + 2], 255);
			}
		} break;

		case PNG_FourCC('t','R','N','S'): {
			if (col == PNG_COL_GRAYSCALE) {
				if (dataSize != 2) return PNG_ERR_TRANS_COUNT;
				res = Stream_Read(stream, tmp, dataSize);
				if (res) return res;

				/* RGB is 16 bits big endian, ignore least significant 8 bits */
				uint8_t palRGB = tmp[0];
				transparentCol = PackedCol_ARGB(palRGB, palRGB, palRGB, 0);
			} else if (col == PNG_COL_INDEXED) {
				if (dataSize > PNG_PALETTE) return PNG_ERR_TRANS_COUNT;
				res = Stream_Read(stream, tmp, dataSize);
				if (res) return res;

				/* set alpha component of palette*/
				for (i = 0; i < dataSize; i++) {
					palette[i] &= PNG_RGB_MASK;
					palette[i] |= (uint32_t)tmp[i] << 24;
				}
			} else if (col == PNG_COL_RGB) {
				if (dataSize != 6) return PNG_ERR_TRANS_COUNT;
				res = Stream_Read(stream, tmp, dataSize);
				if (res) return res;

				/* R,G,B is 16 bits big endian, ignore least significant 8 bits */
				uint8_t palR = tmp[0], palG = tmp[2], palB = tmp[4];
				transparentCol = PackedCol_ARGB(palR, palG, palB, 0);
			} else {
				return PNG_ERR_TRANS_INVALID;
			}
		} break;

		case PNG_FourCC('I','D','A','T'): {
			Stream_ReadonlyPortion(&datStream, stream, dataSize);
			inflate.Source = &datStream;

			/* TODO: This assumes zlib header will be in 1 IDAT chunk */
			while (!zlibHeader.Done) {
				if ((res = ZLibHeader_Read(&datStream, &zlibHeader))) return res;
			}
			if (!bmp->Scan0) return PNG_ERR_NO_DATA;

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

				begY = bufferIdx / scanlineBytes;
				left = bufferLen - bufferIdx;
				/* if row is at 0, last row in buffer is prior row */
				/* hence subtract a row, as don't want to overwrite it */
				if (begY == 0) left -= scanlineBytes;

				res = compStream.Read(&compStream, &buffer[bufferIdx], left, &read);
				if (res) return res;
				if (!read) break;

				bufferIdx += read;
				endY = bufferIdx / scanlineBytes;
				/* reached end of buffer, cycle back to start */
				if (bufferIdx == bufferLen) bufferIdx = 0;

				for (rowY = begY; rowY < endY; rowY++, curY++) {
					uint32_t priorY = rowY == 0 ? bufferRows : rowY;
					uint8_t* prior    = &buffer[(priorY - 1) * scanlineBytes];
					uint8_t* scanline = &buffer[rowY         * scanlineBytes];

					Png_Reconstruct(scanline[0], bytesPerPixel, &scanline[1], &prior[1], scanlineSize);
					rowExpander(bmp->Width, palette, &scanline[1], Bitmap_GetRow(bmp, curY));
				}
			}
		} break;

		case PNG_FourCC('I','E','N','D'): {
			readingChunks = false;
			if (dataSize) return PNG_ERR_INVALID_END_SIZE;
		} break;

		default:
			if ((res = stream->Skip(stream, dataSize))) return res;
			break;
		}

		if ((res = stream->Skip(stream, 4))) return res; /* Skip CRC32 */
	}

	if (transparentCol <= PNG_RGB_MASK) {
		Png_ComputeTransparency(bmp, transparentCol);
	}
	return bmp->Scan0 ? 0 : PNG_ERR_NO_DATA;
}


/*########################################################################################################################*
*------------------------------------------------------PNG encoder--------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Bitmap_Crc32StreamWrite(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified) {
	struct Stream* source;
	uint32_t i, crc32 = stream->Meta.CRC32.CRC32;

	/* TODO: Optimise this calculation */
	for (i = 0; i < count; i++) {
		crc32 = Utils_Crc32Table[(crc32 ^ data[i]) & 0xFF] ^ (crc32 >> 8);
	}
	stream->Meta.CRC32.CRC32 = crc32;

	source = stream->Meta.CRC32.Source;
	return source->Write(source, data, count, modified);
}

static void Bitmap_Crc32Stream(struct Stream* stream, struct Stream* underlying) {
	Stream_Init(stream);
	stream->Meta.CRC32.Source = underlying;
	stream->Meta.CRC32.CRC32  = 0xFFFFFFFFUL;
	stream->Write = Bitmap_Crc32StreamWrite;
}

static void Png_Filter(uint8_t filter, uint8_t* cur, uint8_t* prior, uint8_t* best, int lineLen) {
	/* 3 bytes per pixel constant */
	uint8_t a, b, c;
	int i, p, pa, pb, pc;

	switch (filter) {
	case PNG_FILTER_NONE:
		Mem_Copy(best, cur, lineLen);
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
			a = cur[i - 3]; b = prior[i]; c = prior[i - 3];
			p = a + b - c;

			pa = Math_AbsI(p - a);
			pb = Math_AbsI(p - b);
			pc = Math_AbsI(p - c);

			if (pa <= pb && pa <= pc) { best[i] = cur[i] - a; }
			else if (pb <= pc)        { best[i] = cur[i] - b; }
			else                      { best[i] = cur[i] - c; }
		}
		break;
	}
}

static void Png_EncodeRow(const uint8_t* src, uint8_t* cur, uint8_t* prior, uint8_t* best, int lineLen) {
	uint8_t* dst = cur;
	int bestFilter, bestEstimate = Int32_MaxValue;
	int x, filter, estimate;

	for (x = 0; x < lineLen; x += 3) {
		dst[0] = src[2]; dst[1] = src[1]; dst[2] = src[0];
		src += 4; dst += 3;
	}

	dst = best + 1;
	/* NOTE: Waste of time trying the PNG_NONE filter */
	for (filter = PNG_FILTER_SUB; filter <= PNG_FILTER_PAETH; filter++) {
		Png_Filter(filter, cur, prior, dst, lineLen);

		/* Estimate how well this filtered line will compress, based on */
		/* smallest sum of magnitude of each byte (signed) in the line */
		/* (see note in PNG specification, 12.8 "Filter selection" ) */
		estimate = 0;
		for (x = 0; x < lineLen; x++) {
			estimate += Math_AbsI((int8_t)dst[x]);
		}

		if (estimate > bestEstimate) continue;
		bestEstimate = estimate;
		bestFilter   = filter;
	}

	/* The bytes in dst are from last filter run (paeth) */
	/* However, we want dst to be bytes from the best filter */
	if (bestFilter != PNG_FILTER_PAETH) {
		Png_Filter(bestFilter, cur, prior, dst, lineLen);
	}
	best[0] = bestFilter;
}

ReturnCode Png_Encode(Bitmap* bmp, struct Stream* stream, Png_RowSelector selectRow) {	
	uint8_t tmp[32];
	uint8_t prevLine[PNG_MAX_DIMS * 3], curLine[PNG_MAX_DIMS * 3];
	uint8_t bestLine[PNG_MAX_DIMS * 3 + 1];

	struct ZLibState zlState;
	struct Stream chunk, zlStream;
	uint32_t stream_len;
	int y, lineSize;
	ReturnCode res;

	if ((res = Stream_Write(stream, png_sig, PNG_SIG_SIZE))) return res;
	Bitmap_Crc32Stream(&chunk, stream);

	/* Write header chunk */
	Stream_SetU32_BE(&tmp[0], PNG_IHDR_SIZE);
	Stream_SetU32_BE(&tmp[4], PNG_FourCC('I','H','D','R'));
	{
		Stream_SetU32_BE(&tmp[8],  bmp->Width);
		Stream_SetU32_BE(&tmp[12], bmp->Height);
		tmp[16] = 8;           /* bits per sample */
		tmp[17] = PNG_COL_RGB; /* TODO: RGBA but mask all alpha to 255? */
		tmp[18] = 0;           /* DEFLATE compression method */
		tmp[19] = 0;           /* ADAPTIVE filter method */
		tmp[20] = 0;           /* Not using interlacing */
	}
	Stream_SetU32_BE(&tmp[21], Utils_CRC32(&tmp[4], 17));

	/* Write PNG body */
	Stream_SetU32_BE(&tmp[25], 0); /* size of IDAT, filled in later */
	if ((res = Stream_Write(stream, tmp, 29))) return res;
	Stream_SetU32_BE(&tmp[0], PNG_FourCC('I','D','A','T'));
	if ((res = Stream_Write(&chunk, tmp, 4))) return res;

	ZLib_MakeStream(&zlStream, &zlState, &chunk); 
	lineSize = bmp->Width * 3;
	Mem_Set(prevLine, 0, lineSize);

	for (y = 0; y < bmp->Height; y++) {
		int row = selectRow(bmp, y);
		uint8_t* src  = (uint8_t*)Bitmap_GetRow(bmp, row);
		uint8_t* prev = (y & 1) == 0 ? prevLine : curLine;
		uint8_t* cur  = (y & 1) == 0 ? curLine  : prevLine;

		Png_EncodeRow(src, cur, prev, bestLine, lineSize);
		/* +1 for filter byte */
		if ((res = Stream_Write(&zlStream, bestLine, lineSize + 1))) return res;
	}
	if ((res = zlStream.Close(&zlStream))) return res;
	Stream_SetU32_BE(&tmp[0], chunk.Meta.CRC32.CRC32 ^ 0xFFFFFFFFUL);

	/* Write end chunk */
	Stream_SetU32_BE(&tmp[4],  0);
	Stream_SetU32_BE(&tmp[8],  PNG_FourCC('I','E','N','D'));
	Stream_SetU32_BE(&tmp[12], 0xAE426082UL); /* CRC32 of iend */
	if ((res = Stream_Write(stream, tmp, 16))) return res;

	/* Come back to write size of data chunk */
	if ((res = stream->Length(stream, &stream_len))) return res;
	if ((res = stream->Seek(stream, 33)))            return res;

	Stream_SetU32_BE(&tmp[0], stream_len - 57);
	return Stream_Write(stream, tmp, 4);
}
