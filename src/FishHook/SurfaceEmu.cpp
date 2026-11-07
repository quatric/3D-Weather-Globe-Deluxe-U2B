// SurfaceEmu: a texture level. Locking hands out client memory (the level's
// client storage, or a temporary block) or a mapped pixel buffer; unlocking
// uploads it to the texture.

#include "FHDeviceEmu.h"
#include <stdlib.h>
#include <string.h>

LF_IMPLEMENT_CLASS(SurfaceEmu, LF_CLASSOF(ResourceEmu))

static inline bool isDXTFormat(D3DFORMAT f)
{
	return f == D3DFMT_DXT1 || f == D3DFMT_DXT3 || f == D3DFMT_DXT5;
}

SurfaceEmu::SurfaceEmu()
    : _width(0), _height(0), _format(D3DFMT_UNKNOWN), _multiSample(D3DMULTISAMPLE_NONE), _multiSampleQuality(0),
      _level(0), _target(0), _pbo(0), _sysMem(0), _texture(0), _pitch(0), _lockedBits(0), _lockFlags(0),
      _rowLength(0), _size(0)
{
}

SurfaceEmu *SurfaceEmu::initWithParams(void *sysMem, GLenum target, unsigned int width, unsigned int height,
                                       unsigned int level, D3DFORMAT format, BaseTextureEmu *texture,
                                       unsigned int usage, D3DPOOL pool, DeviceEmu *device)
{
	if (ResourceEmu::initWithParams(usage, pool, device) == 0)
		return 0;
	_target = target;
	_width = width;
	_format = format;
	_height = height;
	_texture = texture;
	_sysMem = sysMem;
	_level = level;
	_rowLength = width;
	_size = FHGetSurfaceSize(_width, _height, format);
	// (The original divided by the width, giving a wrong pitch for non-square levels.)
	_pitch = _size / _height;
	if (_format == D3DFMT_DXT1)
		_pitch = _rowLength * 2;
	else if (_format == D3DFMT_DXT3 || _format == D3DFMT_DXT5)
		_pitch = _rowLength * 4;

	glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, sysMem != 0);
	if (_texture->_useTextureRange && _device)
		glTexParameteri(_target, GL_TEXTURE_STORAGE_HINT_APPLE,
		                (_usage & D3DUSAGE_DYNAMIC) ? GL_STORAGE_SHARED_APPLE : GL_STORAGE_CACHED_APPLE);

	// The level starts all white; compressed levels need data to be created.
	void *data = sysMem;
	StPointerBlock *temp = 0;
	if (isDXTFormat(_format)) {
		temp = new StPointerBlock(_size, false);
		data = temp->bytes();
	}
	if (data)
		memset(data, 0xff, _size);
	GLenum internalFormat = BaseTextureEmu::internalTextureFormatFromD3DFormat(_format);
	if (isDXTFormat(_format)) {
		glCompressedTexImage2DARB(_target, _level, internalFormat, _width < 4 ? 4 : _width,
		                          _height < 4 ? 4 : _height, 0, _size, data);
	} else {
		GLenum srcFormat, srcType;
		BaseTextureEmu::sourceFormatFromD3DFormat(_format, &srcFormat, &srcType);
		glTexImage2D(_target, _level, internalFormat, _width, _height, 0, srcFormat, srcType, data);
	}
	if (sysMem)
		glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, 1);
	delete temp;
	return this;
}

void SurfaceEmu::dealloc()
{
	if (_lockedBits)
		UnlockRect();
	if (_pbo)
		glDeleteBuffers(1, &_pbo);
	ResourceEmu::dealloc();
}

HRESULT SurfaceEmu::LockRect(D3DLOCKED_RECT *locked, const RECT *, unsigned int flags)
{
	if (_device == 0 || !_device->ValidateContext())
		return D3DERR_INVALIDCALL;
	if (_lockedBits)
		return D3DERR_INVALIDCALL;
	_lockFlags = flags;
	if (!_texture->_usePBO) {
		_lockedBits = _sysMem ? _sysMem : malloc(_size);
	} else {
		if (_pbo == 0) {
			glGenBuffers(1, &_pbo);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbo);
			glBufferData(GL_PIXEL_UNPACK_BUFFER, _size, 0, (_usage & D3DUSAGE_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
		}
		GLenum access;
		if (_lockFlags & D3DLOCK_DISCARD)
			access = GL_WRITE_ONLY;
		else if (_lockFlags & D3DLOCK_READONLY)
			access = GL_READ_ONLY;
		else
			access = GL_READ_WRITE;
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbo);
		_lockedBits = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, access);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}
	locked->Pitch = _pitch;
	locked->pBits = _lockedBits;
	return D3D_OK;
}

HRESULT SurfaceEmu::UnlockRect()
{
	if (_lockedBits == 0 || _texture == 0)
		return D3DERR_INVALIDCALL;
	if (_texture->_usePBO) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbo);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		_lockedBits = 0;
	}
	if (!(_lockFlags & D3DLOCK_READONLY))
		_texture->_setImageData(this);
	if (!_texture->_usePBO) {
		if (_lockedBits && _sysMem == 0)
			free(_lockedBits);
		_lockedBits = 0;
	}
	_lockFlags = 0;
	return D3D_OK;
}

HRESULT SurfaceEmu::GetDesc(D3DSURFACE_DESC *desc)
{
	if (desc) {
		desc->Width = _width;
		desc->Height = _height;
		desc->Format = _format;
		desc->MultiSampleQuality = _multiSampleQuality;
		desc->MultiSampleType = _multiSample;
		desc->Type = D3DRTYPE_SURFACE;
		desc->Pool = _pool;
		desc->Usage = _usage;
	}
	return D3D_OK;
}
