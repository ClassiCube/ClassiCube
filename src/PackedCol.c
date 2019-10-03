#include "PackedCol.h"
#include "ExtMath.h"

PackedCol PackedCol_Scale(PackedCol value, float t) {
	value.R = (cc_uint8)(value.R * t);
	value.G = (cc_uint8)(value.G * t);
	value.B = (cc_uint8)(value.B * t);
	return value;
}

PackedCol PackedCol_Lerp(PackedCol a, PackedCol b, float t) {
	a.R = (cc_uint8)Math_Lerp(a.R, b.R, t);
	a.G = (cc_uint8)Math_Lerp(a.G, b.G, t);
	a.B = (cc_uint8)Math_Lerp(a.B, b.B, t);
	return a;
}

PackedCol PackedCol_Tint(PackedCol a, PackedCol b) {
	a.R = (cc_uint8)(a.R * b.R / 255);
	a.G = (cc_uint8)(a.G * b.G / 255);
	a.B = (cc_uint8)(a.B * b.B / 255);
	return a;
}

void PackedCol_GetShaded(PackedCol normal, PackedCol* xSide, PackedCol* zSide, PackedCol* yMin) {
	*xSide = PackedCol_Scale(normal, PACKEDCOL_SHADE_X);
	*zSide = PackedCol_Scale(normal, PACKEDCOL_SHADE_Z);
	*yMin  = PackedCol_Scale(normal, PACKEDCOL_SHADE_YMIN);
}

static int PackedCol_DeHex(char hex) {
	if (hex >= '0' && hex <= '9') {
		return (hex - '0');
	} else if (hex >= 'a' && hex <= 'f') {
		return (hex - 'a') + 10;
	} else if (hex >= 'A' && hex <= 'F') {
		return (hex - 'A') + 10;
	}
	return -1;
}

bool PackedCol_Unhex(const char* src, int* dst, int count) {
	int i;
	for (i = 0; i < count; i++) {
		dst[i] = PackedCol_DeHex(src[i]);
		if (dst[i] == -1) return false;
	}
	return true;
}

void PackedCol_ToHex(String* str, PackedCol value) {
	String_AppendHex(str, value.R);
	String_AppendHex(str, value.G);
	String_AppendHex(str, value.B);
}

bool PackedCol_TryParseHex(const String* str, PackedCol* value) {
	int bits[6];
	char* buffer;
		
	buffer      = str->buffer;
	value->_raw = 0;

	/* accept XXYYZZ or #XXYYZZ forms */
	if (str->length < 6) return false;
	if (str->length > 6 && (str->buffer[0] != '#' || str->length > 7)) return false;

	if (buffer[0] == '#') buffer++;
	if (!PackedCol_Unhex(buffer, bits, 6)) return false;

	value->R = (cc_uint8)((bits[0] << 4) | bits[1]);
	value->G = (cc_uint8)((bits[2] << 4) | bits[3]);
	value->B = (cc_uint8)((bits[4] << 4) | bits[5]);
	value->A = 255;
	return true;
}
