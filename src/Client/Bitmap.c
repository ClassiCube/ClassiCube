#include "Bitmap.h"
#include "Platform.h"
#include "PackedCol.h"
#include "ExtMath.h"
#include "Deflate.h"

void Bitmap_Create(Bitmap* bmp, Int32 width, Int32 height, UInt8* scan0) {
	bmp->Width = width; bmp->Height = height;
	bmp->Stride = width * Bitmap_PixelBytesSize;
	bmp->Scan0 = scan0;
}
void Bitmap_CopyBlock(Int32 srcX, Int32 srcY, Int32 dstX, Int32 dstY, Bitmap* src, Bitmap* dst, Int32 size) {
	Int32 x, y;
	for (y = 0; y < size; y++) {
		UInt32* srcRow = Bitmap_GetRow(src, srcY + y);
		UInt32* dstRow = Bitmap_GetRow(dst, dstY + y);
		for (x = 0; x < size; x++) {
			dstRow[dstX + x] = srcRow[srcX + x];
		}
	}
}

void Bitmap_CopyRow(Int32 srcY, Int32 dstY, Bitmap* src, Bitmap* dst, Int32 width) {
	UInt32* srcRow = Bitmap_GetRow(src, srcY);
	UInt32* dstRow = Bitmap_GetRow(dst, dstY);
	Int32 x;
	for (x = 0; x < width; x++) {
		dstRow[x] = srcRow[x];
	}
}

void Bitmap_Allocate(Bitmap* bmp, Int32 width, Int32 height) {
	bmp->Width = width; bmp->Height = height;
	bmp->Stride = width * Bitmap_PixelBytesSize;
	bmp->Scan0 = Platform_MemAlloc(Bitmap_DataSize(width, height));
	
	if (bmp->Scan0 == NULL) {
		ErrorHandler_Fail("Bitmap - failed to allocate memory");
	}
}

void Bitmap_AllocatePow2(Bitmap* bmp, Int32 width, Int32 height) {
	width = Math_NextPowOf2(width);
	height = Math_NextPowOf2(height);
	Bitmap_Allocate(bmp, width, height);
}


#define PNG_HEADER 8
#define PNG_RGB_MASK 0xFFFFFFUL
#define PNG_PALETTE 256
#define PNG_FOURCC(a, b, c, d) ((UInt32)a << 24) | ((UInt32)b << 16) | ((UInt32)c << 8) | (UInt32)d

#define PNG_COL_GRAYSCALE 0
#define PNG_COL_RGB 2
#define PNG_COL_INDEXED 3
#define PNG_COL_GRAYSCALE_A 4
#define PNG_COL_RGB_A 6

#define PNG_FILTER_NONE 0
#define PNG_FILTER_SUB 1
#define PNG_FILTER_UP 2
#define PNG_FILTER_AVERAGE 3
#define PNG_FILTER_PAETH 4

typedef void(*Png_RowExpander)(UInt8 bpp, Int32 width, UInt32* palette, UInt8* src, UInt32* dst);

void Png_CheckHeader(Stream* stream) {
	UInt8 header[PNG_HEADER];
	static UInt8 sig[PNG_HEADER] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	Stream_Read(stream, header, PNG_HEADER);

	Int32 i;
	for (i = 0; i < PNG_HEADER; i++) {
		if (header[i] != sig[i]) ErrorHandler_Fail("Invalid PNG header");
	}
}

void Png_Filter(UInt8 type, UInt8 bytesPerPixel, UInt8* line, UInt8* prior, UInt32 lineLen) {
	UInt32 i, j;
	switch (type) {
	case PNG_FILTER_NONE:
		return;

	case PNG_FILTER_SUB:
		for (i = bytesPerPixel, j = 0; i < lineLen; i++, j++) {
			line[i] = (UInt8)(line[i] + line[j]);
		}
		return;

	case PNG_FILTER_UP:
		for (i = 0; i < lineLen; i++) {
			line[i] = (UInt8)(line[i] + prior[i]);
		}
		return;

	case PNG_FILTER_AVERAGE:
		for (i = 0; i < bytesPerPixel; i++) {
			line[i] = (UInt8)(line[i] + (prior[i] >> 1));
		}
		for (j = 0; i < lineLen; i++, j++) {
			line[i] = (UInt8)(line[i] + ((prior[i] + line[j]) >> 1));
		}
		return;

	case PNG_FILTER_PAETH:
		/* TODO: verify this is right */
		for (i = bytesPerPixel, j = 0; i < lineLen; i++, j++) {
			UInt8 a = line[j], b = prior[i], c = prior[j];
			Int32 p = a + b - c;
			Int32 pa = Math_AbsI(p - a);
			Int32 pb = Math_AbsI(p - b);
			Int32 pc = Math_AbsI(p - c);

			if (pa <= pb && pa <= pc) { line[i] = (UInt8)(line[i] + a); } else if (pb <= pc) { line[i] = (UInt8)(line[i] + b); } else { line[i] = (UInt8)(line[i] + c); }
		}
		return;

	default:
		ErrorHandler_Fail("PNG scanline has invalid filter type");
		return;
	}
}

void Png_Expand_GRAYSCALE(UInt8 bitsPerSample, Int32 width, UInt32* palette, UInt8* src, UInt32* dst) {
	Int32 i, j, mask;
	UInt8 cur, rgb1, rgb2, rgb3, rgb4;
#define PNG_DO_GRAYSCALE(tmp, dstI, srcI, scale) tmp = src[srcI] * scale; dst[dstI] = PackedCol_ARGB(tmp, tmp, tmp, 255);
#define PNG_DO_GRAYSCALE_X(tmp, dstI, srcI) tmp = src[srcI]; dst[dstI] = PackedCol_ARGB(tmp, tmp, tmp, 255);

	switch (bitsPerSample) {
	case 1:
		for (i = 0, j = 0; i < (width & ~0x7); i += 8, j++) {
			cur = src[j];
			PNG_DO_GRAYSCALE(rgb1, i    , (cur >> 7)    , 255); PNG_DO_GRAYSCALE(rgb2, i + 1, (cur >> 6) & 1, 255);
			PNG_DO_GRAYSCALE(rgb3, i + 2, (cur >> 5) & 1, 255); PNG_DO_GRAYSCALE(rgb4, i + 3, (cur >> 4) & 1, 255);
			PNG_DO_GRAYSCALE(rgb1, i + 4, (cur >> 3) & 1, 255); PNG_DO_GRAYSCALE(rgb2, i + 5, (cur >> 2) & 1, 255);
			PNG_DO_GRAYSCALE(rgb3, i + 6, (cur >> 1) & 1, 255); PNG_DO_GRAYSCALE(rgb4, i + 7, (cur     ) & 1, 255);
		}
		for (; i < width; i++) {
			mask = (7 - (i & 7));
			PNG_DO_GRAYSCALE(rgb1, i, (src[j] >> mask) & 1, 255);
		}
		return;
	case 2:
		for (i = 0, j = 0; i < (width & ~0x3); i += 4, j++) {
			cur = src[j];
			PNG_DO_GRAYSCALE(rgb1, i    , (cur >> 6)    , 85); PNG_DO_GRAYSCALE(rgb2, i + 1, (cur >> 4) & 3, 85);
			PNG_DO_GRAYSCALE(rgb3, i + 2, (cur >> 2) & 3, 85); PNG_DO_GRAYSCALE(rgb4, i + 3, (cur     ) & 3, 85);
		}
		for (; i < width; i++) {
			mask = (3 - (i & 3)) * 2;
			PNG_DO_GRAYSCALE(rgb1, i, (src[j] >> mask) & 3, 85);
		}
		return;
	case 4:
		for (i = 0, j = 0; i < (width & ~0x1); i += 2, j++) {
			cur = src[j];
			PNG_DO_GRAYSCALE(rgb1, i, cur >> 4, 17); PNG_DO_GRAYSCALE(rgb1, i + 1, cur & 0x0F, 17);
		}
		for (; i < width; i++) {
			mask = (1 - (i & 1)) * 4;
			PNG_DO_GRAYSCALE(rgb1, i, (src[j] >> mask) & 15, 17);
		}
		return;
	case 8:
		for (i = 0; i < (width & ~0x3); i += 4) {
			PNG_DO_GRAYSCALE_X(rgb1, i    , i    ); PNG_DO_GRAYSCALE_X(rgb2, i + 1, i + 1);
			PNG_DO_GRAYSCALE_X(rgb3, i + 2, i + 2); PNG_DO_GRAYSCALE_X(rgb4, i + 3, i + 3);
		}
		for (; i < width; i++) { PNG_DO_GRAYSCALE_X(rgb1, i, i); }
		return;
	case 16:
		for (i = 0; i < width; i++) { PNG_DO_GRAYSCALE_X(rgb1, i, i << 1); }
		return;
	}
}

void Png_Expand_RGB(UInt8 bitsPerSample, Int32 width, UInt32* palette, UInt8* src, UInt32* dst) {
	Int32 i, j;
#define PNG_DO_RGB__8(dstI, srcI) dst[dstI] = PackedCol_ARGB(src[srcI], src[srcI + 1], src[srcI + 2], 255);
#define PNG_DO_RGB_16(dstI, srcI) dst[dstI] = PackedCol_ARGB(src[srcI], src[srcI + 3], src[srcI + 5], 255);

	if (bitsPerSample == 8) {
		for (i = 0, j = 0; i < (width & ~0x03); i += 4, j += 12) {
			PNG_DO_RGB__8(i    , j    ); PNG_DO_RGB__8(i + 1, j + 3);
			PNG_DO_RGB__8(i + 2, j + 6); PNG_DO_RGB__8(i + 3, j + 9);
		}
		for (; i < width; i++, j += 3) { PNG_DO_RGB__8(i, j); }
	} else {
		for (i = 0, j = 0; i < width; i++, j += 6) { PNG_DO_RGB_16(i, j); }
	}
}

void Png_Expand_INDEXED(UInt8 bitsPerSample, Int32 width, UInt32* palette, UInt8* src, UInt32* dst) {
	Int32 i, j, mask;
	UInt8 cur;
#define PNG_DO_INDEXED(dstI, srcI) dst[dstI] = palette[srcI];

	switch (bitsPerSample) {
	case 1:
		for (i = 0, j = 0; i < (width & ~0x7); i += 8, j++) {
			cur = src[j];
			PNG_DO_INDEXED(i    , (cur >> 7)    ); PNG_DO_INDEXED(i + 1, (cur >> 6) & 1);
			PNG_DO_INDEXED(i + 2, (cur >> 5) & 1); PNG_DO_INDEXED(i + 3, (cur >> 4) & 1);
			PNG_DO_INDEXED(i + 4, (cur >> 3) & 1); PNG_DO_INDEXED(i + 5, (cur >> 2) & 1);
			PNG_DO_INDEXED(i + 6, (cur >> 1) & 1); PNG_DO_INDEXED(i + 7, (cur     ) & 1);
		}
		for (; i < width; i++) {
			mask = (7 - (i & 7));
			PNG_DO_INDEXED(i, (src[j] >> mask) & 1);
		}
		return;
	case 2:
		for (i = 0, j = 0; i < (width & ~0x3); i += 4, j++) {
			cur = src[j];
			PNG_DO_INDEXED(i    , (cur >> 6)    ); PNG_DO_INDEXED(i + 1, (cur >> 4) & 3);
			PNG_DO_INDEXED(i + 2, (cur >> 2) & 3); PNG_DO_INDEXED(i + 3, (cur     ) & 3);
		}
		for (; i < width; i++) {
			mask = (3 - (i & 3)) * 2;
			PNG_DO_INDEXED(i, (src[j] >> mask) & 3);
		}
		return;
	case 4:
		for (i = 0, j = 0; i < (width & ~0x1); i += 2, j++) {
			cur = src[j];
			PNG_DO_INDEXED(i, cur >> 4); PNG_DO_INDEXED(i + 1, cur & 0x0F);
		}
		for (; i < width; i++) {
			mask = (1 - (i & 1)) * 4;
			PNG_DO_INDEXED(i, (src[j] >> mask) & 15);
		}
		return;
	case 8:
		for (i = 0; i < (width & ~0x3); i += 4) {
			PNG_DO_INDEXED(i    , src[i]    ); PNG_DO_INDEXED(i + 1, src[i + 1]);
			PNG_DO_INDEXED(i + 2, src[i + 2]); PNG_DO_INDEXED(i + 3, src[i + 3]);
		}
		for (; i < width; i++) { PNG_DO_INDEXED(i, src[i]); }
		return;
	}
}

void Png_Expand_GRAYSCALE_A(UInt8 bitsPerSample, Int32 width, UInt32* palette, UInt8* src, UInt32* dst) {
	Int32 i, j;
	UInt8 rgb1, rgb2, rgb3, rgb4;
#define PNG_DO_GRAYSCALE_A__8(tmp, dstI, srcI) tmp = src[srcI]; dst[dstI] = PackedCol_ARGB(tmp, tmp, tmp, src[srcI + 1]);
#define PNG_DO_GRAYSCALE_A_16(tmp, dstI, srcI) tmp = src[srcI]; dst[dstI] = PackedCol_ARGB(tmp, tmp, tmp, src[srcI + 2]);

	if (bitsPerSample == 8) {
		for (i = 0, j = 0; i < (width & ~0x3); i += 4, j += 8) {
			PNG_DO_GRAYSCALE_A__8(rgb1, i    , j    ); PNG_DO_GRAYSCALE_A__8(rgb2, i + 1, j + 2);
			PNG_DO_GRAYSCALE_A__8(rgb3, i + 2, j + 4); PNG_DO_GRAYSCALE_A__8(rgb4, i + 3, j + 5);
		}
		for (; i < width; i++, j += 2) { PNG_DO_GRAYSCALE_A__8(rgb1, i, j); }
	} else {
		for (i = 0, j = 0; i < width; i++, j += 4) { PNG_DO_GRAYSCALE_A_16(rgb1, i, j); }
	}
}

void Png_Expand_RGB_A(UInt8 bitsPerSample, Int32 width, UInt32* palette, UInt8* src, UInt32* dst) {
	Int32 i, j;
#define PNG_DO_RGB_A__8(dstI, srcI) dst[dstI] = PackedCol_ARGB(src[srcI], src[srcI + 1], src[srcI + 2], src[srcI + 3]);
#define PNG_DO_RGB_A_16(dstI, srcI) dst[dstI] = PackedCol_ARGB(src[srcI], src[srcI + 3], src[srcI + 5], src[srcI + 7]);

	if (bitsPerSample == 8) {
		for (i = 0, j = 0; i < (width & ~0x3); i += 4, j += 16) {
			PNG_DO_RGB_A__8(i    , j    ); PNG_DO_RGB_A__8(i + 1, j + 4 );
			PNG_DO_RGB_A__8(i + 2, j + 8); PNG_DO_RGB_A__8(i + 3, j + 12);
		}
		for (; i < width; i++, j += 4) { PNG_DO_RGB_A__8(i, j); }
	} else {
		for (i = 0, j = 0; i < width; i++, j += 8) { PNG_DO_RGB_A_16(i, j); }
	}
}

void Png_ComputeTransparency(Bitmap* bmp, UInt32 transparentCol) {
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

/* TODO: Test a lot of .png files and ensure output is right */
#define PNG_MAX_DIMS 0x8000L
/* Most bits per sample is 16. Most samples per pixel is 4. Add 1 for filter byte. */
#define PNG_BUFFER_SIZE ((PNG_MAX_DIMS * 2 * 4 + 1) * 2)
void Bitmap_DecodePng(Bitmap* bmp, Stream* stream) {
	Png_CheckHeader(stream);
	Bitmap_Create(bmp, 0, 0, NULL);
	UInt32 transparentCol = PackedCol_ARGB(0, 0, 0, 255);
	UInt8 col, bitsPerSample, bytesPerPixel;
	Png_RowExpander rowExpander;

	UInt32 palette[PNG_PALETTE];
	UInt32 i;
	for (i = 0; i < PNG_PALETTE; i++) {
		palette[i] = PackedCol_ARGB(0, 0, 0, 255);
	}

	bool gotHeader = false, readingChunks = true, initDeflate = false;
	DeflateState deflate;
	Stream compStream;
	ZLibHeader zlibHeader;
	ZLibHeader_Init(&zlibHeader);

	UInt32 scanlineSize, scanlineBytes, curY = 0;
	UInt8 buffer[PNG_BUFFER_SIZE];
	UInt32 bufferIdx, bufferLen, bufferCur;
	UInt32 scanlineIndices[2];

	while (readingChunks) {
		UInt32 dataSize = Stream_ReadUInt32_BE(stream);
		UInt32 fourCC = Stream_ReadUInt32_BE(stream);

		switch (fourCC) {
		case PNG_FOURCC('I', 'H', 'D', 'R'): {
			if (dataSize != 13) ErrorHandler_Fail("PNG header chunk has invalid size");
			gotHeader = true;

			bmp->Width = Stream_ReadInt32_BE(stream);
			bmp->Height = Stream_ReadInt32_BE(stream);
			if (bmp->Width  < 0 || bmp->Width  > PNG_MAX_DIMS) ErrorHandler_Fail("PNG image too wide");
			if (bmp->Height < 0 || bmp->Height > PNG_MAX_DIMS) ErrorHandler_Fail("PNG image too tall");

			bmp->Stride = bmp->Width * Bitmap_PixelBytesSize;
			bmp->Scan0 = Platform_MemAlloc(Bitmap_DataSize(bmp->Width, bmp->Height));
			if (bmp->Scan0 == NULL) ErrorHandler_Fail("Failed to allocate memory for PNG bitmap");

			bitsPerSample = Stream_ReadUInt8(stream);
			if (bitsPerSample > 16 || !Math_IsPowOf2(bitsPerSample)) ErrorHandler_Fail("PNG has invalid bits per pixel");
			col = Stream_ReadUInt8(stream);
			if (col == 1 || col == 3 || col > 6) ErrorHandler_Fail("PNG has invalid colour type");
			if (bitsPerSample < 8 && (col >= PNG_COL_RGB && col != PNG_COL_INDEXED)) ErrorHandler_Fail("PNG has invalid bpp for this colour type");
			if (bitsPerSample == 16 && col == PNG_COL_INDEXED) ErrorHandler_Fail("PNG has invalid bpp for this colour type");

			if (Stream_ReadUInt8(stream) != 0) ErrorHandler_Fail("PNG compression method must be DEFLATE");
			if (Stream_ReadUInt8(stream) != 0) ErrorHandler_Fail("PNG filter method must be ADAPTIVE");
			if (Stream_ReadUInt8(stream) != 0) ErrorHandler_Fail("PNG interlacing not supported");

			UInt32 samplesPerPixel[7] = { 1,0,3,1,2,0,4 };
			scanlineSize = ((samplesPerPixel[col] * bitsPerSample * bmp->Width) + 7) >> 3;
			scanlineBytes = scanlineSize + 1;
			bytesPerPixel = ((samplesPerPixel[col] * bitsPerSample) + 7) >> 3;
			bufferLen = (PNG_BUFFER_SIZE / scanlineBytes) * scanlineBytes;

			scanlineIndices[0] = 0;
			scanlineIndices[1] = scanlineBytes;
			Platform_MemSet(buffer, 0, scanlineBytes); /* Prior row should be 0 per PNG spec */
			bufferIdx = scanlineBytes;
			bufferCur = scanlineBytes;

			switch (col) {
			case PNG_COL_GRAYSCALE: rowExpander = Png_Expand_GRAYSCALE; break;
			case PNG_COL_RGB: rowExpander = Png_Expand_RGB; break;
			case PNG_COL_INDEXED: rowExpander = Png_Expand_INDEXED; break;
			case PNG_COL_GRAYSCALE_A: rowExpander = Png_Expand_GRAYSCALE_A; break;
			case PNG_COL_RGB_A: rowExpander = Png_Expand_RGB_A; break;
			}
		} break;

		case PNG_FOURCC('P', 'L', 'T', 'E'): {
			if (dataSize > PNG_PALETTE * 3) ErrorHandler_Fail("PNG palette has too many entries");
			if ((dataSize % 3) != 0) ErrorHandler_Fail("PNG palette chunk has invalid size");

			UInt8 palRGB[PNG_PALETTE * 3];
			Stream_Read(stream, palRGB, dataSize);
			for (i = 0; i < dataSize; i += 3) {
				palette[i / 3] = PackedCol_ARGB(palRGB[i], palRGB[i + 1], palRGB[i + 2], 255);
			}
		} break;

		case PNG_FOURCC('t', 'R', 'N', 'S'): {
			if (col == PNG_COL_GRAYSCALE) {
				if (dataSize != 2) ErrorHandler_Fail("PNG only allows one explicit transparency colour");
				UInt8 palRGB = (UInt8)Stream_ReadUInt16_BE(stream);
				transparentCol = PackedCol_ARGB(palRGB, palRGB, palRGB, 0);
			} else if (col == PNG_COL_INDEXED) {
				if (dataSize > PNG_PALETTE) ErrorHandler_Fail("PNG transparency palette has too many entries");
				UInt8 palA[PNG_PALETTE * 3];
				Stream_Read(stream, palA, dataSize);

				for (i = 0; i < dataSize; i++) {
					palette[i] &= PNG_RGB_MASK;
					palette[i] |= (UInt32)palA[i] << 24;
				}
			} else if (col == PNG_COL_RGB) {
				if (dataSize != 6) ErrorHandler_Fail("PNG only allows one explicit transparency colour");
				UInt8 palR = (UInt8)Stream_ReadUInt16_BE(stream);
				UInt8 palG = (UInt8)Stream_ReadUInt16_BE(stream);
				UInt8 palB = (UInt8)Stream_ReadUInt16_BE(stream);
				transparentCol = PackedCol_ARGB(palR, palG, palB, 0);
			} else {
				ErrorHandler_Fail("PNG cannot have explicit transparency colour for this colour type");
			}
		} break;

		case PNG_FOURCC('I', 'D', 'A', 'T'): {
			Stream datStream;
			Stream_ReadonlyPortion(&datStream, stream, dataSize);
			if (!initDeflate) {
				Deflate_MakeStream(&compStream, &deflate, &datStream);
				initDeflate = true;
			} else {
				deflate.Source = &datStream;
			}

			/* TODO: This assumes zlib header will be in 1 IDAT chunk */
			while (!zlibHeader.Done) { ZLibHeader_Read(&datStream, &zlibHeader); }

			UInt32 scanlineBytes = (scanlineSize + 1); /* Add 1 byte for filter byte of each scanline*/
			while (curY < bmp->Height) {
				UInt32 bufferRemaining = bufferLen - bufferIdx, read;
				ReturnCode code = compStream.Read(&compStream, &buffer[bufferIdx], bufferRemaining, &read);
				if (code != 0) ErrorHandler_FailWithCode(code, "PNG - reading image bulk data");
				if (read == 0) break;			

				/* buffer is arranged like this */
				/* scanline 0 | A
				            1 | B  <-- bufferCur
				            2 | X
				            3 | X  <-- bufferIdx (for example)
				            4 | X
				*/
				/* When reading, we need to handle the case of when the buffer cycles over back to start */

				bufferIdx += read;
				while (bufferIdx >= bufferCur + scanlineBytes && curY < bmp->Height) {					
					UInt8* prior    = &buffer[scanlineIndices[0]];
					UInt8* scanline = &buffer[scanlineIndices[1]];

					Png_Filter(scanline[0], bytesPerPixel, &scanline[1], &prior[1], scanlineSize);
					rowExpander(bitsPerSample, bmp->Width, palette, &scanline[1], Bitmap_GetRow(bmp, curY));
					curY++;

					/* Advance scanlines, with wraparound behaviour */
					scanlineIndices[0] = (scanlineIndices[0] + scanlineBytes) % bufferLen;
					scanlineIndices[1] = (scanlineIndices[1] + scanlineBytes) % bufferLen;
					bufferCur += scanlineBytes;
					if (bufferCur == bufferLen) { bufferCur = 0; break; }
				}
				if (bufferIdx == bufferLen) bufferIdx = 0;
			}
		} break;

		case PNG_FOURCC('I', 'E', 'N', 'D'): {
			readingChunks = false;
			if (dataSize != 0) ErrorHandler_Fail("PNG end chunk must be empty");
		} break;

		default:
			stream->Seek(stream, dataSize, STREAM_SEEKFROM_CURRENT);
			break;
		}

		Stream_ReadUInt32_BE(stream); /* Skip CRC32 */
	}

	if (transparentCol <= PNG_RGB_MASK) {
		Png_ComputeTransparency(bmp, transparentCol);
	}
	if (bmp->Scan0 == NULL) ErrorHandler_Fail("Invalid PNG image");
}