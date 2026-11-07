// UTextUtils
//
// LF::UTextUtils: line break detection, scanning past quoted text and
// comments, comment removal and path conversion; plus base64.

#include "UTextUtils.h"
#include "LData.h"

namespace LF {
namespace UTextUtils {

// A zero length is treated as empty; the original's `length - 1` bound
// wrapped around for it.
template <typename T> static int _lineBreakStyle(const T *text, unsigned int length)
{
	int cr = 0, crlf = 0, lf = 0;
	if (length > 10000)
		length = 10000;
	for (unsigned int i = 0; i + 1 < length; i++) {
		if (text[i] == '\r') {
			if (text[i + 1] == '\n') {
				crlf++;
				i++;
			} else
				cr++;
		} else if (text[i] == '\n')
			lf++;
	}
	if (crlf != 0 && cr == 0 && lf == 0)
		return kCRLFLineBreaks;
	if (lf != 0 && cr == 0 && crlf == 0)
		return kLFLineBreaks;
	if (cr != 0 && lf == 0 && crlf == 0)
		return kCRLineBreaks;
	return kUnknownLineBreaks;
}

int lineBreakStyleForBytes(const char *bytes, unsigned int length)
{
	return _lineBreakStyle(bytes, length);
}

int lineBreakStyleForBytes(const UniChar *chars, unsigned int length)
{
	return _lineBreakStyle(chars, length);
}

// The original passed the byte length for 2-byte text too.
int lineBreakStyleForData(LData *data, bool wide)
{
	if (data == 0)
		return kUnknownLineBreaks;
	unsigned int length = data->length();
	if (wide)
		return _lineBreakStyle((const UniChar *)data->bytes(), length);
	return _lineBreakStyle((const char *)data->bytes(), length);
}

// As in the original, a mismatch does not restart the match.
unsigned int scanStringToString(const char *text, unsigned int length, const char *target, bool skipQuoted)
{
	if (target == 0 || *target == 0)
		return 0;
	unsigned int i = 0;
	int matched = 0;
	for (;;) {
		if (i >= length)
			return i <= length ? i : length;
		char c;
		for (;;) {
			c = text[i++];
			if (c == '"' || c == '\'' || c == '/')
				break;
			matched += target[matched] == c;
			if (target[matched] == 0)
				return i - matched;
			if (i == length)
				return i;
		}
		if (i >= length)
			return i <= length ? i : length;
		if (!skipQuoted)
			continue;
		if (c == '\'' || c == '"') {
			while (i != length && text[i] != c)
				i++;
			i++;
		} else if (text[i] == '/') {
			while (i != length && text[i] != '\n' && text[i] != '\r')
				i++;
		} else if (text[i] == '*') {
			while (i < length - 1 && text[i] != '*' && text[i + 1] != '/')
				i++;
		}
	}
}

unsigned int scanStringToEOL(const char *text, unsigned int length)
{
	unsigned int i = 0;
	for (;; i++, text++) {
		if (i >= length)
			return i;
		if (*text == '\r' || *text == '\n')
			break;
	}
	do {
		text++;
		i++;
		if (*text != '\r' && *text != '\n')
			return i;
	} while (i < length);
	return i;
}

// Compacts the text in place, dropping comments outside quoted strings
// (where a backslash escapes the quote), and returns the new length.
template <typename T> static unsigned int _compactTextByRemovingComments(T *text, unsigned int length)
{
	unsigned int out = 0;	// end of the kept text
	unsigned int keep = 0;	// start of the text not yet kept
	unsigned int after = 1;	// where the last comment ended + 1
	unsigned int i = 0, next = 0;
	// Moves the text from `keep` up to the comment at `at` down to `out`.
	auto flush = [&](unsigned int at) {
		if (after < at) {
			unsigned int n = at - keep - 1;
			if (out < keep)
				memmove(text + out, text + keep, n * sizeof(T));
			out += n;
		}
	};
	do {
		for (;;) {
			next = i;
			if (i >= length)
				goto done;
			T c = text[i];
			if (c == '"' || c == '/' || c == '\'')
				break;
			i++;
		}
		{
			T c = text[i];
			bool comment = c == '/';
			if (c == '\'') {
				bool first = true;
				for (T *p = text + i;; p++) {
					if (*p == '\'' && (first || p[-1] != '\\'))
						break;
					i++;
					first = false;
					if (i == length)
						break;
				}
				comment = true;	// the original went on to look for a comment after the quote
			}
			if (comment) {
				unsigned int j = i + 1;
				next = i + 2;
				if (j < length) {
					if (text[j] == '/') {
						flush(j);
						while (j != length && text[j] != '\n' && text[j] != '\r')
							j++;
						if (j < length && text[j] == '\r' && j + 1 < length && text[j + 1] == '\n')
							j++;
						next = j + 1;
						if (j < length) {
							after = j + 2;
							keep = next;
						}
					} else if (text[j] == '*') {
						flush(j);
						while (j < length - 1 && text[j] != '*' && text[j + 1] != '/')
							j++;
						next = j + 1;
						if (j < length) {
							after = j + 2;
							keep = next;
						}
					}
				}
			} else {
				next = i + 1;
				if (c == '"') {
					bool first = true;
					for (T *p = text + i;; p++) {
						if (*p == '"' && (first || p[-1] != '\\'))
							break;
						i++;
						first = false;
						if (i == length)
							break;
					}
					next = i + 1;
				}
			}
		}
		i = next;
	} while (next < length);
done:
	flush(next);
	return out;
}

bool removeComments(LData *data, bool wide)
{
	unsigned int n;
	if (!wide)
		n = _compactTextByRemovingComments((char *)data->bytes(), data->length());
	else
		n = _compactTextByRemovingComments((wchar_t *)data->bytes(), data->length() / sizeof(wchar_t)) * sizeof(wchar_t);
	unsigned int old = data->length();
	if (n != old)
		data->setLength(n);
	return n != old;
}

LString *convertPathToNativePath(const LString *path)
{
	if (path->isAbsolutePath())
		return LString::stringWithString(path);
	const UniChar *chars = path->chars();
	unsigned int n = path->length();
	UniChar *buf = new UniChar[n + 1];
	for (unsigned int i = 0; i < n; i++)
		buf[i] = chars[i] == '\\' ? '/' : chars[i];
	buf[n] = 0;
	LString *result = LString::stringWithCharacters(buf, n);
	delete[] buf;
	return result;
}

} // namespace UTextUtils
} // namespace LF

static const char sSix2pr[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static unsigned char sPr2six[256];

int UU_encode(const unsigned char *in, unsigned int length, char *out)
{
	char *p = out;
	unsigned int i;
	for (i = 0; i < length; i += 3) {
		const unsigned char *b = in + i;
		// The original read up to two bytes past the end for the last group;
		// they count as zeros here.
		unsigned char b1 = i + 1 < length ? b[1] : 0, b2 = i + 2 < length ? b[2] : 0;
		p[0] = sSix2pr[b[0] >> 2];
		p[1] = sSix2pr[(b[0] & 3) << 4 | b1 >> 4];
		p[2] = sSix2pr[(b1 & 0xf) << 2 | b2 >> 6];
		p[3] = sSix2pr[b2 & 0x3f];
		p += 4;
	}
	if (length + 1 == i)
		p[-1] = '=';
	else if (length + 2 == i) {
		p[-1] = '=';
		p[-2] = '=';
	}
	*p = 0;
	return (int)(p - out);
}

unsigned int UU_decode(const char *in, unsigned int length, unsigned char *out, unsigned int outSize)
{
	static bool first = true;
	if (first) {
		first = false;
		memset(sPr2six, 0x40, sizeof sPr2six);
		for (int i = 0; i < 64; i++)
			sPr2six[(unsigned char)sSix2pr[i]] = (unsigned char)i;
	}
	unsigned int left = 0;
	for (unsigned int i = 0; i < length; i++)
		left += sPr2six[(unsigned char)in[i]] < 0x40;
	unsigned int total = ((left + 3) >> 2) * 3;
	if (total > outSize)
		return 0;
	unsigned char q[4] = { 0, 0, 0, 0 };
	unsigned char *p = out;
	int k = 0;
	for (unsigned int i = 0; left != 0 && i < length; i++) {
		unsigned char v = sPr2six[(unsigned char)in[i]];
		if (v < 0x40) {
			q[k++] = v;
			if (k == 4) {
				left -= 4;
				p[0] = q[1] >> 4 | q[0] << 2;
				p[1] = q[1] << 4 | q[2] >> 2;
				p[2] = q[2] << 6 | q[3];
				p += 3;
				k = 0;
			}
		}
	}
	if (k == 3) {
		p[0] = q[1] >> 4 | q[0] << 2;
		p[1] = q[1] << 4 | q[2] >> 2;
	} else if (k == 2)
		p[0] = q[1] >> 4 | q[0] << 2;
	if (left & 3)
		total -= q[2] > 0x3f ? 2 : 1;
	return total;
}
