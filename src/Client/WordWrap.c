#include "WordWrap.h"
#include "Drawer2D.h"
#include "Funcs.h"
#include "Platform.h"

void WordWrap_OutputLines(String* text, String* lines, Int32* lineLens, Int32 numLines, Int32 usedLines, Int32 charsPerLine) {
	Int32 totalChars = charsPerLine * numLines, i, j;
	for (i = 0; i < totalChars; i++) {
		if (text->buffer[i] == NULL) text->buffer[i] = ' ';
	}

	/* convert %0-f to &0-f for colour preview. */
	for (i = 0; i < totalChars - 1; i++) {
		if (text->buffer[i] == '%' && Drawer2D_ValidColCode(text->buffer[i + 1])) {
			text->buffer[i] = '&';
		}
	}

	usedLines = max(1, usedLines);
	for (i = 0; i < usedLines; i++) {
		String* dst = &lines[i];
		UInt8* src = &text->buffer[i * charsPerLine];
		for (j = 0; j < lineLens[i]; j++) { String_Append(dst, src[j]); }
	}
}

bool WordWrap_IsWrapper(UInt8 c) {
	return c == NULL || c == ' ' || c == '-' || c == '>'
		|| c == '<' || c == '/' || c == '\\';
}

Int32 WordWrap_WrapLine(String* text, Int32 index, Int32 lineSize) {
	Int32 lineEnd = index + (lineSize - 1), i;
	/* wrap - but we don't want to wrap if the entire line is filled. */
	for (i = lineEnd; i >= index + 1; i--) {
		if (!WordWrap_IsWrapper(text->buffer[i])) continue;

		for (Int32 j = lineEnd; j >= i + 1; j--) {
			UInt8 c = text->buffer[j];
			String_InsertAt(text, index + lineSize, c);
			text->buffer[j] = ' ';
		}
		return (i + 1) - index;
	}
	return lineSize;
}

void WordWrap_Do(STRING_TRANSIENT String* text, STRING_TRANSIENT String* lines, Int32 numLines, Int32 maxPerLine) {
	Int32 len = text->length, i;
	Int32 lineLens[WORDWRAP_MAX_LINES_TO_WRAP];
	for (i = 0; i < numLines; i++) {
		String_Clear(&lines[i]);
		lineLens[i] = 0;
	}

	/* Need to make a copy because we mutate the characters. */
	UInt8 copyBuffer[String_BufferSize(WORDWRAP_MAX_BUFFER_SIZE)];
	String copy = String_FromRawBuffer(copyBuffer, WORDWRAP_MAX_BUFFER_SIZE);
	String_AppendString(&copy, text);

	Int32 usedLines = 0, totalChars = maxPerLine * numLines;
	for (Int32 index = 0; index < totalChars; index += maxPerLine) {
		if (copy.buffer[index] == NULL) break;
		usedLines++;
		Int32 lineEnd = index + (maxPerLine - 1), nextStart = lineEnd + 1;		

		/* Do we need word wrapping? */
		bool needWrap = !WordWrap_IsWrapper(copy.buffer[lineEnd]) 
			&& nextStart < totalChars && !WordWrap_IsWrapper(copy.buffer[nextStart]);
		Int32 wrappedLen = needWrap ? WordWrap_WrapLine(&copy, index, maxPerLine) : maxPerLine;

		/* Calculate the maximum size of this line */
		Int32 lineLen = maxPerLine;
		for (Int32 i = lineEnd; i >= index; i--) {
			if (copy.buffer[i] != NULL) break;
			lineLen--;
		}
		lineLens[index / maxPerLine] = min(lineLen, wrappedLen);
	}

	/* Output the used lines */
	WordWrap_OutputLines(&copy, lines, lineLens, numLines, usedLines, maxPerLine);
}

/* Calculates where the given raw index is located in the wrapped lines. */
void WordWrap_GetCoords(Int32 index, STRING_PURE String* lines, Int32 numLines, Int32* coordX, Int32* coordY) {
	if (index == -1) index = Int32_MaxValue;
	Int32 offset = 0; *coordX = -1; *coordY = 0;

	for (Int32 y = 0; y < numLines; y++) {
		Int32 lineLength = lines[y].length;
		if (lineLength == 0) break;

		*coordY = y;
		if (index < offset + lineLength) {
			*coordX = index - offset; break;
		}
		offset += lineLength;
	}
	if (*coordX == -1) *coordX = lines[*coordY].length;
}

Int32 WordWrap_GetBackLength(STRING_PURE String* text, Int32 index) {
	if (index <= 0) return 0;
	if (index >= text->length) {
		ErrorHandler_Fail("WordWrap_GetBackLength - index past end of string");
	}

	Int32 start = index;
	bool lookingSpace = text->buffer[index] == ' ';
	/* go back to the end of the previous word */
	if (lookingSpace) {
		while (index > 0 && text->buffer[index] == ' ') { index--; }
	}

	/* go back to the start of the current word */
	while (index > 0 && text->buffer[index] != ' ') { index--; }
	return start - index;
}

Int32 WordWrap_GetForwardLength(STRING_PURE String* text, Int32 index) {
	if (index == -1) return 0;
	if (index >= text->length) {
		ErrorHandler_Fail("WordWrap_GetForwardLength - index past end of string");
	}

	Int32 start = index, length = text->length;
	bool lookingLetter = text->buffer[index] != ' ';
	/* go forward to the end of the current word */
	if (lookingLetter) {
		while (index < length && text->buffer[index] != ' ') { index++; }
	}

	/* go forward to the start of the next word */
	while (index < length && text->buffer[index] == ' ') { index++; }
	return index - start;
}
