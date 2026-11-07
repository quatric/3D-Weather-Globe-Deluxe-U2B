// BaseTextureEmu, Texture2DEmu, CubicTextureEmu, VolumeTextureEmu: D3D
// textures on GL textures. The GL texture and its level surfaces are created
// on the first GetSurfaceLevel; each surface allocates its level with an
// empty image and uploads its pixels when unlocked.

#include "FHDeviceEmu.h"
#include "FHContext.h"
#include <stdlib.h>

LF_IMPLEMENT_CLASS(BaseTextureEmu, LF_CLASSOF(ResourceEmu))
LF_IMPLEMENT_CLASS(Texture2DEmu, LF_CLASSOF(BaseTextureEmu))
LF_IMPLEMENT_CLASS(CubicTextureEmu, LF_CLASSOF(BaseTextureEmu))
LF_IMPLEMENT_CLASS(VolumeTextureEmu, LF_CLASSOF(BaseTextureEmu))

static inline bool isDXTFormat(D3DFORMAT f)
{
	return f == D3DFMT_DXT1 || f == D3DFMT_DXT3 || f == D3DFMT_DXT5;
}

BaseTextureEmu::BaseTextureEmu()
    : _name(0), _target(0), _format(D3DFMT_UNKNOWN), _width(0), _height(0), _levels(0), _surfaces(0),
      _sysMemBuffer(0), _sysMemSize(0), _useTextureRange(false), _keepSysMem(false), _usePBO(false),
      _canMipmap(true)
{
}

BaseTextureEmu *BaseTextureEmu::initWithParams(GLenum target, unsigned int width, unsigned int height,
                                               D3DFORMAT format, unsigned int levels, unsigned int usage,
                                               D3DPOOL pool, DeviceEmu *device)
{
	if (ResourceEmu::initWithParams(usage, pool, device) == 0)
		return 0;
	if (_device && !_device->context()->checkDeviceFormat(format, usage, GetType())) {
		dealloc();
		return 0;
	}
	_target = target;
	_width = width;
	_height = height;
	_format = format;
	_levels = levels;
	if ((width & (width - 1)) == 0 && (height & (height - 1)) == 0) {
		// A full mip chain when no level count is given.
		if (levels == 0) {
			unsigned int n = width > height ? width : height;
			for (_levels = 0; n != 0; n >>= 1)
				_levels++;
		}
	} else {
		_levels = 1;
	}
	_keepSysMem = false;
	_usePBO = _device->contextCaps()->PBOsAreSupported();
	// (The original checked for Mac OS X 10.5.1 or later before these.)
	if (format == D3DFMT_L8 || format == D3DFMT_A8)
		_usePBO = false;
	if (format == D3DFMT_DXT1 || format == D3DFMT_DXT2 || format == D3DFMT_DXT3 || format == D3DFMT_DXT4 ||
	    format == D3DFMT_DXT5)
		_canMipmap = false;
	_keepSysMem = !_usePBO && ((_usage & D3DUSAGE_DYNAMIC) || (_usage & D3DUSAGE_SOFTWAREPROCESSING));
	if (isDXTFormat(format))
		_keepSysMem = false;
	if (!_canMipmap)
		_levels = 1;
	return this;
}

void BaseTextureEmu::bind(unsigned int stage)
{
	glActiveTextureARB(GL_TEXTURE0_ARB + stage);
	glBindTexture(_target, _name);
	if (_name != 0)
		glEnable(_target);
	else
		glDisable(_target);
}

void BaseTextureEmu::unbindAll(unsigned int stage)
{
	glActiveTextureARB(GL_TEXTURE0_ARB + stage);
	glDisable(GL_TEXTURE_2D);
}

GLenum BaseTextureEmu::internalTextureFormatFromD3DFormat(D3DFORMAT format)
{
	switch (format) {
	case D3DFMT_R8G8B8:
	case D3DFMT_X8R8G8B8:
		return GL_RGB8;
	case D3DFMT_A8R8G8B8:
		return GL_RGBA8;
	case D3DFMT_R5G6B5:
	case D3DFMT_X1R5G5B5:
		return GL_RGB5;
	case D3DFMT_A1R5G5B5:
		return GL_RGB5_A1;
	case D3DFMT_A4R4G4B4:
		return GL_RGBA4;
	case D3DFMT_A8:
		return GL_ALPHA8;
	case D3DFMT_X4R4G4B4:
		return GL_RGB4;
	case D3DFMT_A2B10G10R10:
	case D3DFMT_A2R10G10B10:
		return GL_RGB10_A2;
	case D3DFMT_A16B16G16R16:
		return GL_RGBA16;
	case D3DFMT_L8:
		return GL_LUMINANCE8;
	case D3DFMT_A8L8:
		return GL_LUMINANCE8_ALPHA8;
	case D3DFMT_DXT1:
		return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
	case D3DFMT_DXT3:
		return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	case D3DFMT_DXT5:
		return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	default:
		return 0;
	}
}

void BaseTextureEmu::sourceFormatFromD3DFormat(D3DFORMAT format, GLenum *srcFormat, GLenum *srcType)
{
	*srcFormat = 0;
	*srcType = 0;
	switch (format) {
	case D3DFMT_R8G8B8:
		*srcFormat = GL_BGR;
		*srcType = GL_UNSIGNED_BYTE;
		break;
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
		*srcFormat = GL_BGRA;
		*srcType = GL_UNSIGNED_BYTE;
		break;
	case D3DFMT_R5G6B5:	// (as the original: uploaded as 1-5-5-5)
	case D3DFMT_X1R5G5B5:
	case D3DFMT_A1R5G5B5:
		*srcFormat = GL_BGRA;
		*srcType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		break;
	case D3DFMT_A4R4G4B4:
	case D3DFMT_X4R4G4B4:
		*srcFormat = GL_BGRA;
		*srcType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
		break;
	case D3DFMT_A8:
		*srcFormat = GL_ALPHA;
		*srcType = GL_UNSIGNED_BYTE;
		break;
	case D3DFMT_L8:
		*srcFormat = GL_LUMINANCE;
		*srcType = GL_UNSIGNED_BYTE;
		break;
	case D3DFMT_A8L8:
		*srcFormat = GL_LUMINANCE_ALPHA;
		*srcType = GL_UNSIGNED_BYTE;
		break;
	case D3DFMT_A2R10G10B10:
		*srcFormat = GL_BGRA;
		*srcType = GL_UNSIGNED_INT_2_10_10_10_REV;
		break;
	case D3DFMT_A16B16G16R16:
		*srcFormat = GL_BGRA;
		*srcType = GL_UNSIGNED_SHORT;
		break;
	case D3DFMT_DXT3:
	case D3DFMT_DXT5:
		*srcFormat = GL_RGBA;
		*srcType = GL_UNSIGNED_BYTE;
		break;
	case D3DFMT_DXT1:
		*srcFormat = GL_RGB;
		*srcType = GL_UNSIGNED_BYTE;
		break;
	default:
		break;
	}
}

void BaseTextureEmu::_setImageData(SurfaceEmu *surface)
{
	unsigned int width = surface->_width, height = surface->_height;
	GLenum srcFormat, srcType;
	sourceFormatFromD3DFormat(surface->_format, &srcFormat, &srcType);
	glPushAttrib(GL_TEXTURE_BIT);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(_target, _name);
	_device->_txtUnit0WasChanged();
	glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(_target, GL_GENERATE_MIPMAP, 0);
	glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (_usePBO)
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, surface->_pbo);
	// With a PBO bound, the "pointer" is the offset into it (0).
	const void *pixels = _usePBO ? 0 : surface->_lockedBits;
	if (!isDXTFormat(_format)) {
		glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->_rowLength);
		glTexSubImage2D(surface->_target, surface->_level, 0, 0, width, height, srcFormat, srcType, pixels);
	} else {
		glCompressedTexSubImage2DARB(surface->_target, surface->_level, 0, 0, width < 4 ? 4 : width,
		                             height < 4 ? 4 : height, internalTextureFormatFromD3DFormat(_format),
		                             surface->_size, pixels);
	}
	if (_usePBO)
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glPopClientAttrib();
	glPopAttrib();
}

void BaseTextureEmu::dealloc()
{
	if (_device)
		_device->ValidateContext();
	if (_surfaces) {
		for (unsigned int i = 0; i < _surfaces->count(); i++)
			_surfaces->objectAtIndex(i)->_texture = 0;
		_surfaces->release();
		_surfaces = 0;
	}
	if (_name != 0)
		glDeleteTextures(1, &_name);
	if (_sysMemBuffer) {
		glFlush();
		free(_sysMemBuffer);
		_sysMemBuffer = 0;
	}
	ResourceEmu::dealloc();
}

// ---------------------------------------------------------------------------

void Texture2DEmu::_createSurfacesIfNeeded()
{
	if (_surfaces)
		return;
	glGenTextures(1, &_name);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(_target, _name);
	_device->_txtUnit0WasChanged();
	glPushAttrib(GL_TEXTURE_BIT);
	glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(_target, GL_GENERATE_MIPMAP, 0);
	_surfaces = (TLArray<SurfaceEmu *> *)LArray::_alloc()->init();
	_surfaces->preserveCapacity(_levels);
	unsigned int w, h;
	if (_keepSysMem) {
		// One block of client storage for all the levels.
		w = _width;
		h = _height;
		for (unsigned int i = 0; i < _levels; i++) {
			_sysMemSize += FHGetSurfaceSize(w, h, _format);
			w = w >> 1 ? w >> 1 : 1;
			h = h >> 1 ? h >> 1 : 1;
		}
		_sysMemBuffer = malloc(_sysMemSize);
	}
	unsigned int offset = 0;
	w = _width;
	h = _height;
	for (unsigned int level = 0; level < _levels; level++) {
		void *sysMem = _sysMemBuffer ? (char *)_sysMemBuffer + offset : 0;
		SurfaceEmu *s = SurfaceEmu::_alloc()->initWithParams(sysMem, _target, w, h, level, _format, this,
		                                                     _usage, _pool, _device);
		_surfaces->addObject(s);
		s->release();
		offset += FHGetSurfaceSize(w, h, _format);
		w = w >> 1 ? w >> 1 : 1;
		h = h >> 1 ? h >> 1 : 1;
	}
	glPopAttrib();
}

HRESULT Texture2DEmu::GetSurfaceLevel(unsigned int level, SurfaceEmu **surface)
{
	*surface = 0;
	if (level >= _levels)
		return D3DERR_INVALIDCALL;
	if (_surfaces == 0) {
		if (_device == 0 || !_device->ValidateContext())
			return D3DERR_INVALIDCALL;
		_createSurfacesIfNeeded();
	}
	*surface = _surfaces->objectAtIndex(level);
	(*surface)->AddRef();
	return D3D_OK;
}

HRESULT Texture2DEmu::LockRect(unsigned int level, D3DLOCKED_RECT *locked, const RECT *rect, unsigned int flags)
{
	SurfaceEmu *s;
	HRESULT hr = GetSurfaceLevel(level, &s);
	if (hr >= 0) {
		hr = s->LockRect(locked, rect, flags);
		s->Release();
	}
	return hr;
}

HRESULT Texture2DEmu::UnlockRect(unsigned int level)
{
	SurfaceEmu *s;
	HRESULT hr = GetSurfaceLevel(level, &s);
	if (hr >= 0) {
		s->UnlockRect();
		s->Release();
		hr = D3D_OK;
	}
	return hr;
}

HRESULT Texture2DEmu::GetLevelDesc(unsigned int level, D3DSURFACE_DESC *desc)
{
	SurfaceEmu *s;
	HRESULT hr = GetSurfaceLevel(level, &s);
	if (hr >= 0) {
		if (desc)
			s->GetDesc(desc);
		s->Release();
		hr = D3D_OK;
	}
	return hr;
}

// ---------------------------------------------------------------------------

static const GLenum _faces[6] = {
	GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};

void CubicTextureEmu::_createSurfacesIfNeeded()
{
	if (_surfaces)
		return;
	if (_device)
		_device->ValidateContext();
	// (The original never created the GL texture of a cube map; nothing makes one.)
	glGenTextures(1, &_name);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(_target, _name);
	_device->_txtUnit0WasChanged();
	_surfaces = (TLArray<SurfaceEmu *> *)LArray::_alloc()->init();
	_surfaces->preserveCapacity(_levels * 6);
	unsigned int w = _width, h = _height;
	for (unsigned int level = 0; level < _levels; level++) {
		for (int f = 0; f < 6; f++) {
			SurfaceEmu *s = SurfaceEmu::_alloc()->initWithParams(0, _faces[f], w, h, level, _format, this,
			                                                     _usage, _pool, _device);
			_surfaces->addObject(s);
			s->release();
		}
		w = w >> 1 ? w >> 1 : 1;
		h = h >> 1 ? h >> 1 : 1;
	}
}

HRESULT CubicTextureEmu::GetCubeMapSurface(D3DCUBEMAP_FACES face, unsigned int level, SurfaceEmu **surface)
{
	*surface = 0;
	if (level >= _levels)
		return D3DERR_INVALIDCALL;
	_createSurfacesIfNeeded();
	*surface = _surfaces->objectAtIndex(face + level * 6);
	(*surface)->AddRef();
	return D3D_OK;
}

HRESULT CubicTextureEmu::LockRect(D3DCUBEMAP_FACES face, unsigned int level, D3DLOCKED_RECT *locked,
                                  const RECT *rect, unsigned int flags)
{
	SurfaceEmu *s;
	HRESULT hr = GetCubeMapSurface(face, level, &s);
	if (hr >= 0) {
		hr = s->LockRect(locked, rect, flags);
		s->Release();
	}
	return hr;
}

HRESULT CubicTextureEmu::UnlockRect(D3DCUBEMAP_FACES face, unsigned int level)
{
	SurfaceEmu *s;
	HRESULT hr = GetCubeMapSurface(face, level, &s);
	if (hr >= 0) {
		hr = s->UnlockRect();
		s->Release();
	}
	return hr;
}

HRESULT CubicTextureEmu::GetLevelDesc(unsigned int level, D3DSURFACE_DESC *desc)
{
	SurfaceEmu *s;
	HRESULT hr = GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_X, level, &s);
	if (hr >= 0) {
		if (desc)
			s->GetDesc(desc);
		s->Release();
		hr = D3D_OK;
	}
	return hr;
}
