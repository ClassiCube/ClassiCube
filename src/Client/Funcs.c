#include "Funcs.h"

bool Char_IsUpper(UInt8 c) {
	return c >= 'A' && c <= 'Z';
}

UInt8 Char_ToLower(UInt8 c) {
	if (!Char_IsUpper(c)) return c;
	return (UInt8)(c + ' ');
}