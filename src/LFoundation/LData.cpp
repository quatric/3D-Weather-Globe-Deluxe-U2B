// LData
//
// LF::LData: bytes in a CFData. _cfData holds an immutable CFData (the
// no-copy initialisers); the first change of length copies it into
// _cfMutableData. Text forms: fillTextRepresentation's "<...>" packs each 3
// bytes into 4 characters of sBinToTextRemap (low bits first), after an
// 8-character length; the plist form is standard base64.

#include "LData.h"
#include "LArray.h"
#include "LCoder.h"
#include "LFileManager.h"
#include "LMap.h"
#include "LXMLTree.h"
#include "CLMapCreationStream.h"
#include "UTextUtils.h"
#include <zlib.h>

namespace LF {

LF_IMPLEMENT_CLASS(LData, 0)

static const char sBinToTextRemap[65] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789";

// Four characters to three bytes.
static void _remapTextToBin(const unsigned char *text, unsigned char *out)
{
	static unsigned char *sTextToBin = 0;
	if (sTextToBin == 0) {
		unsigned char *t = new unsigned char[256];
		memset(t, 0, 256);
		for (int i = 0; i < 64; i++)
			t[(unsigned char)sBinToTextRemap[i]] = (unsigned char)i;
		sTextToBin = t;
	}
	unsigned int v = sTextToBin[text[0]] + sTextToBin[text[1]] * 0x40 + sTextToBin[text[2]] * 0x1000
	                 + sTextToBin[text[3]] * 0x40000;
	out[0] = (unsigned char)v;
	out[1] = (unsigned char)(v >> 8);
	out[2] = (unsigned char)(v >> 16);
}

// Three bytes (as a little-endian 24-bit value) to four characters.
static char *_remapBinToText(unsigned int v, char *out)
{
	out[0] = sBinToTextRemap[v & 0x3f];
	out[1] = sBinToTextRemap[(v >> 6) & 0x3f];
	out[2] = sBinToTextRemap[(v >> 12) & 0x3f];
	out[3] = sBinToTextRemap[(v >> 18) & 0x3f];
	return out + 4;
}

// For no-copy data that is freed when done: CF frees the bytes with free().
static void *_lCFDataAllocatorAllocateCallBack(CFIndex, CFOptionFlags, void *)
{
	return 0;
}

static void _lCFDataAllocatorDeallocateCallBack(void *ptr, void *)
{
	free(ptr);
}

LData *LData::dataWithBytes(const void *bytes, unsigned int length)
{
	LData *d = _alloc();
	if (!d)
		return 0;
	d->setData(bytes, length);
	return LAutorelease(d);
}

LData *LData::dataWithContentsOfFile(const LString *path)
{
	LData *d = _alloc();
	if (d && (d = d->initWithContentsOfFile(path)) != 0)
		return LAutorelease(d);
	return 0;
}

LData *LData::dataWithBytesNoCopy(void *bytes, unsigned int length, bool freeWhenDone)
{
	LData *d = _alloc();
	if (d && (d = d->initWithBytesNoCopy(bytes, length, freeWhenDone)) != 0)
		return LAutorelease(d);
	return 0;
}

LData *LData::dataWithLength(unsigned int length)
{
	LData *d = _alloc();
	if (!d)
		return 0;
	d->setLength(length);
	return LAutorelease(d);
}

LData *LData::dataWithResource(LString *name, LString *type, CFBundleRef bundle)
{
	LData *d = _alloc();
	if (d && (d = d->initWithResource(name, type, bundle)) != 0)
		return LAutorelease(d);
	return 0;
}

void LData::dealloc()
{
	if (_cfData)
		CFRelease(_cfData);
	if (_cfMutableData)
		CFRelease(_cfMutableData);
	LObject::dealloc();
}

LData *LData::init()
{
	if (LObject::init() == 0)
		return 0;
	setLength(0);
	return this;
}

LData *LData::initWithBytesNoCopy(void *bytes, unsigned int length, bool freeWhenDone)
{
	if (bytes == 0) {
		dealloc();
		return 0;
	}
	if (LObject::init() == 0)
		return 0;
	static CFAllocatorRef sAllocator = [] {
		CFAllocatorContext ctx;
		memset(&ctx, 0, sizeof ctx);
		ctx.allocate = _lCFDataAllocatorAllocateCallBack;
		ctx.deallocate = _lCFDataAllocatorDeallocateCallBack;
		return CFAllocatorCreate(kCFAllocatorDefault, &ctx);
	}();
	_cfData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8 *)bytes, length,
	                                      freeWhenDone ? sAllocator : kCFAllocatorNull);
	return this;
}

// Through the shared manager, or a temporary one if there is none.
LData *LData::initWithContentsOfFile(const LString *path)
{
	if (LObject::init() == 0)
		return 0;
	LFileManager *manager = LFileManager::manager();
	bool ok;
	if (manager == 0) {
		LFileSystemFileManager temp(0);
		ok = temp.loadFileIntoData(path, this);
	} else
		ok = manager->loadFileIntoData(path, this);
	if (ok)
		return this;
	dealloc();
	return 0;
}

LData *LData::initWithCFData(CFDataRef data)
{
	if (LObject::init() == 0)
		return 0;
	_cfMutableData = data ? CFDataCreateMutableCopy(kCFAllocatorDefault, 0, data)
	                      : CFDataCreateMutable(kCFAllocatorDefault, 0);
	return this;
}

LData *LData::initWithCFMutableDataNoCopy(CFMutableDataRef data)
{
	if (LObject::init() == 0)
		return 0;
	if (data == 0)
		setLength(0);
	else {
		_cfMutableData = data;
		CFRetain(data);
	}
	return this;
}

LData *LData::initWithCFDataNoCopy(CFDataRef data)
{
	if (LObject::init() == 0)
		return 0;
	if (data == 0)
		setLength(0);
	else {
		_cfData = data;
		CFRetain(data);
	}
	return this;
}

LData *LData::initWithResource(LString *name, LString *type, CFBundleRef bundle)
{
	if (bundle == 0)
		bundle = CFBundleGetMainBundle();
	if (bundle) {
		CFURLRef url = CFBundleCopyResourceURL(bundle, name->cfString(), type ? type->cfString() : 0, 0);
		if (url) {
			CFStringRef path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
			LString *str = 0;
			if (path) {
				str = LString::stringWithCFString(path);
				CFRelease(path);
			}
			CFRelease(url);
			if (str)
				return initWithContentsOfFile(str);
		}
	}
	dealloc();
	return 0;
}

LString *LData::internalDescription() const
{
	return description();
}

// Moving from an immutable CFData keeps up to `length` bytes of it.
void LData::setLength(unsigned int length)
{
	if (_cfData) {
		_cfMutableData = CFDataCreateMutable(kCFAllocatorDefault, 0);
		if (length) {
			unsigned int old = (unsigned int)CFDataGetLength(_cfData);
			if (old)
				CFDataAppendBytes(_cfMutableData, CFDataGetBytePtr(_cfData), old < length ? old : length);
			if (old < length)
				CFDataIncreaseLength(_cfMutableData, length - old);
		}
		CFRelease(_cfData);
		_cfData = 0;
		return;
	}
	if (_cfMutableData == 0)
		_cfMutableData = CFDataCreateMutable(kCFAllocatorDefault, 0);
	CFDataSetLength(_cfMutableData, length);
}

LObject *LData::initWithDecoder(const LCoder *coder)
{
	LCoder *c = const_cast<LCoder *>(coder);
	StReadChunk chunk(c);
	if (c->nextBlockSize() != 4)
		LF_RAISE(WrongConditionException());
	uint32_t n;
	c->readBytes(&n);
	if (c->nextBlockSize() != n)
		LF_RAISE(WrongConditionException());
	setLength(n);
	c->readBytes(bytes());
	return this;
}

void LData::encodeWithCoder(LCoder *coder) const
{
	StWriteChunk chunk(coder);
	uint32_t n = length();
	coder->storeBytes(&n, 4);
	coder->storeBytes(bytes(), n);
}

// Reads fillTextRepresentation's form; the first character (the '<') and
// any control characters and spaces after it are skipped.
LData *LData::initWithTextRepresentation(const LData *text)
{
	const unsigned char *p = (const unsigned char *)text->bytes();
	do
		p++;
	while (*p < 0x21);
	unsigned char header[8] = { 0 };
	_remapTextToBin(p, header);
	_remapTextToBin(p + 4, header + 3);
	uint32_t n = header[0] | header[1] << 8 | header[2] << 16 | (uint32_t)header[3] << 24;
	setLength(n);
	unsigned int rest = n % 3;
	unsigned char *out = (unsigned char *)bytes();
	const unsigned char *in = p + 8;
	for (unsigned int i = 0; i < n / 3; i++) {
		_remapTextToBin(in, out);
		in += 4;
		out += 3;
	}
	if (rest) {
		unsigned char tail[8] = { 0 };
		_remapTextToBin(in, tail);
		memcpy(out, tail, rest);
	}
	return this;
}

void LData::setData(const void *src, unsigned int n)
{
	setLength(n);
	memcpy(bytes(), src, n);
}

LData *LData::initWithBase64Data(LData *base64)
{
	if (LObject::init() == 0)
		return 0;
	unsigned int n = 0;
	if (base64 && base64->length() != 0) {
		unsigned int inLength = base64->length();
		const char *in = (const char *)base64->bytes();
		setLength(((int)(inLength * 3) >> 2) + 4);
		n = UU_decode(in, inLength, (unsigned char *)bytes(), length());
	}
	setLength(n);
	return this;
}

LData *LData::copy() const
{
	unsigned int n = length();
	const void *src = bytes();
	LData *d = _alloc();
	d->setData(src, n);
	return d;
}

LString *LData::description() const
{
	return LString::stringWithFormat("LData(%p); length = %d", this, length());
}

LString *LData::fillTextRepresentation(unsigned int) const
{
	unsigned int n = length();
	unsigned int rest = n % 3;
	unsigned int textLength = (n / 3 + (rest != 0)) * 4;
	StPointerBlock block(textLength + 11, false);
	char *out = (char *)block.bytes();
	const unsigned char *in = (const unsigned char *)bytes();
	*out++ = '<';
	out = _remapBinToText(n & 0xffffff, out);
	out = _remapBinToText(n >> 24, out);
	for (unsigned int i = 0; i < n / 3; i++, in += 3)
		out = _remapBinToText(in[0] | in[1] << 8 | in[2] << 16, out);
	if (rest) {
		unsigned char tail[4] = { 0, 0, 0, 0 };
		memcpy(tail, in, rest);
		out = _remapBinToText(tail[0] | tail[1] << 8 | tail[2] << 16, out);
	}
	*out = '>';
	return LString::stringWithCString((const char *)block.bytes(), textLength + 10, kLMacRomanEncoding);
}

// Base64 in lines of 76 characters, each indented by `indent` tabs.
LString *LData::_plistRepresentation(unsigned int indent) const
{
	unsigned int n = length();
	StPointerBlock encoded((n / 3) * 4 + 6, false);
	int left = UU_encode((const unsigned char *)bytes(), n, (char *)encoded.bytes());
	int lines = (left - 1) / 76 + 1;
	StPointerBlock text((indent + 1) * lines + 10 + left, false);
	const char *src = (const char *)encoded.bytes();
	char *out = (char *)text.bytes();
	for (int i = 0; i < lines; i++) {
		memset(out, '\t', indent);
		out += indent;
		int len = left < 77 ? left : 76;
		memmove(out, src, len);
		left -= len;
		src += len;
		out[len] = '\n';
		out += len + 1;
		if (left < 1)
			break;
	}
	memset(out, '\t', indent);
	out[indent] = 0;
	return LString::stringWithFormat("<data>\n%s</data>", (const char *)text.bytes());
}

// Through the shared manager, or a temporary one if there is none (the
// original required a manager).
bool LData::writeToFile(const LString *path)
{
	const void *src = bytes();
	if (src == 0)
		return false;
	unsigned int n = length();
	LFileManager *manager = LFileManager::manager();
	LFileSystemFileManager *temp = 0;
	if (manager == 0)
		manager = temp = new LFileSystemFileManager(0);
	bool ok = false;
	LStreamID stream = manager->openWriteStreamForFile(path);
	if (stream) {
		bool written = manager->appendDataToStream(stream, src, n) != 0;
		bool closed = manager->closeStreamWithID(stream);
		ok = written && closed;
	}
	delete temp;
	return ok;
}

void LData::appendBytes(const void *src, unsigned int n)
{
	if (_cfData)
		setLength(length());
	if (_cfMutableData == 0)
		_cfMutableData = CFDataCreateMutable(kCFAllocatorDefault, 0);
	CFDataAppendBytes(_cfMutableData, (const UInt8 *)src, n);
}

static const char kZTrailer[4] = { 'z', 'd', 'a', 't' };

void LData::compress()
{
	unsigned int n = length();
	float bound = (float)n * 1.001f + 20.0f;
	uLongf destLength = bound <= 0 ? 0 : (uLongf)bound;
	CFMutableDataRef dest = CFDataCreateMutable(kCFAllocatorDefault, 0);
	CFDataSetLength(dest, destLength);
	if (::compress(CFDataGetMutableBytePtr(dest), &destLength, (const Bytef *)bytes(), length()) != Z_OK) {
		CFRelease(dest);
		LF_RAISE(WrongConditionException());
		return;
	}
	CFDataSetLength(dest, destLength + 8);
	UInt8 *p = CFDataGetMutableBytePtr(dest) + destLength;
	uint32_t big = CFSwapInt32HostToBig(length());
	memcpy(p, &big, 4);
	memcpy(p + 4, kZTrailer, 4);
	if (_cfData) {
		CFRelease(_cfData);
		_cfData = 0;
	}
	if (_cfMutableData)
		CFRelease(_cfMutableData);
	_cfMutableData = dest;
}

// The uncompressed bytes of compress()'s form, in a new CFData, or 0.
static CFMutableDataRef _expand(const LData *data)
{
	const UInt8 *src = (const UInt8 *)data->bytes();
	unsigned int n = data->length();
	if (n < 8 || memcmp(src + n - 4, kZTrailer, 4) != 0)
		return 0;
	uint32_t big;
	memcpy(&big, src + n - 8, 4);
	uLongf size = CFSwapInt32BigToHost(big);
	CFMutableDataRef dest = CFDataCreateMutable(kCFAllocatorDefault, 0);
	CFDataSetLength(dest, size);
	if (uncompress(CFDataGetMutableBytePtr(dest), &size, src, n) != Z_OK) {
		CFRelease(dest);
		return 0;
	}
	return dest;
}

void LData::expand()
{
	CFMutableDataRef dest = _expand(this);
	if (dest == 0)
		return;
	if (_cfData) {
		CFRelease(_cfData);
		_cfData = 0;
	}
	if (_cfMutableData)
		CFRelease(_cfMutableData);
	_cfMutableData = dest;
}

LData *LData::detachDecomressedData() const
{
	CFMutableDataRef dest = _expand(this);
	if (dest == 0)
		return 0;
	LData *d = LAutorelease(_alloc());
	d->_cfMutableData = dest;
	return d;
}

// XML first; then whichever of a "(" list or a "{" map comes first.
LObject *LData::propertyList()
{
	LObject *obj = LXMLTree::objectFromXMLData((const char *)bytes(), length(), 0);
	if (obj)
		return obj;
	CLMapCreationStream stream(this);
	unsigned int list = stream.GetBlockOffsetStart('(', ')');
	unsigned int map = stream.GetBlockOffsetStart('{', '}');
	if (map == (unsigned int)-1 || (list != (unsigned int)-1 && list <= map)) {
		if (list == (unsigned int)-1 || (map != (unsigned int)-1 && map <= list))
			return 0;
		obj = LArray::_alloc()->initWithTextRepresentation(this);
		if (obj == 0)
			obj = LMap::_alloc()->initWithTextRepresentation(this);
	} else {
		obj = LMap::_alloc()->initWithTextRepresentation(this);
		if (obj == 0)
			obj = LArray::_alloc()->initWithTextRepresentation(this);
	}
	return obj ? LAutorelease(obj) : 0;
}

} // namespace LF
