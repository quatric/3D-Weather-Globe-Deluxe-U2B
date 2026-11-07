// CBaseTexture, C2DTexture, CVolumeTexture and CParametric2DTextureDataSource.

#include "FHTextures.h"

LF_IMPLEMENT_CLASS(CBaseTexture, LF_CLASSOF(CContextObject))
LF_IMPLEMENT_CLASS(C2DTexture, LF_CLASSOF(CBaseTexture))
LF_IMPLEMENT_CLASS(CVolumeTexture, LF_CLASSOF(CBaseTexture))
LF_IMPLEMENT_CLASS(CParametric2DTextureDataSource, 0)

// ---------------------------------------------------------------------------
// Pixel formats

unsigned int CBaseTexture::sizeOfPixelInFormat(D3DFORMAT format)
{
	static unsigned int sizes[0x77];
	static bool valid = false;
	if (!valid) {
		sizes[0x14] = 3;
		sizes[0x15] = 4;
		sizes[0x16] = 4;
		sizes[0x17] = 2;
		sizes[0x18] = 2;
		sizes[0x19] = 2;
		sizes[0x1a] = 2;
		sizes[0x1b] = 1;
		sizes[0x1c] = 1;
		sizes[0x32] = 1;
		sizes[0x33] = 2;
		sizes[0x1d] = 2;
		sizes[0x1e] = 2;
		sizes[0x1f] = 4;
		sizes[0x20] = 4;
		sizes[0x21] = 4;
		sizes[0x22] = 4;
		sizes[0x23] = 4;
		sizes[0x24] = 8;
		sizes[0x46] = 2;
		sizes[0x6f] = 2;
		sizes[0x70] = 4;
		sizes[0x71] = 8;
		sizes[0x72] = 4;
		sizes[0x73] = 8;
		sizes[0x74] = 16;
		valid = true;
	}
	if ((unsigned int)format < 0x77)
		return sizes[format];
	switch ((unsigned int)format) {
	case D3DFMT_DXT1:
		return 2;
	case MAKEFOURCC('M', 'E', 'T', '1'):
		return 8;
	case MAKEFOURCC('Y', 'U', 'Y', '2'):
		return 3;
	case D3DFMT_DXT2:
	case D3DFMT_DXT3:
	case D3DFMT_DXT4:
	case D3DFMT_DXT5:
	case MAKEFOURCC('G', 'R', 'G', 'B'):
	case MAKEFOURCC('R', 'G', 'B', 'G'):
	case MAKEFOURCC('U', 'Y', 'V', 'Y'):
		return 4;
	default:
		return 0;
	}
}

unsigned int CBaseTexture::colorChannelBits(D3DFORMAT format)
{
	switch (format) {
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x32:
		return 8;
	case 0x17:
	case 0x18:
	case 0x19:
		return 5;
	case 0x1a:
	case 0x1e:
		return 4;
	case 0x1b:
	case 0x1d:
		return 2;
	case 0x1f:
	case 0x23:
		return 10;
	default:
		return 0;
	}
}

unsigned int CBaseTexture::alphaChannelBits(D3DFORMAT format)
{
	switch (format) {
	case 0x15:
	case 0x1d:
		return 8;
	case 0x19:
		return 1;
	case 0x1a:
		return 4;
	case 0x1f:
	case 0x23:
		return 2;
	default:
		return 0;
	}
}

unsigned int CBaseTexture::depthBits(D3DFORMAT format)
{
	switch (format) {
	case 0x47:
		return 32;
	case 0x49:
		return 15;
	case 0x4b:
	case 0x4d:
	case 0x4f:
		return 24;
	case 0x50:
		return 16;
	default:
		return 0;
	}
}

unsigned int CBaseTexture::stencilBits(D3DFORMAT format)
{
	if (format == 0x4b)
		return 8;
	if (format == 0x4f)
		return 4;
	return format == 0x49;
}

// ---------------------------------------------------------------------------
// CBaseTexture

CBaseTexture::CBaseTexture()
    : CContextObject(10), _path(0), _dataSource(0), _dataSourceOwner(0), _dynamic(false), _autoGenMipmaps(false),
      _autoGenMipmapsOn(false), _texture(0)
{
	memset(&_info, 0, sizeof _info);
}

CBaseTexture *CBaseTexture::initWithDataSource(CTextureDataSource *source, CHolder *holder, unsigned char retainSource)
{
	initWithHolder(holder);
	_dataSource = source;
	_dataSourceOwner = (retainSource && source) ? dynamic_cast<LObject *>(source) : 0;
	if (_dataSourceOwner)
		_dataSourceOwner->retain();
	return this;
}

CBaseTexture *CBaseTexture::initWithHolderAndPath(CHolder *holder, const LString *path)
{
	initWithHolder(holder);
	_path = LString::_alloc()->init();
	_path->setValue(path);
	if (_path && holder->textureByFileDesc(_path) == 0)
		_holder->setTextureForFileDescr(this, _path);
	return this;
}

void CBaseTexture::releaseSelf()
{
	_dynamic = false;
	_autoGenMipmaps = false;
	_autoGenMipmapsOn = false;
	if (_dataSource)
		_dataSource->textureWillRelease(this);
	FHSafeRelease(_texture);
}

void CBaseTexture::dealloc()
{
	if (_path) {
		if (_holder && _holder->textureByFileDesc(_path) == this)
			_holder->removeTextureWithFileDescr(_path);
		_path->release();
		_path = 0;
	}
	if (_dataSource)
		_dataSource->textureWillDie(this);
	FHSafeRelease(_texture);
	if (_dataSourceOwner) {
		_dataSourceOwner->release();
		_dataSourceOwner = 0;
	}
	_dataSource = 0;
	CContextObject::dealloc();
}

void CBaseTexture::instantiateSelf()
{
	if (_dataSource) {
		CContext *context = _context;
		D3DRESOURCETYPE type = _resourceType();
		D3DFORMAT format;
		for (unsigned int i = 0;; i++) {
			format = _dataSource->textureFormat(this, i, context);
			if (format == D3DFMT_UNKNOWN)
				return;
			if (context->checkDeviceFormat(format, 0, type))
				break;
		}
		unsigned int usage;
		D3DPOOL pool;
		if (_dataSource->isDynamicTexture(this, context) && context->caps()->dynamicTexturesAreSupported()) {
			pool = D3DPOOL_DEFAULT;
			_dynamic = true;
			usage = D3DUSAGE_DYNAMIC;
		} else {
			usage = 0;
			pool = D3DPOOL_MANAGED;
		}
		if (_dataSource->autoGenMipMapsForTexture(this, context) &&
		    context->checkDeviceFormat(format, usage | D3DUSAGE_AUTOGENMIPMAP, type)) {
			_autoGenMipmaps = true;
			_autoGenMipmapsOn = true;
			usage |= D3DUSAGE_AUTOGENMIPMAP;
		}
		// Default-pool textures are lost with the device.
		_managesSelf = pool == D3DPOOL_DEFAULT;
		_info.Format = format;
		_info.ImageFileFormat = D3DXIFF_DDS;
		_createTextureObjectWithInternalProvider(format, usage, pool);
		_dataSource->textureWasInstantiated(this);
		return;
	}
	if (_path == 0)
		return;
	LData *data = LData::dataWithContentsOfFile(_path);
	if (data == 0)
		return;
	FHGetImageInfoFromFileInMemory(data->bytes(), data->length(), &_info);
	_createTextureObjectFromFile(data, !_autoGenMipmaps, D3DFMT_UNKNOWN, 0, D3DPOOL_MANAGED);
}

LString *CBaseTexture::fillDescription() const
{
	char buffer[312];
	snprintf(buffer, sizeof buffer, "%s <%p>; texture path:\"%s\"", className(), this,
	         _path ? _path->cString() : "NULL");
	return LString::stringWithCString(buffer);
}

CBaseTexture *CBaseTexture::textureWithContentsOfFile(LString *path, CHolder *holder, LClass *cls)
{
	if (path == 0 || holder == 0)
		return 0;
	if (cls == 0)
		cls = LF_CLASSOF(C2DTexture);
	CBaseTexture *texture = holder->textureByFileDesc(path);
	if (texture)
		return texture;
	holder->beginAccumulationBlock();
	LObject *object = cls->alloc();
	if (!object->getClass()->isSubclassOf(LF_CLASSOF(CBaseTexture))) {
		object->release();
	} else {
		texture = ((CBaseTexture *)object)->initWithHolderAndPath(holder, path);
		if (texture)
			texture->l_autorelease();
	}
	holder->endAccumulationBlock();
	return texture;
}

// ---------------------------------------------------------------------------
// C2DTexture

C2DTexture::StLocker::StLocker(C2DTexture *texture, unsigned int level, unsigned int flags)
{
	_texture = (Texture2DEmu *)texture->_texture;
	LF_CHECK_PTR(_texture);
	_level = level;
	_flags = flags;
	HRESULT hr = _texture->GetLevelDesc(level, &_desc);
	if (hr < 0)
		LF_RAISE(FailedWithCodeException(hr));
	hr = _texture->LockRect(_level, &_locked, 0, _flags);
	if (hr < 0)
		LF_RAISE(FailedWithCodeException(hr));
}

C2DTexture::StLocker::~StLocker()
{
	HRESULT hr = _texture->UnlockRect(_level);
	if (hr < 0)
		LF_RAISE(FailedWithCodeException(hr));
}

unsigned int C2DTexture::StLocker::bytesPerRow()
{
	D3DFORMAT format = _desc.Format;
	if (format == D3DFMT_DXT1 || format == D3DFMT_DXT2 || format == D3DFMT_DXT3 || format == D3DFMT_DXT4 ||
	    format == D3DFMT_DXT5)
		return (CBaseTexture::sizeOfPixelInFormat(format) * _desc.Width) >> 2;
	return _locked.Pitch;
}

C2DTexture *C2DTexture::initWithDataSource(C2DTextureDataSource *source, CHolder *holder, unsigned char retainSource)
{
	CBaseTexture::initWithDataSource(source, holder, retainSource);
	return this;
}

C2DTexture *C2DTexture::initWithHolderAndParams(CHolder *holder, unsigned int width, unsigned int height,
                                                D3DFORMAT format, unsigned int usage, unsigned char noMipmaps)
{
	C2DTextureDataSource *source =
	    CParametric2DTextureDataSource::dataSourceWithParams(width, height, format, usage, noMipmaps);
	CBaseTexture::initWithDataSource(source, holder, 1);
	return this;
}

void C2DTexture::dataSourceWillDie()
{
	if (_dataSourceOwner) {
		_dataSourceOwner->release();
		_dataSourceOwner = 0;
	}
	_dataSource = 0;
}

void C2DTexture::_createTextureObjectFromFile(LData *data, unsigned int mipLevels, D3DFORMAT, unsigned int usage,
                                              D3DPOOL pool)
{
	if (data)
		FHCreateTextureFromFileInMemory(_context->device(), data->bytes(), data->length(), mipLevels, usage, pool,
		                                (Texture2DEmu **)&_texture);
}

void C2DTexture::_createTextureObjectWithInternalProvider(D3DFORMAT format, unsigned int usage, D3DPOOL pool)
{
	C2DTextureDataSource *source = static_cast<C2DTextureDataSource *>(_dataSource);
	_info.Width = source->widthOfTexture(this, _context);
	_info.Height = source->heightOfTexture(this, _context);
	_info.ResourceType = D3DRTYPE_TEXTURE;
	_info.MipLevels = source->mipmapsCountForTexture(this, _context);
	HRESULT hr = _context->device()->CreateTexture(_info.Width, _info.Height, _info.MipLevels, usage, format, pool,
	                                               (Texture2DEmu **)&_texture);
	if (hr < 0)
		LF_RAISE(FailedWithCodeException(hr));
}

// ---------------------------------------------------------------------------
// CVolumeTexture

void CVolumeTexture::_createTextureObjectWithInternalProvider(D3DFORMAT format, unsigned int usage, D3DPOOL pool)
{
	CVolumeTextureDataSource *source = static_cast<CVolumeTextureDataSource *>(_dataSource);
	_info.Width = source->widthOfTexture(this, _context);
	_info.Height = source->heightOfTexture(this, _context);
	_info.Depth = source->depthOfTexture(this, _context);
	_info.ResourceType = D3DRTYPE_VOLUMETEXTURE;
	_info.MipLevels = source->mipmapsCountForTexture(this, _context);
	HRESULT hr = _context->device()->CreateVolumeTexture(_info.Width, _info.Height, _info.Depth, _info.MipLevels,
	                                                     usage, format, pool, (VolumeTextureEmu **)&_texture);
	if (hr < 0)
		LF_RAISE(FailedWithCodeException(hr));
}

// ---------------------------------------------------------------------------
// CParametric2DTextureDataSource

C2DTextureDataSource *CParametric2DTextureDataSource::dataSourceWithParams(unsigned int width, unsigned int height,
                                                                           D3DFORMAT format, unsigned int usage,
                                                                           unsigned char noMipmaps)
{
	CParametric2DTextureDataSource *source = _alloc();
	source->_width = width;
	source->_height = height;
	source->_format = format;
	source->_usage = usage;
	source->_noMipmaps = noMipmaps;
	return LAutorelease(source);
}
