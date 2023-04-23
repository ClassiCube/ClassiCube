#include "Bitmap.h"
#include "Platform.h"
#include "ExtMath.h"
#include "Deflate.h"
#include "Logger.h"
#include "Stream.h"
#include "Errors.h"
#include "Utils.h"

BitmapCol BitmapColor_Offset(BitmapCol color, int rBy, int gBy, int bBy) {
	int r, g, b;
	r = BitmapCol_R(color) + rBy; Math_Clamp(r, 0, 255);
	g = BitmapCol_G(color) + gBy; Math_Clamp(g, 0, 255);
	b = BitmapCol_B(color) + bBy; Math_Clamp(b, 0, 255);
	return BitmapColor_RGB(r, g, b);
}

BitmapCol BitmapColor_Scale(BitmapCol a, float t) {
	cc_uint8 R = (cc_uint8)(BitmapCol_R(a) * t);
	cc_uint8 G = (cc_uint8)(BitmapCol_G(a) * t);
	cc_uint8 B = (cc_uint8)(BitmapCol_B(a) * t);
	return (a & BITMAPCOLOR_A_MASK) | BitmapColor_R_Bits(R) | BitmapColor_G_Bits(G) | BitmapColor_B_Bits(B);
}

void Bitmap_UNSAFE_CopyBlock(int srcX, int srcY, int dstX, int dstY, 
							struct Bitmap* src, struct Bitmap* dst, int size) {
	int x, y;
	for (y = 0; y < size; y++) {
		BitmapCol* srcRow = Bitmap_GetRow(src, srcY + y) + srcX;
		BitmapCol* dstRow = Bitmap_GetRow(dst, dstY + y) + dstX;
		for (x = 0; x < size; x++) { dstRow[x] = srcRow[x]; }
	}
}

void Bitmap_Allocate(struct Bitmap* bmp, int width, int height) {
	bmp->width = width; bmp->height = height;
	bmp->scan0 = (BitmapCol*)Mem_Alloc(width * height, 4, "bitmap data");
}

void Bitmap_TryAllocate(struct Bitmap* bmp, int width, int height) {
	bmp->width = width; bmp->height = height;
	bmp->scan0 = (BitmapCol*)Mem_TryAlloc(width * height, 4);
}

void Bitmap_Scale(struct Bitmap* dst, struct Bitmap* src, 
					int srcX, int srcY, int srcWidth, int srcHeight) {
	BitmapCol* dstRow;
	BitmapCol* srcRow;
	int x, y, width, height;

	width  = dst->width;
	height = dst->height;

	for (y = 0; y < height; y++) {
		srcRow = Bitmap_GetRow(src, srcY + (y * srcHeight / height));
		dstRow = Bitmap_GetRow(dst, y);

		for (x = 0; x < width; x++) {
			dstRow[x] = srcRow[srcX + (x * srcWidth / width)];
		}
	}
}


/*########################################################################################################################*
*------------------------------------------------------PNG decoder--------------------------------------------------------*
*#########################################################################################################################*/
#define PNG_IHDR_SIZE 13
#define PNG_PALETTE 256
#define PNG_FourCC(a, b, c, d) (((cc_uint32)a << 24) | ((cc_uint32)b << 16) | ((cc_uint32)c << 8) | (cc_uint32)d)

enum PngColor {
	PNG_COLOR_GRAYSCALE = 0, PNG_COLOR_RGB = 2, PNG_COLOR_INDEXED = 3,
	PNG_COLOR_GRAYSCALE_A = 4, PNG_COLOR_RGB_A = 6
};

enum PngFilter {
	PNG_FILTER_NONE, PNG_FILTER_SUB, PNG_FILTER_UP, PNG_FILTER_AVERAGE, PNG_FILTER_PAETH
};

typedef void (*Png_RowExpander)(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst);
static const cc_uint8 pngSig[PNG_SIG_SIZE] = { 137, 80, 78, 71, 13, 10, 26, 10 };

cc_bool Png_Detect(const cc_uint8* data, cc_uint32 len) {
	return len >= PNG_SIG_SIZE && Mem_Equal(data, pngSig, PNG_SIG_SIZE);
}

static void Png_Reconstruct(cc_uint8 type, cc_uint8 bytesPerPixel, cc_uint8* line, cc_uint8* prior, cc_uint32 lineLen) {
	cc_uint32 i, j;
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
			cc_uint8 a = line[j], b = prior[i], c = prior[j];
			int p = a + b - c;
			int pa = Math_AbsI(p - a);
			int pb = Math_AbsI(p - b);
			int pc = Math_AbsI(p - c);

			if (pa <= pb && pa <= pc) { line[i] += a; } 
			else if (pb <= pc) {        line[i] += b; } 
			else {                      line[i] += c; }
		}
		return;
	}
}

#define Bitmap_Set(dst, r,g,b,a) dst = BitmapCol_Make(r, g, b, a);

#define PNG_Do_Grayscale(dstI, src, scale)  rgb = (src) * scale; Bitmap_Set(dst[dstI], rgb, rgb, rgb, 255);
#define PNG_Do_Grayscale_8(dstI, srcI)      rgb = src[srcI];     Bitmap_Set(dst[dstI], rgb, rgb, rgb, 255);
#define PNG_Do_Grayscale_A__8(dstI, srcI)   rgb = src[srcI];     Bitmap_Set(dst[dstI], rgb, rgb, rgb, src[srcI + 1]);
#define PNG_Do_RGB__8(dstI, srcI)           Bitmap_Set(dst[dstI], src[srcI], src[srcI + 1], src[srcI + 2], 255);
#define PNG_Do_RGB_A__8(dstI, srcI)         Bitmap_Set(dst[dstI], src[srcI], src[srcI + 1], src[srcI + 2], src[srcI + 3]);

#define PNG_Mask_1(i) (7 - (i & 7))
#define PNG_Mask_2(i) ((3 - (i & 3)) * 2)
#define PNG_Mask_4(i) ((1 - (i & 1)) * 4)
#define PNG_Get__1(i) ((src[i >> 3] >> PNG_Mask_1(i)) & 1)
#define PNG_Get__2(i) ((src[i >> 2] >> PNG_Mask_2(i)) & 3)

static void Png_Expand_GRAYSCALE_1(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i; cc_uint8 rgb; /* NOTE: not optimised*/
	for (i = 0; i < width; i++) { PNG_Do_Grayscale(i, PNG_Get__1(i), 255); }
}

static void Png_Expand_GRAYSCALE_2(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i; cc_uint8 rgb; /* NOTE: not optimised */
	for (i = 0; i < width; i++) { PNG_Do_Grayscale(i, PNG_Get__2(i), 85); }
}

static void Png_Expand_GRAYSCALE_4(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i, j; cc_uint8 rgb;

	for (i = 0, j = 0; i < (width & ~0x1); i += 2, j++) {
		PNG_Do_Grayscale(i, src[j] >> 4, 17); PNG_Do_Grayscale(i + 1, src[j] & 0x0F, 17);
	}
	for (; i < width; i++) {
		PNG_Do_Grayscale(i, (src[j] >> PNG_Mask_4(i)) & 15, 17);
	}
}

static void Png_Expand_GRAYSCALE_8(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i; cc_uint8 rgb;

	for (i = 0; i < (width & ~0x3); i += 4) {
		PNG_Do_Grayscale_8(i    , i    ); PNG_Do_Grayscale_8(i + 1, i + 1);
		PNG_Do_Grayscale_8(i + 2, i + 2); PNG_Do_Grayscale_8(i + 3, i + 3);
	}
	for (; i < width; i++) { PNG_Do_Grayscale_8(i, i); }
}

static void Png_Expand_GRAYSCALE_16(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i; cc_uint8 rgb; /* NOTE: not optimised */
	for (i = 0; i < width; i++) {
		rgb = src[i * 2]; Bitmap_Set(dst[i], rgb, rgb, rgb, 255);
	}
}

static void Png_Expand_RGB_8(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i, j;

	for (i = 0, j = 0; i < (width & ~0x03); i += 4, j += 12) {
		PNG_Do_RGB__8(i    , j    ); PNG_Do_RGB__8(i + 1, j + 3);
		PNG_Do_RGB__8(i + 2, j + 6); PNG_Do_RGB__8(i + 3, j + 9);
	}
	for (; i < width; i++, j += 3) { PNG_Do_RGB__8(i, j); }
}

static void Png_Expand_RGB_16(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i, j; /* NOTE: not optimised */
	for (i = 0, j = 0; i < width; i++, j += 6) { 
		Bitmap_Set(dst[i], src[j], src[j + 2], src[j + 4], 255);
	}
}

static void Png_Expand_INDEXED_1(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i; /* NOTE: not optimised */
	for (i = 0; i < width; i++) { dst[i] = palette[PNG_Get__1(i)]; }
}

static void Png_Expand_INDEXED_2(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i; /* NOTE: not optimised */
	for (i = 0; i < width; i++) { dst[i] = palette[PNG_Get__2(i)]; }
}

static void Png_Expand_INDEXED_4(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i, j; cc_uint8 cur;

	for (i = 0, j = 0; i < (width & ~0x1); i += 2, j++) {
		cur = src[j];
		dst[i] = palette[cur >> 4]; dst[i + 1] = palette[cur & 0x0F];
	}
	for (; i < width; i++) {
		dst[i] = palette[(src[j] >> PNG_Mask_4(i)) & 15];
	}
}

static void Png_Expand_INDEXED_8(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i;

	for (i = 0; i < (width & ~0x3); i += 4) {
		dst[i]     = palette[src[i]];     dst[i + 1] = palette[src[i + 1]];
		dst[i + 2] = palette[src[i + 2]]; dst[i + 3] = palette[src[i + 3]];
	}
	for (; i < width; i++) { dst[i] = palette[src[i]]; }
}

static void Png_Expand_GRAYSCALE_A_8(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i, j; cc_uint8 rgb;

	for (i = 0, j = 0; i < (width & ~0x3); i += 4, j += 8) {
		PNG_Do_Grayscale_A__8(i    , j    ); PNG_Do_Grayscale_A__8(i + 1, j + 2);
		PNG_Do_Grayscale_A__8(i + 2, j + 4); PNG_Do_Grayscale_A__8(i + 3, j + 6);
	}
	for (; i < width; i++, j += 2) { PNG_Do_Grayscale_A__8(i, j); }
}

static void Png_Expand_GRAYSCALE_A_16(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i; cc_uint8 rgb; /* NOTE: not optimised*/
	for (i = 0; i < width; i++) {
		rgb = src[i * 4]; Bitmap_Set(dst[i], rgb, rgb, rgb, src[i * 4 + 2]);
	}
}

static void Png_Expand_RGB_A_8(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i, j;

	for (i = 0, j = 0; i < (width & ~0x3); i += 4, j += 16) {
		PNG_Do_RGB_A__8(i    , j    ); PNG_Do_RGB_A__8(i + 1, j + 4 );
		PNG_Do_RGB_A__8(i + 2, j + 8); PNG_Do_RGB_A__8(i + 3, j + 12);
	}
	for (; i < width; i++, j += 4) { PNG_Do_RGB_A__8(i, j); }
}

static void Png_Expand_RGB_A_16(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i, j; /* NOTE: not optimised*/
	for (i = 0, j = 0; i < width; i++, j += 8) { 
		Bitmap_Set(dst[i], src[j], src[j + 2], src[j + 4], src[j + 6]);
	}
}

static Png_RowExpander Png_GetExpander(cc_uint8 col, cc_uint8 bitsPerSample) {
	switch (col) {
	case PNG_COLOR_GRAYSCALE:
		switch (bitsPerSample) {
		case 1:  return Png_Expand_GRAYSCALE_1;
		case 2:  return Png_Expand_GRAYSCALE_2;
		case 4:  return Png_Expand_GRAYSCALE_4;
		case 8:  return Png_Expand_GRAYSCALE_8;
		case 16: return Png_Expand_GRAYSCALE_16;
		}
		return NULL;

	case PNG_COLOR_RGB:
		switch (bitsPerSample) {
		case 8:  return Png_Expand_RGB_8;
		case 16: return Png_Expand_RGB_16;
		}
		return NULL;

	case PNG_COLOR_INDEXED:
		switch (bitsPerSample) {
		case 1: return Png_Expand_INDEXED_1;
		case 2: return Png_Expand_INDEXED_2;
		case 4: return Png_Expand_INDEXED_4;
		case 8: return Png_Expand_INDEXED_8;
		}
		return NULL;

	case PNG_COLOR_GRAYSCALE_A:
		switch (bitsPerSample) {
		case 8:  return Png_Expand_GRAYSCALE_A_8;
		case 16: return Png_Expand_GRAYSCALE_A_16;
		}
		return NULL;

	case PNG_COLOR_RGB_A:
		switch (bitsPerSample) {
		case 8:  return Png_Expand_RGB_A_8;
		case 16: return Png_Expand_RGB_A_16;
		}
		return NULL;
	}
	return NULL;
}

/* Sets alpha to 0 for any pixels in the bitmap whose RGB is same as col */
static void ComputeTransparency(struct Bitmap* bmp, BitmapCol col) {
	BitmapCol trnsRGB = col & BITMAPCOLOR_RGB_MASK;
	int x, y, width = bmp->width, height = bmp->height;

	for (y = 0; y < height; y++) {
		BitmapCol* row = Bitmap_GetRow(bmp, y);
		for (x = 0; x < width; x++) {
			BitmapCol rgb = row[x] & BITMAPCOLOR_RGB_MASK;
			row[x] = (rgb == trnsRGB) ? trnsRGB : row[x];
		}
	}
}

/* Most bits per sample is 16. Most samples per pixel is 4. Add 1 for filter byte. */
/* Need to store both current and prior row, per PNG specification. */
#define PNG_BUFFER_SIZE ((PNG_MAX_DIMS * 2 * 4 + 1) * 2)

/* TODO: Test a lot of .png files and ensure output is right */
cc_result Png_Decode(struct Bitmap* bmp, struct Stream* stream) {
	cc_uint8 tmp[PNG_PALETTE * 3];
	cc_uint32 dataSize, fourCC;
	cc_result res;

	/* header variables */
	static cc_uint32 samplesPerPixel[7] = { 1, 0, 3, 1, 2, 0, 4 };
	cc_uint8 col, bitsPerSample, bytesPerPixel;
	Png_RowExpander rowExpander;
	cc_uint32 scanlineSize, scanlineBytes;

	/* palette data */
	BitmapCol trnsCol;
	BitmapCol palette[PNG_PALETTE];
	cc_uint32 i;

	/* idat state */
	cc_uint32 curY = 0, begY, rowY, endY;
	cc_uint8 buffer[PNG_BUFFER_SIZE];
	cc_uint32 bufferRows, bufferLen;
	cc_uint32 bufferIdx, read, left;

	/* idat decompressor */
	struct InflateState inflate;
	struct Stream compStream, datStream;
	struct ZLibHeader zlibHeader;

	bmp->width = 0; bmp->height = 0;
	bmp->scan0 = NULL;

	res = Stream_Read(stream, tmp, PNG_SIG_SIZE);
	if (res) return res;
	if (!Png_Detect(tmp, PNG_SIG_SIZE)) return PNG_ERR_INVALID_SIG;

	trnsCol = BITMAPCOLOR_BLACK;
	for (i = 0; i < PNG_PALETTE; i++) { palette[i] = BITMAPCOLOR_BLACK; }

	Inflate_MakeStream2(&compStream, &inflate, stream);
	ZLibHeader_Init(&zlibHeader);

	for (;;) {
		res = Stream_Read(stream, tmp, 8);
		if (res) return res;
		dataSize = Stream_GetU32_BE(tmp + 0);
		fourCC   = Stream_GetU32_BE(tmp + 4);

		switch (fourCC) {
		case PNG_FourCC('I','H','D','R'): {
			if (dataSize != PNG_IHDR_SIZE) return PNG_ERR_INVALID_HDR_SIZE;
			res = Stream_Read(stream, tmp, PNG_IHDR_SIZE);
			if (res) return res;

			bmp->width  = (int)Stream_GetU32_BE(tmp + 0);
			bmp->height = (int)Stream_GetU32_BE(tmp + 4);
			if (bmp->width  < 0 || bmp->width  > PNG_MAX_DIMS) return PNG_ERR_TOO_WIDE;
			if (bmp->height < 0 || bmp->height > PNG_MAX_DIMS) return PNG_ERR_TOO_TALL;

			bmp->scan0 = (BitmapCol*)Mem_TryAlloc(bmp->width * bmp->height, 4);
			if (!bmp->scan0) return ERR_OUT_OF_MEMORY;

			bitsPerSample = tmp[8]; col = tmp[9];
			rowExpander = Png_GetExpander(col, bitsPerSample);
			if (rowExpander == NULL) return PNG_ERR_INVALID_COL_BPP;

			if (tmp[10] != 0) return PNG_ERR_COMP_METHOD;
			if (tmp[11] != 0) return PNG_ERR_FILTER;
			if (tmp[12] != 0) return PNG_ERR_INTERLACED;

			bytesPerPixel = ((samplesPerPixel[col] * bitsPerSample) + 7) >> 3;
			scanlineSize  = ((samplesPerPixel[col] * bitsPerSample * bmp->width) + 7) >> 3;
			scanlineBytes = scanlineSize + 1; /* Add 1 byte for filter byte of each scanline */

			Mem_Set(buffer, 0, scanlineBytes); /* Prior row should be 0 per PNG spec */
			bufferIdx  = scanlineBytes;
			bufferRows = PNG_BUFFER_SIZE / scanlineBytes;
			bufferLen  = bufferRows * scanlineBytes;
		} break;

		case PNG_FourCC('P','L','T','E'): {
			if (dataSize > PNG_PALETTE * 3) return PNG_ERR_PAL_SIZE;
			if ((dataSize % 3) != 0)        return PNG_ERR_PAL_SIZE;
			res = Stream_Read(stream, tmp, dataSize);
			if (res) return res;

			for (i = 0; i < dataSize; i += 3) {
				palette[i / 3] &= BITMAPCOLOR_A_MASK; /* set RGB to 0 */
				palette[i / 3] |= tmp[i    ] << BITMAPCOLOR_R_SHIFT;
				palette[i / 3] |= tmp[i + 1] << BITMAPCOLOR_G_SHIFT;
				palette[i / 3] |= tmp[i + 2] << BITMAPCOLOR_B_SHIFT;
			}
		} break;

		case PNG_FourCC('t','R','N','S'): {
			if (col == PNG_COLOR_GRAYSCALE) {
				if (dataSize != 2) return PNG_ERR_TRANS_COUNT;
				res = Stream_Read(stream, tmp, dataSize);
				if (res) return res;

				/* RGB is always two bytes */
				/* TODO is this right for 16 bits per channel images? */
				trnsCol = BitmapCol_Make(tmp[1], tmp[1], tmp[1], 0);
			} else if (col == PNG_COLOR_INDEXED) {
				if (dataSize > PNG_PALETTE) return PNG_ERR_TRANS_COUNT;
				res = Stream_Read(stream, tmp, dataSize);
				if (res) return res;

				/* set alpha component of palette */
				for (i = 0; i < dataSize; i++) {
					palette[i] &= BITMAPCOLOR_RGB_MASK; /* set A to 0 */
					palette[i] |= tmp[i] << BITMAPCOLOR_A_SHIFT;
				}
			} else if (col == PNG_COLOR_RGB) {
				if (dataSize != 6) return PNG_ERR_TRANS_COUNT;
				res = Stream_Read(stream, tmp, dataSize);
				if (res) return res;

				/* R,G,B are always two bytes */
				/* TODO is this right for 16 bits per channel images? */
				trnsCol = BitmapCol_Make(tmp[1], tmp[3], tmp[5], 0);
			} else {
				return PNG_ERR_TRANS_INVALID;
			}
		} break;

		case PNG_FourCC('I','D','A','T'): {
			Stream_ReadonlyPortion(&datStream, stream, dataSize);
			inflate.Source = &datStream;

			/* TODO: This assumes zlib header will be in 1 IDAT chunk */
			while (!zlibHeader.done) {
				if ((res = ZLibHeader_Read(&datStream, &zlibHeader))) return res;
			}
			if (!bmp->scan0) return PNG_ERR_NO_DATA;

			while (curY < bmp->height) {
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

				/* NOTE: Need to check curY too, in case IDAT is corrupted and has extra data */
				for (rowY = begY; rowY < endY && curY < bmp->height; rowY++, curY++) {
					cc_uint32 priorY = rowY == 0 ? bufferRows : rowY;
					cc_uint8* prior    = &buffer[(priorY - 1) * scanlineBytes];
					cc_uint8* scanline = &buffer[rowY         * scanlineBytes];

					if (scanline[0] > PNG_FILTER_PAETH) return PNG_ERR_INVALID_SCANLINE;
					Png_Reconstruct(scanline[0], bytesPerPixel, &scanline[1], &prior[1], scanlineSize);
					rowExpander(bmp->width, palette, &scanline[1], Bitmap_GetRow(bmp, curY));
				}
			}

			if (curY == bmp->height) {
				if (!BitmapCol_A(trnsCol)) ComputeTransparency(bmp, trnsCol);
				return 0;
			}
		} break;

		case PNG_FourCC('I','E','N','D'):
			/* Reading all image data should be handled by above if in the IDAT chunk */
			/* If we reached here, it means not all of the image data was read */
			return PNG_ERR_REACHED_IEND;

		default:
			if ((res = stream->Skip(stream, dataSize))) return res;
			break;
		}

		if ((res = stream->Skip(stream, 4))) return res; /* Skip CRC32 */
	}
}


/*########################################################################################################################*
*------------------------------------------------------PNG encoder--------------------------------------------------------*
*#########################################################################################################################*/
static void Png_Filter(cc_uint8 filter, const cc_uint8* cur, const cc_uint8* prior, cc_uint8* best, int lineLen, int bpp) {
	/* 3 bytes per pixel constant */
	cc_uint8 a, b, c;
	int i, p, pa, pb, pc;

	switch (filter) {
	case PNG_FILTER_SUB:
		for (i = 0; i < bpp; i++) { best[i] = cur[i]; }

		for (; i < lineLen; i++) {
			best[i] = cur[i] - cur[i - bpp];
		}
		break;

	case PNG_FILTER_UP:
		for (i = 0; i < lineLen; i++) {
			best[i] = cur[i] - prior[i];
		}
		break;

	case PNG_FILTER_AVERAGE:
		for (i = 0; i < bpp; i++) { best[i] = cur[i] - (prior[i] >> 1); }

		for (; i < lineLen; i++) {
			best[i] = cur[i] - ((prior[i] + cur[i - bpp]) >> 1);
		}
		break;

	case PNG_FILTER_PAETH:
		for (i = 0; i < bpp; i++) { best[i] = cur[i] - prior[i]; }

		for (; i < lineLen; i++) {
			a = cur[i - bpp]; b = prior[i]; c = prior[i - bpp];
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

static void Png_MakeRow(const BitmapCol* src, cc_uint8* dst, int lineLen, cc_bool alpha) {
	cc_uint8* end = dst + lineLen;
	BitmapCol col; /* if we use *src, register gets reloaded each time */

	if (alpha) {
		for (; dst < end; src++, dst += 4) {
			col    = *src;
			dst[0] = BitmapCol_R(col); dst[1] = BitmapCol_G(col);
			dst[2] = BitmapCol_B(col); dst[3] = BitmapCol_A(col);
		}
	} else {
		for (; dst < end; src++, dst += 3) {
			col    = *src;
			dst[0] = BitmapCol_R(col); dst[1] = BitmapCol_G(col);
			dst[2] = BitmapCol_B(col);
		}
	}
}

static void Png_EncodeRow(const cc_uint8* cur, const cc_uint8* prior, cc_uint8* best, int lineLen, cc_bool alpha) {
	cc_uint8* dst;
	int bestFilter, bestEstimate = Int32_MaxValue;
	int x, filter, estimate;

	dst = best + 1;
	/* NOTE: Waste of time trying the PNG_NONE filter */
	for (filter = PNG_FILTER_SUB; filter <= PNG_FILTER_PAETH; filter++) {
		Png_Filter(filter, cur, prior, dst, lineLen, alpha ? 4 : 3);

		/* Estimate how well this filtered line will compress, based on */
		/* smallest sum of magnitude of each byte (signed) in the line */
		/* (see note in PNG specification, 12.8 "Filter selection" ) */
		estimate = 0;
		for (x = 0; x < lineLen; x++) {
			estimate += Math_AbsI((cc_int8)dst[x]);
		}

		if (estimate > bestEstimate) continue;
		bestEstimate = estimate;
		bestFilter   = filter;
	}

	/* The bytes in dst are from last filter run (paeth) */
	/* However, we want dst to be bytes from the best filter */
	if (bestFilter != PNG_FILTER_PAETH) {
		Png_Filter(bestFilter, cur, prior, dst, lineLen, alpha ? 4 : 3);
	}

	best[0] = bestFilter;
}

static BitmapCol* DefaultGetRow(struct Bitmap* bmp, int y) { return Bitmap_GetRow(bmp, y); }
cc_result Png_Encode(struct Bitmap* bmp, struct Stream* stream, 
					Png_RowGetter getRow, cc_bool alpha) {
	cc_uint8 tmp[32];
	/* TODO: This should be * 4 for alpha (should switch to mem_alloc though) */
	cc_uint8 prevLine[PNG_MAX_DIMS * 3], curLine[PNG_MAX_DIMS * 3];
	cc_uint8 bestLine[PNG_MAX_DIMS * 3 + 1];

	struct ZLibState zlState;
	struct Stream chunk, zlStream;
	cc_uint32 stream_end, stream_beg;
	int y, lineSize;
	cc_result res;

	/* stream may not start at 0 (e.g. when making default.zip) */
	if ((res = stream->Position(stream, &stream_beg))) return res;

	if (!getRow) getRow = DefaultGetRow;
	if ((res = Stream_Write(stream, pngSig, PNG_SIG_SIZE))) return res;
	Stream_WriteonlyCrc32(&chunk, stream);

	/* Write header chunk */
	Stream_SetU32_BE(&tmp[0], PNG_IHDR_SIZE);
	Stream_SetU32_BE(&tmp[4], PNG_FourCC('I','H','D','R'));
	{
		Stream_SetU32_BE(&tmp[8],  bmp->width);
		Stream_SetU32_BE(&tmp[12], bmp->height);
		tmp[16] = 8;           /* bits per sample */
		tmp[17] = alpha ? PNG_COLOR_RGB_A : PNG_COLOR_RGB;
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
	lineSize = bmp->width * (alpha ? 4 : 3);
	Mem_Set(prevLine, 0, lineSize);

	for (y = 0; y < bmp->height; y++) {
		BitmapCol* src = getRow(bmp, y);
		cc_uint8* prev = (y & 1) == 0 ? prevLine : curLine;
		cc_uint8* cur  = (y & 1) == 0 ? curLine  : prevLine;

		Png_MakeRow(src, cur, lineSize, alpha);
		Png_EncodeRow(cur, prev, bestLine, lineSize, alpha);

		/* +1 for filter byte */
		if ((res = Stream_Write(&zlStream, bestLine, lineSize + 1))) return res;
	}
	if ((res = zlStream.Close(&zlStream))) return res;
	Stream_SetU32_BE(&tmp[0], chunk.Meta.CRC32.CRC32 ^ 0xFFFFFFFFUL);

	/* Write end chunk */
	Stream_SetU32_BE(&tmp[4],  0);
	Stream_SetU32_BE(&tmp[8],  PNG_FourCC('I','E','N','D'));
	Stream_SetU32_BE(&tmp[12], 0xAE426082UL); /* CRC32 of IEND */
	if ((res = Stream_Write(stream, tmp, 16))) return res;

	/* Come back to fixup size of data in data chunk */
	/* TODO: Position instead of Length */
	if ((res = stream->Length(stream, &stream_end)))   return res;
	if ((res = stream->Seek(stream, stream_beg + 33))) return res;

	Stream_SetU32_BE(&tmp[0], (stream_end - stream_beg) - 57);
	if ((res = Stream_Write(stream, tmp, 4))) return res;
	return stream->Seek(stream, stream_end);
}
