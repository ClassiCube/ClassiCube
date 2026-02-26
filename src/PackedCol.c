#include "PackedCol.h"
#include "String_.h"
#include "ExtMath.h"

PackedCol PackedCol_Scale(PackedCol a, float t) {
	cc_uint8 R = (cc_uint8)(PackedCol_R(a) * t);
	cc_uint8 G = (cc_uint8)(PackedCol_G(a) * t);
	cc_uint8 B = (cc_uint8)(PackedCol_B(a) * t);
	return (a & PACKEDCOL_A_MASK) | PackedCol_R_Bits(R) | PackedCol_G_Bits(G) | PackedCol_B_Bits(B);
}

PackedCol PackedCol_Lerp(PackedCol a, PackedCol b, float t) {
	cc_uint8 R = (cc_uint8)Math_Lerp(PackedCol_R(a), PackedCol_R(b), t);
	cc_uint8 G = (cc_uint8)Math_Lerp(PackedCol_G(a), PackedCol_G(b), t);
	cc_uint8 B = (cc_uint8)Math_Lerp(PackedCol_B(a), PackedCol_B(b), t);
	return (a & PACKEDCOL_A_MASK) | PackedCol_R_Bits(R) | PackedCol_G_Bits(G) | PackedCol_B_Bits(B);
}

PackedCol PackedCol_Tint(PackedCol a, PackedCol b) {
	cc_uint32 R = PackedCol_R(a) * PackedCol_R(b) / 255;
	cc_uint32 G = PackedCol_G(a) * PackedCol_G(b) / 255;
	cc_uint32 B = PackedCol_B(a) * PackedCol_B(b) / 255;
	/* TODO: don't shift when multiplying */
	return (a & PACKEDCOL_A_MASK) | (R << PACKEDCOL_R_SHIFT) | (G << PACKEDCOL_G_SHIFT) | (B << PACKEDCOL_B_SHIFT);
}

PackedCol PackedCol_ScreenBlend(PackedCol a, PackedCol b) {
	PackedCol finalColor, aInverted, bInverted;
	cc_uint8 R, G, B;
	/* With Screen blend mode, the values of the pixels in the two layers are inverted, multiplied, and then inverted again. */
	R = 255 - PackedCol_R(a);
	G = 255 - PackedCol_G(a);
	B = 255 - PackedCol_B(a);
	aInverted = PackedCol_Make(R, G, B, 255);

	R = 255 - PackedCol_R(b);
	G = 255 - PackedCol_G(b);
	B = 255 - PackedCol_B(b);
	bInverted = PackedCol_Make(R, G, B, 255);

	finalColor = PackedCol_Tint(aInverted, bInverted);
	R = 255 - PackedCol_R(finalColor);
	G = 255 - PackedCol_G(finalColor);
	B = 255 - PackedCol_B(finalColor);
	return PackedCol_Make(R, G, B, 255);
}

void PackedCol_GetShaded(PackedCol normal, PackedCol* xSide, PackedCol* zSide, PackedCol* yMin) {
	*xSide = PackedCol_Scale(normal, PACKEDCOL_SHADE_X);
	*zSide = PackedCol_Scale(normal, PACKEDCOL_SHADE_Z);
	*yMin  = PackedCol_Scale(normal, PACKEDCOL_SHADE_YMIN);
}

int PackedCol_DeHex(char hex) {
	if (hex >= '0' && hex <= '9') {
		return (hex - '0');
	} else if (hex >= 'a' && hex <= 'f') {
		return (hex - 'a') + 10;
	} else if (hex >= 'A' && hex <= 'F') {
		return (hex - 'A') + 10;
	}
	return -1;
}

cc_bool PackedCol_Unhex(const char* src, int* dst, int count) {
	int i;
	for (i = 0; i < count; i++) {
		dst[i] = PackedCol_DeHex(src[i]);
		if (dst[i] == -1) return false;
	}
	return true;
}

void PackedCol_ToHex(cc_string* str, PackedCol value) {
	String_AppendHex(str, PackedCol_R(value));
	String_AppendHex(str, PackedCol_G(value));
	String_AppendHex(str, PackedCol_B(value));
}

cc_bool PackedCol_TryParseHex(const cc_string* str, cc_uint8* rgb) {
	int bits[6];
	char* buffer = str->buffer;

	/* accept XXYYZZ or #XXYYZZ forms */
	if (str->length < 6) return false;
	if (str->length > 6 && (str->buffer[0] != '#' || str->length > 7)) return false;

	if (buffer[0] == '#') buffer++;
	if (!PackedCol_Unhex(buffer, bits, 6)) return false;

	rgb[0] = (cc_uint8)((bits[0] << 4) | bits[1]);
	rgb[1] = (cc_uint8)((bits[2] << 4) | bits[3]);
	rgb[2] = (cc_uint8)((bits[4] << 4) | bits[5]);
	return true;
}
