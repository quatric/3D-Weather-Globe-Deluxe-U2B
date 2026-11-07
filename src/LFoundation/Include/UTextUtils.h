// UTextUtils: helpers for scanning text, and base64 (UU_encode/UU_decode).
#pragma once

#include "LString.h"

namespace LF {

class LData;

namespace UTextUtils {

enum LineBreakStyle {
	kUnknownLineBreaks = -1,	// none, or a mix
	kCRLFLineBreaks = 0,
	kLFLineBreaks = 1,
	kCRLineBreaks = 2,
};

// Looks at the first 10000 characters.
int lineBreakStyleForBytes(const char *bytes, unsigned int length);
int lineBreakStyleForBytes(const UniChar *chars, unsigned int length);
// `wide`: the data holds 4-byte wchar_t text.
int lineBreakStyleForData(LData *data, bool wide);
// The offset of the first match of `target`, optionally skipping quoted
// text and comments.
unsigned int scanStringToString(const char *text, unsigned int length, const char *target, bool skipQuoted);
// The offset just past the end of the line and its line breaks.
unsigned int scanStringToEOL(const char *text, unsigned int length);
// Strips // and /* */ comments outside quotes; true if anything changed.
bool removeComments(LData *data, bool wide);
// Backslashes become slashes in a relative path.
LString *convertPathToNativePath(const LString *path);

} // namespace UTextUtils
} // namespace LF

// Base64. UU_encode writes a terminated string and returns its length;
// UU_decode returns the number of bytes, or 0 if `outSize` is too small.
int UU_encode(const unsigned char *in, unsigned int length, char *out);
unsigned int UU_decode(const char *in, unsigned int length, unsigned char *out, unsigned int outSize);
