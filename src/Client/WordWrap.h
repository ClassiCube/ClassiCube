#ifndef CC_WORDWRAP_H
#define CC_WORDWRAP_H
#include "String.h"
/* Allows wrapping a single line of text into multiple lines.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
#define WORDWRAP_MAX_LINES_TO_WRAP 128
#define WORDWRAP_MAX_BUFFER_SIZE 2048

void WordWrap_Do(STRING_TRANSIENT String* text, STRING_TRANSIENT String* lines, Int32 numLines, Int32 maxPerLine);
/* Calculates where the given raw index is located in the wrapped lines. */
void WordWrap_GetCoords(Int32 index, STRING_PURE String* lines, Int32 numLines, Int32* coordX, Int32* coordY);
Int32 WordWrap_GetBackLength(STRING_PURE String* text, Int32 index);
Int32 WordWrap_GetForwardLength(STRING_PURE String* text, Int32 index);
#endif