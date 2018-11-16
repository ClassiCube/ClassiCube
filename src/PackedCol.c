#include "PackedCol.h"
#include "ExtMath.h"

bool PackedCol_Equals(PackedCol a, PackedCol b) {
	return a.R == b.R && a.G == b.G && a.B == b.B && a.A == b.A;
}

PackedCol PackedCol_Scale(PackedCol value, float t) {
	value.R = (uint8_t)(value.R * t);
	value.G = (uint8_t)(value.G * t);
	value.B = (uint8_t)(value.B * t);
	return value;
}

PackedCol PackedCol_Lerp(PackedCol a, PackedCol b, float t) {
	a.R = (uint8_t)Math_Lerp(a.R, b.R, t);
	a.G = (uint8_t)Math_Lerp(a.G, b.G, t);
	a.B = (uint8_t)Math_Lerp(a.B, b.B, t);
	return a;
}

void PackedCol_GetShaded(PackedCol normal, PackedCol* xSide, PackedCol* zSide, PackedCol* yMin) {
	*xSide = PackedCol_Scale(normal, PACKEDCOL_SHADE_X);
	*zSide = PackedCol_Scale(normal, PACKEDCOL_SHADE_Z);
	*yMin  = PackedCol_Scale(normal, PACKEDCOL_SHADE_YMIN);
}

bool PackedCol_Unhex(char hex, int* value) {
	*value = 0;
	if (hex >= '0' && hex <= '9') {
		*value = (hex - '0');
	} else if (hex >= 'a' && hex <= 'f') {
		*value = (hex - 'a') + 10;
	} else if (hex >= 'A' && hex <= 'F') {
		*value = (hex - 'A') + 10;
	} else {
		return false;
	}
	return true;
}

void PackedCol_ToHex(String* str, PackedCol value) {
	String_AppendHex(str, value.R);
	String_AppendHex(str, value.G);
	String_AppendHex(str, value.B);
}

bool PackedCol_TryParseHex(const String* str, PackedCol* value) {
	PackedCol colZero = PACKEDCOL_CONST(0, 0, 0, 0);
	int rH, rL, gH, gL, bH, bL;
	char* buffer;
		
	buffer = str->buffer;
	*value = colZero;

	/* accept XXYYZZ or #XXYYZZ forms */
	if (str->length < 6) return false;
	if (str->length > 6 && (str->buffer[0] != '#' || str->length > 7)) return false;
	if (buffer[0] == '#') buffer++;

	if (!PackedCol_Unhex(buffer[0], &rH) || !PackedCol_Unhex(buffer[1], &rL)) return false;
	if (!PackedCol_Unhex(buffer[2], &gH) || !PackedCol_Unhex(buffer[3], &gL)) return false;
	if (!PackedCol_Unhex(buffer[4], &bH) || !PackedCol_Unhex(buffer[5], &bL)) return false;

	value->R = (uint8_t)((rH << 4) | rL);
	value->G = (uint8_t)((gH << 4) | gL);
	value->B = (uint8_t)((bH << 4) | bL);
	value->A = 255;
	return true;
}
