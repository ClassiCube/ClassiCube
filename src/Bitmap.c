#include "Bitmap.h"
#include "Platform.h"
#include "ExtMath.h"
#include "Deflate.h"
#include "Logger.h"
#include "Stream.h"
#include "Errors.h"
#include "Utils.h"
#include "Funcs.h"

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
	bmp->scan0 = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "bitmap data");
}

void Bitmap_TryAllocate(struct Bitmap* bmp, int width, int height) {
	bmp->width = width; bmp->height = height;
	bmp->scan0 = (BitmapCol*)Mem_TryAlloc(width * height, BITMAPCOLOR_SIZE);
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

/* 5.2 PNG signature */
cc_bool Png_Detect(const cc_uint8* data, cc_uint32 len) {
	return len >= PNG_SIG_SIZE && Mem_Equal(data, pngSig, PNG_SIG_SIZE);
}

/* 9 Filtering */
/* 13.9 Filtering */
static void Png_ReconstructFirst(cc_uint8 type, cc_uint8 bytesPerPixel, cc_uint8* line, cc_uint32 lineLen) {
	/* First scanline is a special case, where all values in prior array are 0 */
	cc_uint32 i, j;

	switch (type) {
	case PNG_FILTER_SUB:
		for (i = bytesPerPixel, j = 0; i < lineLen; i++, j++) {
			line[i] += line[j];
		}
		return;

	case PNG_FILTER_AVERAGE:
		for (i = bytesPerPixel, j = 0; i < lineLen; i++, j++) {
			line[i] += (line[j] >> 1);
		}
		return;

	case PNG_FILTER_PAETH:
		for (i = bytesPerPixel, j = 0; i < lineLen; i++, j++) {
			line[i] += line[j];
		}
		return;
	}
}

static void Png_Reconstruct(cc_uint8 type, cc_uint8 bytesPerPixel, cc_uint8* line, cc_uint8* prior, cc_uint32 lineLen) {
	cc_uint32 i, j;

	switch (type) {
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

/* 7.2 Scanlines */
#define PNG_Do_Grayscale(dstI, src, scale)  rgb = (src) * scale; Bitmap_Set(dst[dstI], rgb, rgb, rgb, 255);
#define PNG_Do_Grayscale_8()      rgb = src[0]; Bitmap_Set(*dst, rgb, rgb, rgb,    255); dst--; src -= 1;
#define PNG_Do_Grayscale_A__8()   rgb = src[0]; Bitmap_Set(*dst, rgb, rgb, rgb, src[1]); dst--; src -= 2;
#define PNG_Do_RGB__8()           Bitmap_Set(*dst, src[0], src[1], src[2],    255); dst--; src -= 3;
#define PNG_Do_RGB_A__8()         Bitmap_Set(*dst, src[0], src[1], src[2], src[3]); dst++; src += 4;
#define PNG_Do_Palette__8()       *dst-- = palette[*src--];

#define PNG_Mask_1(i) (7  - (i & 7))
#define PNG_Mask_2(i) ((3 - (i & 3)) * 2)
#define PNG_Mask_4(i) ((1 - (i & 1)) * 4)
#define PNG_Get__1(i) ((src[i >> 3] >> PNG_Mask_1(i)) & 0x01)
#define PNG_Get__2(i) ((src[i >> 2] >> PNG_Mask_2(i)) & 0x03)
#define PNG_Get__4(i) ((src[i >> 1] >> PNG_Mask_4(i)) & 0x0F)

static void Png_Expand_GRAYSCALE_1(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i; cc_uint8 rgb; /* NOTE: not optimised*/
	for (i = width - 1; i >= 0; i--) { PNG_Do_Grayscale(i, PNG_Get__1(i), 255); }
}

static void Png_Expand_GRAYSCALE_2(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i; cc_uint8 rgb; /* NOTE: not optimised */
	for (i = width - 1; i >= 0; i--) { PNG_Do_Grayscale(i, PNG_Get__2(i), 85); }
}

static void Png_Expand_GRAYSCALE_4(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i; cc_uint8 rgb;
	for (i = width - 1; i >= 0; i--) { PNG_Do_Grayscale(i, PNG_Get__4(i), 17); }
}

static void Png_Expand_GRAYSCALE_8(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	cc_uint8 rgb;
	src += (width - 1);
	dst += (width - 1);

	for (; width >= 4; width -= 4) {
		PNG_Do_Grayscale_8(); PNG_Do_Grayscale_8();
		PNG_Do_Grayscale_8(); PNG_Do_Grayscale_8();
	}
	for (; width > 0; width--) { PNG_Do_Grayscale_8(); }
}

static void Png_Expand_RGB_8(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	src += (width - 1) * 3;
	dst += (width - 1);

	for (; width >= 4; width -= 4) {
		PNG_Do_RGB__8(); PNG_Do_RGB__8(); 
		PNG_Do_RGB__8(); PNG_Do_RGB__8();
	}
	for (; width > 0; width--) { PNG_Do_RGB__8(); }
}

static void Png_Expand_INDEXED_1(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i; /* NOTE: not optimised */
	for (i = width - 1; i >= 0; i--) { dst[i] = palette[PNG_Get__1(i)]; }
}

static void Png_Expand_INDEXED_2(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i; /* NOTE: not optimised */
	for (i = width - 1; i >= 0; i--) { dst[i] = palette[PNG_Get__2(i)]; }
}

static void Png_Expand_INDEXED_4(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	int i; /* NOTE: not optimised */
	for (i = width - 1; i >= 0; i--) { dst[i] = palette[PNG_Get__4(i)]; }
}

static void Png_Expand_INDEXED_8(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	src += (width - 1) * 1;
	dst += (width - 1);

	for (; width >= 4; width -= 4) {
		PNG_Do_Palette__8(); PNG_Do_Palette__8();
		PNG_Do_Palette__8(); PNG_Do_Palette__8();
	}
	for (; width > 0; width--) { PNG_Do_Palette__8(); }
}

static void Png_Expand_GRAYSCALE_A_8(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	cc_uint8 rgb;
	src += (width - 1) * 2;
	dst += (width - 1);

	for (; width >= 4; width -= 4) {
		PNG_Do_Grayscale_A__8(); PNG_Do_Grayscale_A__8();
		PNG_Do_Grayscale_A__8(); PNG_Do_Grayscale_A__8();
	}
	for (; width > 0; width--) { PNG_Do_Grayscale_A__8(); }
}

static void Png_Expand_RGB_A_8(int width, BitmapCol* palette, cc_uint8* src, BitmapCol* dst) {
	/* Processed in forward order */

	for (; width >= 4; width -= 4) {
		PNG_Do_RGB_A__8(); PNG_Do_RGB_A__8();
		PNG_Do_RGB_A__8(); PNG_Do_RGB_A__8();
	}
	for (; width > 0; width--) { PNG_Do_RGB_A__8(); }
}

static Png_RowExpander Png_GetExpander(cc_uint8 col, cc_uint8 bitsPerSample) {
	switch (col) {
	case PNG_COLOR_GRAYSCALE:
		switch (bitsPerSample) {
		case 1:  return Png_Expand_GRAYSCALE_1;
		case 2:  return Png_Expand_GRAYSCALE_2;
		case 4:  return Png_Expand_GRAYSCALE_4;
		case 8:  return Png_Expand_GRAYSCALE_8;
		}
		return NULL;

	case PNG_COLOR_RGB:
		switch (bitsPerSample) {
		case 8:  return Png_Expand_RGB_8;
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
		}
		return NULL;

	case PNG_COLOR_RGB_A:
		switch (bitsPerSample) {
		case 8:  return Png_Expand_RGB_A_8;
		}
		return NULL;
	}
	return NULL;
}

/* Sets alpha to 0 for any pixels in the bitmap whose RGB is same as colorspace */
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

static BitmapCol ExpandRGB(cc_uint8 bitsPerSample, int r, int g, int b) {
	switch (bitsPerSample) {
	case 1: 
		r *= 255; g *= 255; b *= 255; break;
	case 2:
		r *=  85; g *=  85; b *=  85; break;
	case 4:
		r *=  17; g *=  17; b *=  17; break;
	}
	return BitmapCol_Make(r, g, b, 0);
}

cc_result Png_Decode(struct Bitmap* bmp, struct Stream* stream) {
	cc_uint8 tmp[64];
	cc_uint32 dataSize, fourCC;
	cc_result res;

	/* header variables */
	static const cc_uint8 samplesPerPixel[] = { 1, 0, 3, 1, 2, 0, 4 };
	cc_uint8 colorspace, bitsPerSample, bytesPerPixel = 0;
	Png_RowExpander rowExpander = NULL;
	cc_uint32 scanlineSize = 0, scanlineBytes = 0;

	/* palette data */
	BitmapCol trnsColor;
	BitmapCol palette[PNG_PALETTE];
	cc_uint32 i;

	/* idat state */
	cc_uint32 available = 0, rowY = 0;
	cc_uint8 buffer[PNG_PALETTE * 3];
	cc_uint32 read, bufferIdx = 0;
	cc_uint32 left, bufferLen = 0;
	int curY;

	/* idat decompressor */
#ifdef CC_BUILD_TINYSTACK
	static struct InflateState inflate;
#else
	struct InflateState inflate;
#endif
	struct Stream compStream, datStream;
	struct ZLibHeader zlibHeader;
	cc_uint8* data = NULL;

	bmp->width = 0; bmp->height = 0;
	bmp->scan0 = NULL;

	res = Stream_Read(stream, tmp, PNG_SIG_SIZE);
	if (res) return res;
	if (!Png_Detect(tmp, PNG_SIG_SIZE)) return PNG_ERR_INVALID_SIG;

	colorspace = 0xFF; /* Unknown colour space */
	trnsColor  = BITMAPCOLOR_BLACK;
	for (i = 0; i < PNG_PALETTE; i++) { palette[i] = BITMAPCOLOR_BLACK; }

	Inflate_MakeStream2(&compStream, &inflate, stream);
	ZLibHeader_Init(&zlibHeader);

	for (;;) {
		res = Stream_Read(stream,   tmp, 8);
		if (res) return res;
		dataSize = Stream_GetU32_BE(tmp + 0);
		fourCC   = Stream_GetU32_BE(tmp + 4);

		switch (fourCC) {
		/* 11.2.2 IHDR Image header */
		case PNG_FourCC('I','H','D','R'): {
			if (dataSize != PNG_IHDR_SIZE) return PNG_ERR_INVALID_HDR_SIZE;
			res = Stream_Read(stream, tmp, PNG_IHDR_SIZE);
			if (res) return res;

			bmp->width  = (int)Stream_GetU32_BE(tmp + 0);
			bmp->height = (int)Stream_GetU32_BE(tmp + 4);
			if (bmp->width  < 0 || bmp->width  > PNG_MAX_DIMS) return PNG_ERR_TOO_WIDE;
			if (bmp->height < 0 || bmp->height > PNG_MAX_DIMS) return PNG_ERR_TOO_TALL;

			bitsPerSample = tmp[8]; colorspace = tmp[9];
			if (bitsPerSample == 16) return PNG_ERR_16BITSAMPLES;

			rowExpander = Png_GetExpander(colorspace, bitsPerSample);
			if (!rowExpander) return PNG_ERR_INVALID_COL_BPP;

			if (tmp[10] != 0) return PNG_ERR_COMP_METHOD;
			if (tmp[11] != 0) return PNG_ERR_FILTER;
			if (tmp[12] != 0) return PNG_ERR_INTERLACED;

			bytesPerPixel = ((samplesPerPixel[colorspace] * bitsPerSample) + 7) >> 3;
			scanlineSize  = ((samplesPerPixel[colorspace] * bitsPerSample * bmp->width) + 7) >> 3;
			scanlineBytes = scanlineSize + 1; /* Add 1 byte for filter byte of each scanline */

			data = (cc_uint8*)Mem_TryAlloc(bmp->height, max(scanlineBytes, bmp->width * 4));
			bmp->scan0 = (BitmapCol*)data;
			if (!bmp->scan0) return ERR_OUT_OF_MEMORY;

			bufferLen = bmp->height * scanlineBytes;
		} break;

		/* 11.2.3 PLTE Palette */
		case PNG_FourCC('P','L','T','E'): {
			if (dataSize > PNG_PALETTE * 3) return PNG_ERR_PAL_SIZE;
			if ((dataSize % 3) != 0)        return PNG_ERR_PAL_SIZE;

			res = Stream_Read(stream, buffer, dataSize);
			if (res) return res;

			for (i = 0; i < dataSize; i += 3) {
				palette[i / 3] &= BITMAPCOLOR_A_MASK; /* set RGB to 0 */
				palette[i / 3] |= BitmapColor_R_Bits(buffer[i    ]);
				palette[i / 3] |= BitmapColor_G_Bits(buffer[i + 1]);
				palette[i / 3] |= BitmapColor_B_Bits(buffer[i + 2]);
			}
		} break;

		/* 11.3.2.1 tRNS Transparency */
		case PNG_FourCC('t','R','N','S'): {
			if (colorspace == PNG_COLOR_GRAYSCALE) {
				if (dataSize != 2) return PNG_ERR_TRANS_COUNT;

				res = Stream_Read(stream, buffer, dataSize);
				if (res) return res;

				/* RGB is always two bytes */
				trnsColor = ExpandRGB(bitsPerSample, buffer[1], buffer[1], buffer[1]);
			} else if (colorspace == PNG_COLOR_INDEXED) {
				if (dataSize > PNG_PALETTE) return PNG_ERR_TRANS_COUNT;

				res = Stream_Read(stream, buffer, dataSize);
				if (res) return res;

				/* set alpha component of palette */
				for (i = 0; i < dataSize; i++) {
					palette[i] &= BITMAPCOLOR_RGB_MASK; /* set A to 0 */
					palette[i] |= BitmapColor_A_Bits(buffer[i]);
				}
			} else if (colorspace == PNG_COLOR_RGB) {
				if (dataSize != 6) return PNG_ERR_TRANS_COUNT;

				res = Stream_Read(stream, buffer, dataSize);
				if (res) return res;

				/* R,G,B are always two bytes */
				trnsColor = ExpandRGB(bitsPerSample, buffer[1], buffer[3], buffer[5]);
			} else {
				return PNG_ERR_TRANS_INVALID;
			}
		} break;

		/* 11.2.4 IDAT Image data */
		case PNG_FourCC('I','D','A','T'): {
			Stream_ReadonlyPortion(&datStream, stream, dataSize);
			inflate.Source = &datStream;

			/* TODO: This assumes zlib header will be in 1 IDAT chunk */
			while (!zlibHeader.done) {
				if ((res = ZLibHeader_Read(&datStream, &zlibHeader))) return res;
			}

			if (!bmp->scan0) return PNG_ERR_NO_DATA;
			if (rowY >= bmp->height) break;
			left = bufferLen - bufferIdx;

			res  = compStream.Read(&compStream, &data[bufferIdx], left, &read);
			if (res) return res;
			if (!read) break;

			available += read;
			bufferIdx += read;

			/* Process all of the scanline(s) that have been fully decompressed */
			/* NOTE: Need to check height too, in case IDAT is corrupted and has extra data */
			for (; available >= scanlineBytes && rowY < bmp->height; rowY++, available -= scanlineBytes) {
				cc_uint8* scanline = &data[rowY * scanlineBytes];
				if (scanline[0] > PNG_FILTER_PAETH) return PNG_ERR_INVALID_SCANLINE;

				if (rowY == 0) {
					/* First row, prior is assumed as 0 */
					Png_ReconstructFirst(scanline[0], bytesPerPixel, &scanline[1], scanlineSize);
				} else {
					cc_uint8* prior = &data[(rowY - 1) * scanlineBytes];
					Png_Reconstruct(scanline[0], bytesPerPixel, &scanline[1], &prior[1], scanlineSize);
					
					/* With the RGBA colourspace, each scanline is (1 + width*4) bytes wide */
					/* Therefore once a row has been reconstructed, the prior row can be converted */
					/* immediately into the destination colour format */
					if (colorspace == PNG_COLOR_RGB_A) {
						/* Prior line is no longer needed and can be overwritten now */
						rowExpander(bmp->width, palette, &prior[1], Bitmap_GetRow(bmp, rowY - 1));
					}
				}

				/* Current line is also no longer needed and can be overwritten now */
				if (colorspace == PNG_COLOR_RGB_A && rowY == bmp->height - 1) {
					rowExpander(bmp->width, palette, &scanline[1], Bitmap_GetRow(bmp, rowY));
				}
			}

			/* Check if image fully decoded or not */
			if (bufferIdx != bufferLen) break;

			/* With other colourspaces, the length of a scanline might be less than the width of a 32 bpp image row */
			/* Therefore image expansion can only be done after all the rows have been reconstructed, and must */
			/*  be done backwards to avoid overwriting any source data that has yet to be processed */
			/* This is slightly slower, but the majority of images ClassiCube encounters are RGBA anyways */
			if (colorspace != PNG_COLOR_RGB_A) {
				for (curY = bmp->height - 1; curY >= 0; curY--) {
					cc_uint8* scanline = &data[curY * scanlineBytes];
					rowExpander(bmp->width, palette, &scanline[1], Bitmap_GetRow(bmp, curY));
				}
			}

			if (!BitmapCol_A(trnsColor)) ComputeTransparency(bmp, trnsColor);
			return 0;
		} break;

		/* 11.2.5 IEND Image trailer */
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
#ifdef CC_BUILD_FILESYSTEM
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
	int bestFilter   = PNG_FILTER_SUB;
	int bestEstimate = Int32_MaxValue;
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

static BitmapCol* DefaultGetRow(struct Bitmap* bmp, int y, void* ctx) { return Bitmap_GetRow(bmp, y); }
static cc_result Png_EncodeCore(struct Bitmap* bmp, struct Stream* stream, cc_uint8* buffer,
					Png_RowGetter getRow, cc_bool alpha, void* ctx) {
	cc_uint8 tmp[32];
	cc_uint8* prevLine = buffer;
	cc_uint8*  curLine = buffer + (bmp->width * 4) * 1;
	cc_uint8* bestLine = buffer + (bmp->width * 4) * 2;

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
		BitmapCol* src = getRow(bmp, y, ctx);
		cc_uint8* prev = (y & 1) == 0 ? prevLine : curLine;
		cc_uint8* cur  = (y & 1) == 0 ? curLine  : prevLine;

		Png_MakeRow(src, cur, lineSize, alpha);
		Png_EncodeRow(cur, prev, bestLine, lineSize, alpha);

		/* +1 for filter byte */
		if ((res = Stream_Write(&zlStream, bestLine, lineSize + 1))) return res;
	}
	if ((res = zlStream.Close(&zlStream))) return res;
	Stream_SetU32_BE(&tmp[0], chunk.meta.crc32.crc32 ^ 0xFFFFFFFFUL);

	/* Write end chunk */
	Stream_SetU32_BE(&tmp[4],  0);
	Stream_SetU32_BE(&tmp[8],  PNG_FourCC('I','E','N','D'));
	Stream_SetU32_BE(&tmp[12], 0xAE426082UL); /* CRC32 of IEND */
	if ((res = Stream_Write(stream, tmp, 16))) return res;

	/* Come back to fixup size of data in data chunk */
	if ((res = stream->Position(stream, &stream_end))) return res;
	if ((res = stream->Seek(stream, stream_beg + 33))) return res;

	Stream_SetU32_BE(&tmp[0], (stream_end - stream_beg) - 57);
	if ((res = Stream_Write(stream, tmp, 4))) return res;
	return stream->Seek(stream, stream_end);
}

cc_result Png_Encode(struct Bitmap* bmp, struct Stream* stream, 
					Png_RowGetter getRow, cc_bool alpha, void* ctx) {
	cc_result res;
	/* Add 1 for scanline filter type byter */
	cc_uint8* buffer = (cc_uint8*)Mem_TryAlloc(3, bmp->width * 4 + 1);
	if (!buffer) return ERR_NOT_SUPPORTED;

	res = Png_EncodeCore(bmp, stream, buffer, getRow, alpha, ctx);
	Mem_Free(buffer);
	return res;
}
#else
/* No point including encoding code when can't save screenshots anyways */
cc_result Png_Encode(struct Bitmap* bmp, struct Stream* stream, 
					Png_RowGetter getRow, cc_bool alpha, void* ctx) {
	return ERR_NOT_SUPPORTED;
}
#endif

