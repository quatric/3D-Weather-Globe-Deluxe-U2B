// Textures: CBaseTexture and its 2D and volume kinds, which get their pixels
// either from an image file (DDS) or from a data source object, and the image
// decompressors that read JPEG, PNG and DDS files.
#ifndef FHTEXTURES_H
#define FHTEXTURES_H

#include "FHContext.h"
#include "FHRender.h"
#include <CoreGraphics/CoreGraphics.h>

class CBaseTexture;
class C2DTexture;
class CVolumeTexture;

// ---------------------------------------------------------------------------
// Data sources: describe a texture and fill it once it exists.

class CTextureDataSource {
public:
	virtual bool isDynamicTexture(CBaseTexture *texture, CContext *context) = 0;
	virtual bool autoGenMipMapsForTexture(CBaseTexture *, CContext *) { return false; }
	virtual unsigned int mipmapsCountForTexture(CBaseTexture *, CContext *) { return 1; }
	// The formats the texture can have, best first; 0 ends the list.
	virtual D3DFORMAT textureFormat(CBaseTexture *texture, unsigned int index, CContext *context) = 0;
	virtual void textureWasInstantiated(CBaseTexture *texture) = 0;
	virtual void textureWillRelease(CBaseTexture *) {}
	virtual void textureWillDie(CBaseTexture *) {}
	virtual ~CTextureDataSource() {}
};

class C2DTextureDataSource : public CTextureDataSource {
public:
	virtual unsigned int widthOfTexture(C2DTexture *texture, CContext *context) = 0;
	virtual unsigned int heightOfTexture(C2DTexture *texture, CContext *context) = 0;
};

class CVolumeTextureDataSource : public CTextureDataSource {
public:
	virtual unsigned int widthOfTexture(CVolumeTexture *texture, CContext *context) = 0;
	virtual unsigned int heightOfTexture(CVolumeTexture *texture, CContext *context) = 0;
	virtual unsigned int depthOfTexture(CVolumeTexture *texture, CContext *context) = 0;
};

// A texture of a given size and format whose pixels the owner sets.
class CParametric2DTextureDataSource : public LObject, public C2DTextureDataSource {
	LF_DECLARE_CLASS(CParametric2DTextureDataSource)
public:
	CParametric2DTextureDataSource() : _width(0), _height(0), _format(D3DFMT_UNKNOWN), _usage(0), _noMipmaps(false) {}
	virtual CParametric2DTextureDataSource *getOwnerLObject() { return this; }
	virtual bool isDynamicTexture(CBaseTexture *, CContext *) { return (_usage >> 9) & 1; }
	virtual bool autoGenMipMapsForTexture(CBaseTexture *, CContext *) { return (_usage >> 10) & 1; }
	virtual unsigned int mipmapsCountForTexture(CBaseTexture *, CContext *) { return _noMipmaps == 0; }
	virtual D3DFORMAT textureFormat(CBaseTexture *, unsigned int index, CContext *)
	{
		return index == 0 ? _format : D3DFMT_UNKNOWN;
	}
	virtual unsigned int widthOfTexture(C2DTexture *, CContext *) { return _width; }
	virtual unsigned int heightOfTexture(C2DTexture *, CContext *) { return _height; }
	virtual void textureWasInstantiated(CBaseTexture *) {}

	// Autoreleased.
	static C2DTextureDataSource *dataSourceWithParams(unsigned int width, unsigned int height, D3DFORMAT format,
	                                                  unsigned int usage, unsigned char noMipmaps);

	unsigned int _width, _height;
	D3DFORMAT _format;
	unsigned int _usage;
	unsigned char _noMipmaps;
};

// ---------------------------------------------------------------------------
// CBaseTexture

class CBaseTexture : public CContextObject, public CTextureProviderCallback {
	LF_DECLARE_ABSTRACT_CLASS(CBaseTexture)
public:
	CBaseTexture();
	virtual void dealloc();
	virtual void instantiateSelf();
	virtual void releaseSelf();
	virtual BaseTextureEmu *provideTexture() { return _texture; }
	virtual LString *fillDescription() const;
	virtual D3DRESOURCETYPE _resourceType() = 0;
	virtual void _createTextureObjectFromFile(LData *data, unsigned int mipLevels, D3DFORMAT format,
	                                          unsigned int usage, D3DPOOL pool) = 0;
	virtual void _createTextureObjectWithInternalProvider(D3DFORMAT format, unsigned int usage, D3DPOOL pool) = 0;

	CBaseTexture *initWithDataSource(CTextureDataSource *source, CHolder *holder, unsigned char retainSource);
	CBaseTexture *initWithHolderAndPath(CHolder *holder, const LString *path);
	BaseTextureEmu *texture() const { return _texture; }
	const D3DXIMAGE_INFO *imageInfo() const { return &_info; }
	LString *path() const { return _path; }
	CTextureDataSource *dataSource() const { return _dataSource; }

	// The holder's texture for the file, or a new autoreleased one of `cls`
	// (C2DTexture if 0).
	static CBaseTexture *textureWithContentsOfFile(LString *path, CHolder *holder, LClass *cls);
	static unsigned int sizeOfPixelInFormat(D3DFORMAT format);
	static unsigned int colorChannelBits(D3DFORMAT format);
	static unsigned int alphaChannelBits(D3DFORMAT format);
	static unsigned int depthBits(D3DFORMAT format);
	static unsigned int stencilBits(D3DFORMAT format);

	D3DXIMAGE_INFO _info;
	LString *_path;
	CTextureDataSource *_dataSource;
	LObject *_dataSourceOwner;	// the source, retained, when it is an LObject
	bool _dynamic;
	bool _autoGenMipmaps;
	bool _autoGenMipmapsOn;
	BaseTextureEmu *_texture;
};

class C2DTexture : public CBaseTexture {
	LF_DECLARE_CLASS(C2DTexture)
public:
	// Locks a level of the texture for its scope.
	class StLocker {
	public:
		StLocker(C2DTexture *texture, unsigned int level, unsigned int flags);
		~StLocker();
		void *bits() const { return _locked.pBits; }
		unsigned int bytesPerRow();
		const D3DSURFACE_DESC *desc() const { return &_desc; }

		Texture2DEmu *_texture;
		unsigned int _level;
		D3DLOCKED_RECT _locked;
		D3DSURFACE_DESC _desc;
		unsigned int _flags;
	};

	virtual D3DRESOURCETYPE _resourceType() { return D3DRTYPE_TEXTURE; }
	virtual void _createTextureObjectFromFile(LData *data, unsigned int mipLevels, D3DFORMAT format,
	                                          unsigned int usage, D3DPOOL pool);
	virtual void _createTextureObjectWithInternalProvider(D3DFORMAT format, unsigned int usage, D3DPOOL pool);
	virtual C2DTexture *getOwnerLObject() { return this; }

	C2DTexture *initWithDataSource(C2DTextureDataSource *source, CHolder *holder, unsigned char retainSource);
	C2DTexture *initWithHolderAndParams(CHolder *holder, unsigned int width, unsigned int height, D3DFORMAT format,
	                                    unsigned int usage, unsigned char noMipmaps);
	C2DTexture *initWithHolderAndPath(CHolder *holder, const LString *path)
	{
		CBaseTexture::initWithHolderAndPath(holder, path);
		return this;
	}
	void dataSourceWillDie();
	Texture2DEmu *texture2D() const { return (Texture2DEmu *)_texture; }

	static C2DTexture *textureWithContentsOfFile(LString *path, CHolder *holder)
	{
		return (C2DTexture *)CBaseTexture::textureWithContentsOfFile(path, holder, LF_CLASSOF(C2DTexture));
	}
};

class CVolumeTexture : public CBaseTexture {
	LF_DECLARE_CLASS(CVolumeTexture)
public:
	virtual D3DRESOURCETYPE _resourceType() { return D3DRTYPE_VOLUMETEXTURE; }
	virtual void _createTextureObjectFromFile(LData *, unsigned int, D3DFORMAT, unsigned int, D3DPOOL) {}
	virtual void _createTextureObjectWithInternalProvider(D3DFORMAT format, unsigned int usage, D3DPOOL pool);
	virtual CVolumeTexture *getOwnerLObject() { return this; }

	CVolumeTexture *initWithDataSource(CVolumeTextureDataSource *source, CHolder *holder, unsigned char retainSource)
	{
		CBaseTexture::initWithDataSource(source, holder, retainSource);
		return this;
	}
	CVolumeTexture *initWithHolderAndPath(CHolder *holder, const LString *path)
	{
		CBaseTexture::initWithHolderAndPath(holder, path);
		return this;
	}
	static CVolumeTexture *textureWithContentsOfFile(LString *path, CHolder *holder)
	{
		return (CVolumeTexture *)CBaseTexture::textureWithContentsOfFile(path, holder, LF_CLASSOF(CVolumeTexture));
	}
};

// ---------------------------------------------------------------------------
// Image decompressors.

class ImageDecompressor : public LObject {
	LF_DECLARE_ABSTRACT_CLASS(ImageDecompressor)
public:
	ImageDecompressor() : _data(0) {}
	virtual void dealloc();
	virtual unsigned int width() = 0;
	virtual unsigned int height() = 0;
	virtual unsigned int depth() { return 1; }
	virtual D3DFORMAT nativeFormat() = 0;
	virtual unsigned int mipLevelsCount() { return 1; }
	virtual D3DRESOURCETYPE type() { return D3DRTYPE_TEXTURE; }
	virtual D3DXIMAGE_FILEFORMAT imageFormat() = 0;
	virtual unsigned int nativeBitmapSize();
	virtual bool decompressToNativeBitmap(void *buffer);
	virtual bool decompressToBitmapWithFormat(D3DFORMAT format, void *buffer);
	virtual bool decompressToTexture(C2DTexture *) { return false; }
	// Returns the number of decompressLine calls to make.
	virtual int startDecompress() = 0;
	virtual bool decompressLine(void *buffer) = 0;
	virtual bool finishDecompress() = 0;
	virtual unsigned int _nativeRowSize();
	virtual LData *_nativeData();

	ImageDecompressor *initWithData(LData *data);
	void fillImageInfo(D3DXIMAGE_INFO *info);

	// A JPEG, PNG or DDS decompressor for the data (autoreleased), or 0.
	static ImageDecompressor *decompressorForData(LData *data);
	// A DDS file header for a picture (autoreleased).
	static LData *ddsHeaderForFormatAndSize(D3DFORMAT format, unsigned int width, unsigned int height);
	// Make a private GL context current while converting formats.
	static bool _pushAndSetCurrentRefContext();
	static void _popContext();

	LData *_data;
};

// CGImage based decoding: the whole picture is drawn in one "line".
class _imageDecompressorImp {
public:
	_imageDecompressorImp() : _image(0), _alwaysRGB(false) {}
	virtual ~_imageDecompressorImp();
	virtual int startDecompress() { return 1; }
	virtual bool decompressLine(void *buffer);
	virtual bool finishDecompress() { return true; }
	unsigned int width();
	unsigned int height();
	// The D3D format decompressLine produces.
	D3DFORMAT srcPixelFormat();

	CGImageRef _image;
	bool _alwaysRGB;	// JPEG: grey pictures as X8R8G8B8 too (JPEGDecompressor's native format)
};

class _jpegDecompressorImp : public _imageDecompressorImp {
public:
	_jpegDecompressorImp(char *bytes, unsigned int length);
};

class _pngDecompressorImp : public _imageDecompressorImp {
public:
	_pngDecompressorImp(char *bytes, unsigned int length);
};

class JPEGDecompressor : public ImageDecompressor {
	LF_DECLARE_CLASS(JPEGDecompressor)
public:
	JPEGDecompressor() : _imp(0) {}
	virtual void dealloc();
	virtual unsigned int width() { return _imp->width(); }
	virtual unsigned int height() { return _imp->height(); }
	virtual D3DFORMAT nativeFormat() { return D3DFMT_X8R8G8B8; }
	virtual D3DXIMAGE_FILEFORMAT imageFormat() { return D3DXIFF_JPG; }
	virtual int startDecompress() { return _imp->startDecompress(); }
	virtual bool decompressLine(void *buffer) { return _imp->decompressLine(buffer); }
	virtual bool finishDecompress() { return _imp->finishDecompress(); }
	virtual JPEGDecompressor *getOwnerLObject() { return this; }
	JPEGDecompressor *initWithData(LData *data);

	_jpegDecompressorImp *_imp;
};

class PNGDecompressor : public ImageDecompressor {
	LF_DECLARE_CLASS(PNGDecompressor)
public:
	PNGDecompressor() : _imp(0) {}
	virtual void dealloc();
	virtual unsigned int width() { return _imp->width(); }
	virtual unsigned int height() { return _imp->height(); }
	virtual D3DFORMAT nativeFormat() { return _imp->srcPixelFormat(); }
	virtual D3DXIMAGE_FILEFORMAT imageFormat() { return D3DXIFF_PNG; }
	virtual int startDecompress() { return _imp->startDecompress(); }
	virtual bool decompressLine(void *buffer) { return _imp->decompressLine(buffer); }
	virtual bool finishDecompress() { return _imp->finishDecompress(); }
	virtual PNGDecompressor *getOwnerLObject() { return this; }
	PNGDecompressor *initWithData(LData *data);

	_pngDecompressorImp *_imp;
};

class DDSDecompressor : public ImageDecompressor {
	LF_DECLARE_CLASS(DDSDecompressor)
public:
	DDSDecompressor() { memset(&_info, 0, sizeof _info); }
	virtual unsigned int width() { return _info.Width; }
	virtual unsigned int height() { return _info.Height; }
	virtual unsigned int depth() { return _info.Depth; }
	virtual D3DFORMAT nativeFormat() { return _info.Format; }
	virtual unsigned int mipLevelsCount() { return _info.MipLevels; }
	virtual D3DXIMAGE_FILEFORMAT imageFormat() { return D3DXIFF_DDS; }
	virtual bool decompressToTexture(C2DTexture *texture);
	virtual int startDecompress() { return 1; }
	virtual bool decompressLine(void *buffer);
	virtual bool finishDecompress() { return true; }
	virtual LData *_nativeData();
	virtual DDSDecompressor *getOwnerLObject() { return this; }
	DDSDecompressor *initWithData(LData *data);

	// Width, height, depth, mip levels, format, resource type (3 texture,
	// 4 volume, 5 cube) and file format.
	D3DXIMAGE_INFO _info;
};

#endif
