// LData: a block of bytes in a CFData. An immutable CFData (from a no-copy
// initialiser) is swapped for a mutable copy as soon as the length changes.
#pragma once

#include "LString.h"

namespace LF {

class LData : public LObject {
	LF_DECLARE_CLASS(LData)
public:
	LData() : _cfData(0), _cfMutableData(0) {}
	virtual ~LData() {}
	virtual LData *init();
	virtual LData *copy() const;
	virtual LObject *initWithDecoder(const LCoder *coder);
	virtual void encodeWithCoder(LCoder *coder) const;
	virtual LString *description() const;
	virtual LString *internalDescription() const;
	// "<" and base64-like text (see LData.cpp) and ">".
	virtual LString *fillTextRepresentation(unsigned int indent) const;
	virtual void dealloc();
	virtual LString *_plistRepresentation(unsigned int indent) const;
	virtual LData *getOwnerLObject() { return this; }
	virtual bool writeToFile(const LString *path);
	virtual void setData(const void *bytes, unsigned int length);
	virtual void appendBytes(const void *bytes, unsigned int length);

	// Factories (autoreleased)
	static LData *dataWithBytes(const void *bytes, unsigned int length);
	static LData *dataWithBytesNoCopy(void *bytes, unsigned int length, bool freeWhenDone);
	static LData *dataWithLength(unsigned int length);
	static LData *dataWithContentsOfFile(const LString *path);
	static LData *dataWithResource(LString *name, LString *type, CFBundleRef bundle = 0);

	LData *initWithBytesNoCopy(void *bytes, unsigned int length, bool freeWhenDone);
	LData *initWithContentsOfFile(const LString *path);
	LData *initWithResource(LString *name, LString *type, CFBundleRef bundle = 0);
	LData *initWithCFData(CFDataRef data);
	LData *initWithCFDataNoCopy(CFDataRef data);
	LData *initWithCFMutableDataNoCopy(CFMutableDataRef data);
	LData *initWithTextRepresentation(const LData *text);
	LData *initWithBase64Data(LData *base64);

	void *bytes() const
	{
		CFDataRef d = _cf();
		return d ? (void *)CFDataGetBytePtr(d) : 0;
	}
	unsigned int length() const
	{
		CFDataRef d = _cf();
		return d ? (unsigned int)CFDataGetLength(d) : 0;
	}
	void setLength(unsigned int length);
	CFDataRef cfData() const { return _cf(); }

	// zlib: compress() appends the original length (big-endian) and 'zdat'.
	void compress();
	void expand();
	LData *detachDecomressedData() const;
	// The XML or old-style property list in the data (autoreleased).
	LObject *propertyList();

	CFDataRef _cf() const { return _cfMutableData ? (CFDataRef)_cfMutableData : _cfData; }

	CFDataRef _cfData;
	CFMutableDataRef _cfMutableData;
};

} // namespace LF
