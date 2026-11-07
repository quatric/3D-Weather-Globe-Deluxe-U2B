// ImageDecompressor and its JPEG, PNG and DDS kinds, and GLContextHolder.

#import <AppKit/AppKit.h>
#include "FHTextures.h"
#include "CThreadData.h"

LF_IMPLEMENT_CLASS(ImageDecompressor, 0)
LF_IMPLEMENT_CLASS(JPEGDecompressor, LF_CLASSOF(ImageDecompressor))
LF_IMPLEMENT_CLASS(PNGDecompressor, LF_CLASSOF(ImageDecompressor))
LF_IMPLEMENT_CLASS(DDSDecompressor, LF_CLASSOF(ImageDecompressor))
LF_IMPLEMENT_CLASS(GLContextHolder, 0)

static LConstantString _kGLContextHolderKey("_kGLContextHolderKey");
static CSharableResource _sRefContextLock;
static NSOpenGLContext *_oldContext = nil;

// ---------------------------------------------------------------------------
// GLContextHolder

GLContextHolder::GLContextHolder() : _context(0)
{
	NSOpenGLPixelFormatAttribute attributes[] = { NSOpenGLPFAAllRenderers, NSOpenGLPFADepthSize, 0, 0 };
	NSOpenGLPixelFormat *format = [[[NSOpenGLPixelFormat alloc] initWithAttributes:attributes] autorelease];
	if (format)
		_context = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
}

void GLContextHolder::dealloc()
{
	[(NSOpenGLContext *)_context release];
	_context = 0;
	LObject::dealloc();
}

// ---------------------------------------------------------------------------
// ImageDecompressor

ImageDecompressor *ImageDecompressor::initWithData(LData *data)
{
	data->retain();
	_data = data;
	return this;
}

void ImageDecompressor::dealloc()
{
	if (_data) {
		_data->release();
		_data = 0;
	}
	LObject::dealloc();
}

void ImageDecompressor::fillImageInfo(D3DXIMAGE_INFO *info)
{
	info->Width = width();
	info->Height = height();
	info->Depth = depth();
	info->Format = nativeFormat();
	info->MipLevels = mipLevelsCount();
	info->ResourceType = type();
	info->ImageFileFormat = imageFormat();
}

unsigned int ImageDecompressor::_nativeRowSize()
{
	return CBaseTexture::sizeOfPixelInFormat(nativeFormat()) * width();
}

unsigned int ImageDecompressor::nativeBitmapSize()
{
	return FHGetSurfaceSize(width(), height(), nativeFormat());
}

bool ImageDecompressor::decompressToNativeBitmap(void *buffer)
{
	int lines = startDecompress();
	if (lines == 0)
		return false;
	unsigned int rowSize = _nativeRowSize();
	bool ok = true;
	for (int i = 0; i < lines; i++) {
		if (!decompressLine(buffer)) {
			ok = false;
			break;
		}
		buffer = (char *)buffer + rowSize;
	}
	finishDecompress();
	return ok;
}

LData *ImageDecompressor::_nativeData()
{
	LData *data = LData::dataWithLength(nativeBitmapSize());
	decompressToNativeBitmap(data->bytes());
	return data;
}

static bool _isDXTFormat(D3DFORMAT format)
{
	return format == D3DFMT_DXT1 || format == D3DFMT_DXT3 || format == D3DFMT_DXT5;
}

// Other formats are converted by GL, in a private context.
bool ImageDecompressor::decompressToBitmapWithFormat(D3DFORMAT format, void *buffer)
{
	D3DFORMAT native = nativeFormat();
	unsigned int w = width();
	unsigned int h = height();
	if (native == format && format != D3DFMT_UNKNOWN) {
		LData *data = _nativeData();
		memcpy(buffer, data->bytes(), data->length());
		return true;
	}
	if (!_pushAndSetCurrentRefContext())
		return false;
	GLuint texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	LData *data = _nativeData();
	GLenum srcFormat, srcType;
	if (_isDXTFormat(native)) {
		glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, BaseTextureEmu::internalTextureFormatFromD3DFormat(native),
		                          w > 3 ? w : 4, h < 4 ? 4 : h, 0, data->length(), data->bytes());
	} else {
		BaseTextureEmu::sourceFormatFromD3DFormat(native, &srcFormat, &srcType);
		glTexImage2D(GL_TEXTURE_2D, 0, BaseTextureEmu::internalTextureFormatFromD3DFormat(format), w, h, 0, srcFormat,
		             srcType, data->bytes());
		if (_isDXTFormat(format)) {
			GLint compressedSize = 0;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &compressedSize);
		}
	}
	if (_isDXTFormat(format)) {
		if (!_isDXTFormat(native) || native == format)
			glGetCompressedTexImage(GL_TEXTURE_2D, 0, buffer);
	} else {
		BaseTextureEmu::sourceFormatFromD3DFormat(format, &srcFormat, &srcType);
		glGetTexImage(GL_TEXTURE_2D, 0, srcFormat, srcType, buffer);
	}
	glPopClientAttrib();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &texture);
	_popContext();
	return true;
}

LData *ImageDecompressor::ddsHeaderForFormatAndSize(D3DFORMAT format, unsigned int width, unsigned int height)
{
	LData *data = LData::dataWithLength(0x80);
	uint32_t *header = (uint32_t *)data->bytes();
	header[0] = 0x20534444;	// "DDS "
	memset(header + 1, 0, 0x7c);
	bool compressed = format == D3DFMT_DXT1 || format == D3DFMT_DXT2 || format == D3DFMT_DXT3 ||
	                  format == D3DFMT_DXT4 || format == D3DFMT_DXT5;
	header[1] = 0x7c;
	// CAPS, HEIGHT, WIDTH, PIXELFORMAT, MIPMAPCOUNT and LINEARSIZE or PITCH
	header[2] = compressed ? 0xa1007 : 0x2100f;
	header[3] = height;
	header[4] = width;
	if (compressed)
		header[5] = FHGetSurfaceSize(width, height, format);
	else
		header[5] = FHGetSurfaceSize(width, header[3], format) / header[3];
	header[0x13] = 0x20;
	if (compressed) {
		header[0x14] = 4;	// DDPF_FOURCC
		header[0x15] = format;
	} else {
		unsigned int flags, bits, r, g, b, a;
		switch ((unsigned int)format) {
		case 0x14:
		case 0x17:
			flags = 0x40;
			bits = 0x18;
			r = 0xff0000, g = 0xff00, b = 0xff, a = 0;
			break;
		case 0x15:
			flags = 0x41;
			bits = 0x20;
			r = 0xff0000, g = 0xff00, b = 0xff, a = 0xff000000;
			break;
		case 0x16:
			flags = 0x40;
			bits = 0x20;
			r = 0xff0000, g = 0xff00, b = 0xff, a = 0;
			break;
		case 0x18:
			flags = 0x40;
			bits = 0x10;
			r = 0x7c00, g = 0x3e0, b = 0x1f, a = 0;
			break;
		case 0x19:
			flags = 0x41;
			bits = 0x10;
			r = 0x7c00, g = 0x3e0, b = 0x1f, a = 0x8000;
			break;
		case 0x1a:
			flags = 0x41;
			bits = 0x10;
			r = 0xf00, g = 0xf0, b = 0xf, a = 0xf000;
			break;
		case 0x1c:
			flags = 2;
			bits = 8;
			r = 0, g = 0, b = 0, a = 0xff;
			break;
		case 0x1e:
			flags = 0x40;
			bits = 0x10;
			r = 0xf00, g = 0xf0, b = 0xf, a = 0;
			break;
		case 0x23:
			flags = 0x41;
			bits = 0x20;
			r = 0x3ff, g = 0xffc00, b = 0x3ff00000, a = 0xc0000000;
			break;
		case 0x32:
			flags = 0x20000;
			bits = 8;
			r = 0xff, g = 0, b = 0, a = 0;
			break;
		default:
			return 0;
		}
		header[0x14] = flags;
		header[0x16] = bits;
		header[0x17] = r;
		header[0x18] = g;
		header[0x19] = b;
		header[0x1a] = a;
	}
	header[0x1b] = 0x1000;	// DDSCAPS_TEXTURE
	return data;
}

ImageDecompressor *ImageDecompressor::decompressorForData(LData *data)
{
	if (data == 0 || data->length() < 8)
		return 0;
	const unsigned char *bytes = (const unsigned char *)data->bytes();
	ImageDecompressor *decompressor;
	if (bytes[0] == 0xff) {
		if (bytes[1] != 0xd8 || bytes[2] != 0xff || (bytes[3] != 0xe0 && bytes[3] != 0xdb))
			return 0;
		decompressor = JPEGDecompressor::_alloc()->initWithData(data);
	} else if (bytes[0] == 0x89) {
		if (memcmp(bytes + 1, "PNG\r\n\x1a\n", 7) != 0)
			return 0;
		decompressor = PNGDecompressor::_alloc()->initWithData(data);
	} else {
		if (memcmp(bytes, "DDS ", 4) != 0)
			return 0;
		decompressor = DDSDecompressor::_alloc()->initWithData(data);
	}
	return decompressor ? LAutorelease(decompressor) : 0;
}

bool ImageDecompressor::_pushAndSetCurrentRefContext()
{
	_sRefContextLock.lock();
	_oldContext = [NSOpenGLContext currentContext];
	CThreadData *threadData = CThreadData::threadData();
	LObject *holder = threadData->objectForKey(&_kGLContextHolderKey);
	if (holder == 0 || !holder->getClass()->isSubclassOf(LF_CLASSOF(GLContextHolder))) {
		holder = GLContextHolder::_alloc();
		threadData->setObjectForKey(holder, &_kGLContextHolderKey);
		holder->release();
	}
	NSOpenGLContext *context = (NSOpenGLContext *)((GLContextHolder *)holder)->glContext();
	if (context == nil)
		_sRefContextLock.unlock();
	else
		[context makeCurrentContext];
	return context != nil;
}

void ImageDecompressor::_popContext()
{
	if (_oldContext == nil) {
		[NSOpenGLContext clearCurrentContext];
	} else {
		[_oldContext makeCurrentContext];
		_oldContext = nil;
	}
	_sRefContextLock.unlock();
}

// ---------------------------------------------------------------------------
// CGImage decoding

_imageDecompressorImp::~_imageDecompressorImp()
{
	CGImageRelease(_image);
	_image = 0;
}

unsigned int _imageDecompressorImp::width()
{
	return (unsigned int)CGImageGetWidth(_image);
}

unsigned int _imageDecompressorImp::height()
{
	return (unsigned int)CGImageGetHeight(_image);
}

// The original took the format from the image's bits per pixel, which no
// longer match what ImageIO returns (RGB comes padded to 32 bits, PNG alpha
// isn't "first"). Now: 8-bit grey without alpha is A8, as before (not for
// JPEG, whose native format is always X8R8G8B8); anything else is drawn as
// 32-bit BGRA, with alpha or without.
D3DFORMAT _imageDecompressorImp::srcPixelFormat()
{
	if (_image == 0)
		return D3DFMT_UNKNOWN;
	CGImageAlphaInfo alpha = CGImageGetAlphaInfo(_image);
	bool hasAlpha = alpha != kCGImageAlphaNone && alpha != kCGImageAlphaNoneSkipLast && alpha != kCGImageAlphaNoneSkipFirst;
	CGColorSpaceModel model = CGColorSpaceGetModel(CGImageGetColorSpace(_image));
	if (!_alwaysRGB && model == kCGColorSpaceModelMonochrome && !hasAlpha && CGImageGetBitsPerPixel(_image) == 8)
		return D3DFMT_A8;
	return hasAlpha ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8;
}

bool _imageDecompressorImp::decompressLine(void *buffer)
{
	if (_image == 0)
		return false;
	size_t w = CGImageGetWidth(_image);
	size_t h = CGImageGetHeight(_image);
	CGColorSpaceRef space;
	size_t rowBytes;
	uint32_t info;
	switch (srcPixelFormat()) {
	case D3DFMT_A8:
		space = CGColorSpaceCreateDeviceGray();
		rowBytes = w;
		info = kCGImageAlphaNone;
		break;
	case D3DFMT_A8R8G8B8:
		space = CGColorSpaceCreateDeviceRGB();
		rowBytes = w * 4;
		info = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little;
		break;
	default:
		space = CGColorSpaceCreateDeviceRGB();
		rowBytes = w * 4;
		info = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little;
		break;
	}
	CGContextRef context = CGBitmapContextCreate(buffer, w, h, 8, rowBytes, space, info);
	CGColorSpaceRelease(space);
	if (context == 0)
		return false;
	CGContextSetInterpolationQuality(context, kCGInterpolationNone);
	CGRect rect = CGRectMake(0, 0, (CGFloat)w, (CGFloat)h);
	CGContextClearRect(context, rect);
	CGContextDrawImage(context, rect, _image);
	CGContextRelease(context);
	return true;
}

_jpegDecompressorImp::_jpegDecompressorImp(char *bytes, unsigned int length)
{
	CGDataProviderRef provider = CGDataProviderCreateWithData(0, bytes, length, 0);
	if (provider) {
		_image = CGImageCreateWithJPEGDataProvider(provider, 0, false, kCGRenderingIntentDefault);
		_alwaysRGB = true;
		CFRelease(provider);
	}
}

_pngDecompressorImp::_pngDecompressorImp(char *bytes, unsigned int length)
{
	CGDataProviderRef provider = CGDataProviderCreateWithData(0, bytes, length, 0);
	if (provider) {
		_image = CGImageCreateWithPNGDataProvider(provider, 0, false, kCGRenderingIntentDefault);
		CFRelease(provider);
	}
}

// ---------------------------------------------------------------------------
// JPEG and PNG

JPEGDecompressor *JPEGDecompressor::initWithData(LData *data)
{
	ImageDecompressor::initWithData(data);
	_imp = new _jpegDecompressorImp((char *)data->bytes(), data->length());
	return this;
}

void JPEGDecompressor::dealloc()
{
	delete _imp;
	_imp = 0;
	ImageDecompressor::dealloc();
}

PNGDecompressor *PNGDecompressor::initWithData(LData *data)
{
	ImageDecompressor::initWithData(data);
	_imp = new _pngDecompressorImp((char *)data->bytes(), data->length());
	return this;
}

void PNGDecompressor::dealloc()
{
	delete _imp;
	_imp = 0;
	ImageDecompressor::dealloc();
}

// ---------------------------------------------------------------------------
// DDS

DDSDecompressor *DDSDecompressor::initWithData(LData *data)
{
	ImageDecompressor::initWithData(data);
	const uint32_t *header = (const uint32_t *)data->bytes();
	unsigned int flags;
	if (data->length() <= 0x80 || header[0] != 0x20534444)
		goto failed;
	flags = header[2];
	_info.ImageFileFormat = D3DXIFF_DDS;
	// DDSD_WIDTH, DDSD_HEIGHT and DDSD_CAPS are needed
	if (!(flags & 4))
		goto failed;
	_info.Width = header[4];
	if (!(flags & 2))
		goto failed;
	_info.Height = header[3];
	_info.Depth = (flags & 0x800000) ? header[6] : 0;
	if (!(flags & 1))
		goto failed;
	{
		_info.MipLevels = (flags & 0x20000) ? header[7] : 0;
		unsigned int caps2 = header[0x1c];
		_info.ResourceType = D3DRTYPE_TEXTURE;
		if (caps2 & 0x200)
			_info.ResourceType = D3DRTYPE_CUBETEXTURE;
		else if (caps2 & 0x200000)
			_info.ResourceType = D3DRTYPE_VOLUMETEXTURE;
		unsigned int pfFlags = header[0x14];
		unsigned int bits = header[0x16];
		unsigned int r = header[0x17], g = header[0x18], b = header[0x19], a = header[0x1a];
		_info.Format = D3DFMT_UNKNOWN;
		if (pfFlags & 0x40) {	// DDPF_RGB
			if (bits == 0x18) {
				if (g == 0xff00 && r == 0xff0000 && b == 0xff)
					_info.Format = D3DFMT_R8G8B8;
			} else if (bits == 0x20) {
				if (g == 0xff00 && r == 0xff0000 && b == 0xff)
					_info.Format = a != 0xff000000 ? D3DFMT_X8R8G8B8 : D3DFMT_A8R8G8B8;
				else if (g == 0xffc00 && b == 0x3ff00000 && r == 0x3ff && a == 0xc0000000)
					_info.Format = (D3DFORMAT)0x23;
			} else if (bits == 0x10) {
				if (g == 0x3e0 && r == 0x7c00 && b == 0x1f)
					_info.Format = a == 0x8000 ? D3DFMT_A1R5G5B5 : D3DFMT_X1R5G5B5;
				else if (g == 0xf0 && r == 0xf00 && b == 0xf)
					_info.Format = a != 0xf000 ? D3DFMT_X4R4G4B4 : D3DFMT_A4R4G4B4;
				else if (g == 0x7e0 && r == 0xf800 && b == 0x1f)
					_info.Format = D3DFMT_R5G6B5;
			}
		} else if (pfFlags & 4) {	// DDPF_FOURCC
			_info.Format = (D3DFORMAT)header[0x15];
		} else if (pfFlags & 2) {	// DDPF_ALPHA
			if (bits == 8)
				_info.Format = D3DFMT_L8;
		} else if (pfFlags & 0x20000) {	// DDPF_LUMINANCE
			if (bits == 8)
				_info.Format = D3DFMT_L8;
			else if (bits == 0x10 && a == 0xff00)
				_info.Format = D3DFMT_A8L8;
		}
		return this;
	}
failed:
	release();
	return 0;
}

bool DDSDecompressor::decompressToTexture(C2DTexture *texture)
{
	return FHFillTextureWithData(&_info, _data->bytes(), (Texture2DEmu *)texture->provideTexture()) >= 0;
}

bool DDSDecompressor::decompressLine(void *buffer)
{
	LData *data = _nativeData();
	memcpy(buffer, data->bytes(), data->length());
	return true;
}

LData *DDSDecompressor::_nativeData()
{
	return LData::dataWithBytesNoCopy((char *)_data->bytes() + 0x80, nativeBitmapSize(), false);
}
