#include "Bitmap.h"
#include "Platform.h"

void Bitmap_Create(Bitmap* bmp, Int32 width, Int32 height, Int32 stride, UInt8* scan0) {
	bmp->Width = width; bmp->Height = height; bmp->Stride = stride; bmp->Scan0 = scan0;
}

UInt32* Bitmap_GetRow(Bitmap* bmp, Int32 y) {
	return (UInt32*)(bmp->Scan0 + (y * bmp->Stride));
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
	bmp->Stride = width * sizeof(UInt32);
	bmp->Scan0 = Platform_MemAlloc(Bitmap_DataSize(width, height));
}