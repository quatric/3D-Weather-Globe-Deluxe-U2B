// LImage
//
// LF::LImage, LF::LBitmapImage, its StLocker and the data provider that
// gives Core Graphics the locked pixels.

#import <Foundation/Foundation.h>
#include "LImage.h"
#include <Accelerate/Accelerate.h>
#include <stdlib.h>

namespace LF {

LF_IMPLEMENT_CLASS(LImage, 0)
LF_IMPLEMENT_CLASS(LBitmapImage, LF_CLASSOF(LImage))

ILImageCallback *ILImageCallback::sCallback = 0;

// The data provider of a locked bitmap: reads BGR[A] pixels as RGB[A].

static size_t _dataProviderGetBytes(void *info, void *buffer, size_t count)
{
	LBitmapImage::SBitmapInfo *bitmap = (LBitmapImage::SBitmapInfo *)info;
	if (bitmap == 0)
		return count;
	unsigned int pixelSize = bitmap->bitsPerPixel >> 3;
	unsigned char *dst = (unsigned char *)buffer;
	for (int i = 0; i < (int)(count / pixelSize); i++) {
		dst[0] = bitmap->cursor[2];
		dst[1] = bitmap->cursor[1];
		dst[2] = bitmap->cursor[0];
		dst += 3;
		if (pixelSize == 4)
			*dst++ = bitmap->cursor[3];
		bitmap->cursor += pixelSize;
	}
	return count;
}

static off_t _dataProviderSkipBytes(void *info, off_t count)
{
	if (info)
		((LBitmapImage::SBitmapInfo *)info)->cursor += count;
	return count;
}

static void _dataProviderRewind(void *info)
{
	if (info) {
		LBitmapImage::SBitmapInfo *bitmap = (LBitmapImage::SBitmapInfo *)info;
		bitmap->cursor = (unsigned char *)bitmap->data->bytes();
	}
}

static void _dataProviderReleaseInfo(void *info)
{
	if (info == 0)
		return;
	LBitmapImage::SBitmapInfo *bitmap = (LBitmapImage::SBitmapInfo *)info;
	if (bitmap->data) {
		bitmap->data->release();
		bitmap->data = 0;
	}
	free(bitmap);
}

static const CGDataProviderSequentialCallbacks _dataProviderCallbacks = {
	0, _dataProviderGetBytes, _dataProviderSkipBytes, _dataProviderRewind, _dataProviderReleaseInfo
};

// Releases the LData behind an image read from data.
static void _dataProviderReleaseData(void *info, const void *, size_t)
{
	if (info)
		((LObject *)info)->release();
}

// LImage

void LImage::dealloc()
{
	if (_image) {
		if (_ownsImage)
			CGImageRelease(_image);
		_image = 0;
	}
	_invalidateNSImage();
	LObject::dealloc();
}

LImage *LImage::initWithCGImage(CGImageRef image, unsigned char owns)
{
	if (image == 0) {
		dealloc();
		return 0;
	}
	if (LObject::init() == 0)
		return 0;
	setCGImage(image, owns);
	return this;
}

void LImage::setCGImage(CGImageRef image, unsigned char owns)
{
	if (_image == image)
		return;
	if (_image && _ownsImage)
		CGImageRelease(_image);
	_image = image;
	_ownsImage = owns;
	_invalidateNSImage();
}

LSize LImage::size() const
{
	if (_image == 0)
		return LZeroSize;
	LSize size = {(float)CGImageGetWidth(_image), (float)CGImageGetHeight(_image)};
	return size;
}

CGImageRef LImage::CGImageFromImageData(LData *data)
{
	if (data == 0)
		return 0;
	unsigned int length = data->length();
	const unsigned char *bytes = (const unsigned char *)data->bytes();
	bool jpeg = length >= 4 && bytes[0] == 0xff && bytes[1] == 0xd8 && bytes[2] == 0xff &&
	    (bytes[3] == 0xe0 || bytes[3] == 0xdb);
	bool png = !jpeg && length >= 8 && memcmp(bytes, "\x89PNG\r\n\x1a\n", 8) == 0;
	if (!jpeg && !png)
		return 0;
	data->retain();
	CGDataProviderRef provider = CGDataProviderCreateWithData(data, bytes, length, _dataProviderReleaseData);
	if (provider == 0)
		return 0;
	CGImageRef image = jpeg ? CGImageCreateWithJPEGDataProvider(provider, 0, false, kCGRenderingIntentDefault)
	                        : CGImageCreateWithPNGDataProvider(provider, 0, false, kCGRenderingIntentDefault);
	CGDataProviderRelease(provider);
	return image;
}

LImage *LImage::initWithData(LData *data)
{
	if (data == 0) {
		dealloc();
		return 0;
	}
	CGImageRef image = CGImageFromImageData(data);
	if (image == 0 && ILImageCallback::sCallback)
		image = ILImageCallback::sCallback->_CGImageFromImageData(data);
	return initWithCGImage(image, true);
}

LImage *LImage::initWithContentsOfFile(const LString *path)
{
	return initWithData(LData::dataWithContentsOfFile(path));
}

LImage *LImage::initWithBytes(void *bytes, unsigned int length)
{
	return initWithData(LData::dataWithBytes(bytes, length));
}

id LImage::NSImage()
{
	return [[_nsImage retain] autorelease];
}

void LImage::setNSImage(id image)
{
	[image retain];
	[_nsImage release];
	_nsImage = image;
}

void LImage::_invalidateNSImage()
{
	[_nsImage release];
	_nsImage = 0;
}

// LBitmapImage

void LBitmapImage::CGImageFormatFromFormat(TPixelFormat format, size_t *bitsPerComponent, size_t *bitsPerPixel,
    CGImageAlphaInfo *alphaInfo)
{
	size_t bits = 32;
	CGImageAlphaInfo alpha = kCGImageAlphaLast;
	if (format == kPixelFormatBGRX) {
		bits = 32;
		alpha = kCGImageAlphaNoneSkipLast;
	} else if (format == kPixelFormatBGR) {
		bits = 24;
		alpha = kCGImageAlphaNone;
	}
	if (bitsPerComponent)
		*bitsPerComponent = 8;
	if (bitsPerPixel)
		*bitsPerPixel = bits;
	if (alphaInfo)
		*alphaInfo = alpha;
}

void LBitmapImage::_convertBitmapToFormat(const unsigned char *src, unsigned char *dst, TPixelFormat srcFormat,
    TPixelFormat dstFormat, int pixels)
{
	for (int i = 0; i < pixels; i++) {
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		if (dstFormat == kPixelFormatBGRA)
			dst[3] = srcFormat == kPixelFormatBGRA ? src[3] : 0xff;
		dst += dstFormat < kPixelFormatBGR ? 4 : 3;
		src += srcFormat < kPixelFormatBGR ? 4 : 3;
	}
}

// An image of the size whose pixels come from info (0 for none yet).
static CGImageRef _createImage(unsigned int width, unsigned int height, TPixelFormat format,
    LBitmapImage::SBitmapInfo *info)
{
	size_t bitsPerPixel;
	CGImageAlphaInfo alpha;
	LBitmapImage::CGImageFormatFromFormat(format, 0, &bitsPerPixel, &alpha);
	CGDataProviderRef provider = CGDataProviderCreateSequential(info, &_dataProviderCallbacks);
	CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
	CGImageRef image = CGImageCreate(width, height, 8, bitsPerPixel, (bitsPerPixel >> 3) * width, space,
	    (CGBitmapInfo)alpha, provider, 0, false, kCGRenderingIntentDefault);
	if (space)
		CGColorSpaceRelease(space);
	if (provider)
		CGDataProviderRelease(provider);
	return image;
}

LBitmapImage *LBitmapImage::initWithSizeFormat(unsigned int width, unsigned int height, TPixelFormat format)
{
	if (LObject::init() == 0)
		return 0;
	_format = format;
	CGImageRef image = _createImage(width, height, format, 0);
	if (image) {
		setCGImage(image, true);
		return this;
	}
	dealloc();
	return 0;
}

void LBitmapImage::dealloc()
{
	if (_bitmap) {
		_bitmap->release();
		_bitmap = 0;
	}
	LImage::dealloc();
}

void LBitmapImage::setCGImage(CGImageRef image, unsigned char owns)
{
	// The pixels no longer match.
	if (_image != image && _bitmap) {
		_bitmap->release();
		_bitmap = 0;
	}
	LImage::setCGImage(image, owns);
}

LBitmapImage *LBitmapImage::initWithCGImage(CGImageRef image, unsigned char owns)
{
	return LImage::initWithCGImage(image, owns) ? this : 0;
}

LBitmapImage *LBitmapImage::initWithData(LData *data)
{
	return LImage::initWithData(data) ? this : 0;
}

LBitmapImage *LBitmapImage::initWithContentsOfFile(const LString *path)
{
	return LImage::initWithContentsOfFile(path) ? this : 0;
}

LBitmapImage *LBitmapImage::initWithBytes(void *bytes, unsigned int length)
{
	return LImage::initWithBytes(bytes, length) ? this : 0;
}

LData *LBitmapImage::representationOfType(TBitmapRepresentationType type)
{
	return ILImageCallback::sCallback ? ILImageCallback::sCallback->_representationOfTypeForBitmapImage(type, this) : 0;
}

LBitmapImage::StLocker::StLocker(LBitmapImage *image, unsigned int flags, TPixelFormat format)
    : _image(image), _flags(flags), _format(format)
{
	unsigned int width = image->width();
	unsigned int height = image->height();
	_info = (SBitmapInfo *)calloc(1, sizeof(SBitmapInfo));
	_info->bitsPerPixel = format == kPixelFormatBGR ? 24 : 32;
	_info->bytesPerRow = (_info->bitsPerPixel >> 3) * width;
	unsigned int size = height * _info->bytesPerRow;

	if (image->_bitmap == 0) {
		if (!(flags & kRead)) {
			image->_format = format;
			image->_bitmap = LData::_alloc();
			image->_bitmap->setLength(size);
		} else {
			// Draw the image to get BGRA pixels, not premultiplied.
			image->_bitmap = LData::_alloc();
			image->_bitmap->setLength(width * height * 4);
			uint32_t *pixel = (uint32_t *)image->_bitmap->bytes();
			CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
			if (space) {
				CGContextRef context = CGBitmapContextCreate(pixel, width, height, 8, width * 4, space,
				    kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Big);
				if (context) {
					CGRect rect = CGRectMake(0, 0, width, height);
					CGContextSetInterpolationQuality(context, kCGInterpolationNone);
					CGContextClearRect(context, rect);
					CGContextDrawImage(context, rect, image->_image);
					CGContextRelease(context);
					for (int i = 0; i < (int)(width * height); i++, pixel++) {
						unsigned char *argb = (unsigned char *)pixel;
						if ((unsigned char)(argb[0] - 1) < 0xfe) {
							unsigned int scale = 0xff00 / argb[0];
							argb[1] = (unsigned char)((argb[1] * scale) >> 8);
							argb[2] = (unsigned char)((argb[2] * scale) >> 8);
							argb[3] = (unsigned char)((argb[3] * scale) >> 8);
						}
						*pixel = OSSwapInt32(*pixel);
					}
				}
				CGColorSpaceRelease(space);
			}
		}
	}

	if (size == image->_bitmap->length()) {
		_info->data = LRetain(image->_bitmap);
		if ((flags & kRead) && format == kPixelFormatBGRA && image->_format == kPixelFormatBGRX) {
			unsigned char *bytes = (unsigned char *)_info->data->bytes();
			unsigned char *end = bytes + _info->data->length();
			for (unsigned char *p = bytes + 3; p < end; p += 4)
				*p = 0xff;
		}
	} else {
		_info->data = LData::_alloc();
		_info->data->setLength(size);
		if (flags & kRead)
			_convertBitmapToFormat((const unsigned char *)image->_bitmap->bytes(),
			    (unsigned char *)_info->data->bytes(), image->_format, format, width * height);
	}
	_info->cursor = (unsigned char *)_info->data->bytes();
}

LBitmapImage::StLocker::~StLocker()
{
	if (_flags & kWrite) {
		CGImageRef image = _createImage(_image->width(), _image->height(), _format, _info);
		if (image) {
			// The provider owns _info from now on.
			_image->setCGImage(image, true);
			_image->_format = _format;
			_image->_bitmap = LRetain(_info->data);
			return;
		}
	}
	if (_info->data) {
		_info->data->release();
		_info->data = 0;
	}
	free(_info);
}

LBitmapImage *LBitmapImage::cloneWithPixelFormat(TPixelFormat format)
{
	unsigned int w = width(), h = height();
	LBitmapImage *clone = LBitmapImage::_alloc()->initWithSizeFormat(w, h, format);
	if (clone == 0)
		return 0;
	StLocker src(this, StLocker::kRead, _format);
	StLocker dst(clone, StLocker::kWrite, format);
	_convertBitmapToFormat((const unsigned char *)src.bitmapData(), (unsigned char *)dst.bitmapData(), _format,
	    format, w * h);
	return clone;
}

LBitmapImage *LBitmapImage::scaledCloneWithSize(LSize size, unsigned int options)
{
	if (!(size.width > 0 && size.height > 0))
		return 0;
	unsigned int w = (unsigned int)size.width, h = (unsigned int)size.height;
	LBitmapImage *clone = LBitmapImage::_alloc()->initWithSizeFormat(w, h, kPixelFormatBGRA);
	if (clone == 0)
		return 0;
	bool failed;
	{
		StLocker src(this, StLocker::kRead, kPixelFormatBGRA);
		StLocker dst(clone, StLocker::kWrite, kPixelFormatBGRA);
		LSize from = this->size();
		vImage_Buffer srcBuffer = {src.bitmapData(), (vImagePixelCount)from.height, (vImagePixelCount)from.width,
		    src.bytesPerRow()};
		vImage_Buffer dstBuffer = {dst.bitmapData(), h, w, dst.bytesPerRow()};
		float sx = size.width / from.width;
		float sy = size.height / from.height;
		if (options & 1)
			sx = sy = sx < sy ? sx : sy;
		if (!(options & 2)) {
			if (sx > 1)
				sx = 1;
			if (sy > 1)
				sy = 1;
		}
		vImage_AffineTransform transform = {sx, 0, 0, sy, (size.width - sx * from.width) * 0.5f,
		    (size.height - sy * from.height) * 0.5f};
		Pixel_8888 background = {0, 0, 0, 0};
		// The original passed no edge flag, which current vImage rejects; the
		// background it gave is used for the edges.
		failed = vImageAffineWarp_ARGB8888(&srcBuffer, &dstBuffer, 0, &transform, background,
		    kvImageBackgroundColorFill) != kvImageNoError;
	}
	// After the lockers are done with it.
	if (failed) {
		clone->release();
		clone = 0;
	}
	return clone;
}

} // namespace LF
