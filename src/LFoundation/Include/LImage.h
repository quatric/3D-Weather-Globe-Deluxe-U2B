// LImage: a CGImage (optionally owned) with a cached NSImage. LBitmapImage
// adds pixels in memory: BGRA, BGRX or BGR rows (TPixelFormat), reached
// through an StLocker; a locker opened for writing makes a new CGImage of
// the pixels when it goes. Formats other than JPEG and PNG, and encoding,
// are left to the application's ILImageCallback.
#pragma once

#include "LData.h"
#include <CoreGraphics/CoreGraphics.h>
#include <objc/objc.h>

namespace LF {

enum TPixelFormat {
	kPixelFormatBGRA = 0,
	kPixelFormatBGRX,	// 32 bits, no alpha
	kPixelFormatBGR	// 24 bits
};

enum TBitmapRepresentationType {
	kPNGRepresentation = 0,
	kJPEGRepresentation,
	kTIFFRepresentation
};

enum TImageFileFormat {
	kImageFilePNG = 0,
	kImageFileJPEG
};

class LBitmapImage;

class ILImageCallback {
public:
	virtual CGImageRef _CGImageFromImageData(LData *data) = 0;
	virtual LData *_representationOfTypeForBitmapImage(TBitmapRepresentationType type, LBitmapImage *image) = 0;

	static void setLImageCallback(ILImageCallback *callback) { sCallback = callback; }
	static ILImageCallback *sCallback;
};

class LImage : public LObject {
	LF_DECLARE_CLASS(LImage)
public:
	LImage() : _image(0), _nsImage(0), _ownsImage(false) {}
	virtual ~LImage() {}
	virtual void dealloc();
	virtual LImage *getOwnerLObject() { return this; }
	virtual LImage *initWithContentsOfFile(const LString *path);
	virtual LImage *initWithBytes(void *bytes, unsigned int length);
	virtual LImage *initWithData(LData *data);
	// Fails (and deallocates) without an image; owns it if asked.
	virtual LImage *initWithCGImage(CGImageRef image, unsigned char owns);
	virtual void setCGImage(CGImageRef image, unsigned char owns);

	// A JPEG or PNG image, or 0.
	static CGImageRef CGImageFromImageData(LData *data);

	CGImageRef CGImage() const { return _image; }
	unsigned int width() const { return _image ? (unsigned int)CGImageGetWidth(_image) : 0; }
	unsigned int height() const { return _image ? (unsigned int)CGImageGetHeight(_image) : 0; }
	LSize size() const;
	// Not supported here.
	bool writeToFile(LString *, TImageFileFormat, float) { return false; }

	id NSImage();
	void setNSImage(id image);
	void _invalidateNSImage();

	CGImageRef _image;
	id _nsImage;
	bool _ownsImage;
};

class LBitmapImage : public LImage {
	LF_DECLARE_CLASS(LBitmapImage)
public:
	// The pixels of a locked image.
	struct SBitmapInfo {
		unsigned char *cursor;	// the data provider's read position
		LData *data;
		unsigned int bytesPerRow;
		unsigned int bitsPerPixel;
	};

	class StLocker {
	public:
		enum {
			kRead = 1,	// start with the image's pixels
			kWrite = 2	// make the image from the pixels at the end
		};
		StLocker(LBitmapImage *image, unsigned int flags, TPixelFormat format);
		~StLocker();
		unsigned int bitsPerPixel() const { return _info->bitsPerPixel; }
		void *bitmapData() const { return _info->data->bytes(); }
		unsigned int bytesPerRow() const { return _info->bytesPerRow; }

		LBitmapImage *_image;
		SBitmapInfo *_info;
		unsigned int _flags;
		TPixelFormat _format;
	};

	LBitmapImage() : _format(kPixelFormatBGRA), _bitmap(0) {}
	virtual ~LBitmapImage() {}
	virtual void dealloc();
	virtual LBitmapImage *getOwnerLObject() { return this; }
	virtual LBitmapImage *initWithContentsOfFile(const LString *path);
	virtual LBitmapImage *initWithBytes(void *bytes, unsigned int length);
	virtual LBitmapImage *initWithData(LData *data);
	virtual LBitmapImage *initWithCGImage(CGImageRef image, unsigned char owns);
	virtual void setCGImage(CGImageRef image, unsigned char owns);
	virtual LBitmapImage *initWithSizeFormat(unsigned int width, unsigned int height, TPixelFormat format);

	static void CGImageFormatFromFormat(TPixelFormat format, size_t *bitsPerComponent, size_t *bitsPerPixel,
	    CGImageAlphaInfo *alphaInfo);
	// New (retained) images.
	LBitmapImage *cloneWithPixelFormat(TPixelFormat format);
	// options: 1 keeps the aspect ratio, 2 allows enlarging. The image is
	// centered in the new size.
	LBitmapImage *scaledCloneWithSize(LSize size, unsigned int options);
	// Encoded by the application's callback, or 0.
	LData *representationOfType(TBitmapRepresentationType type);

	static void _convertBitmapToFormat(const unsigned char *src, unsigned char *dst, TPixelFormat srcFormat,
	    TPixelFormat dstFormat, int pixels);

	TPixelFormat _format;	// of _bitmap
	LData *_bitmap;
};

} // namespace LF
