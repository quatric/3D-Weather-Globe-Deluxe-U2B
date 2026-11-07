// VertexBufferEmu, IndexBufferEmu and their storage (GLBufferImp): a VBO when
// the context has them and the buffer isn't meant for system memory, plain
// memory otherwise.

#include "FHDeviceEmu.h"
#include <string.h>

LF_IMPLEMENT_CLASS(VertexBufferEmu, LF_CLASSOF(ResourceEmu))
LF_IMPLEMENT_CLASS(IndexBufferEmu, LF_CLASSOF(ResourceEmu))

void GLBufferImp::unbindAll(bool unbindGL)
{
	if (unbindGL) {
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}
	DeviceEmu *device = DeviceEmu::currentDevice();
	if (device)
		device->_resetCurrentBuffers();
}

GLBufferImpGeneric::GLBufferImpGeneric(LData *data, unsigned int size)
{
	if (data) {
		_data = LRetain(data);
		return;
	}
	_data = LData::_alloc()->init();
	_data->setLength(size);
	memset(_data->bytes(), 0, size);
}

GLBufferImpGeneric::~GLBufferImpGeneric()
{
	LRelease(_data);
	_data = 0;
}

GLBufferImpVBO::GLBufferImpVBO(LData *data, unsigned int size, GLenum target, unsigned int usage)
    : _name(0), _target(target)
{
	glGenBuffersARB(1, &_name);
	glBindBufferARB(_target, _name);
	glBufferDataARB(_target, size, data ? data->bytes() : 0,
	                (usage & D3DUSAGE_DYNAMIC) ? GL_DYNAMIC_DRAW_ARB : GL_STATIC_DRAW_ARB);
}

GLBufferImpVBO::~GLBufferImpVBO()
{
	glDeleteBuffersARB(1, &_name);
	_name = 0;
}

char *GLBufferImpVBO::bind()
{
	glBindBufferARB(_target, _name);
	return 0;
}

void GLBufferImpVBO::unbind()
{
	glBindBufferARB(_target, 0);
}

void *GLBufferImpVBO::lock(unsigned int flags)
{
	glBindBufferARB(_target, _name);
	GLenum access;
	if (flags & D3DLOCK_READONLY)
		access = GL_READ_ONLY_ARB;
	else if (flags & D3DLOCK_DISCARD)
		access = GL_WRITE_ONLY_ARB;
	else
		access = GL_READ_WRITE_ARB;
	return glMapBufferARB(_target, access);
}

void GLBufferImpVBO::unlock()
{
	glBindBufferARB(_target, _name);
	if (!glUnmapBufferARB(_target))
		LLog("buffer data was lost.");
}

// ---------------------------------------------------------------------------

VertexBufferEmu *VertexBufferEmu::initWithParams(unsigned int length, unsigned int usage, D3DPOOL pool,
                                                 DeviceEmu *device)
{
	if (ResourceEmu::initWithParams(usage, pool, device) == 0)
		return 0;
	_length = length;
	_createBuffer();
	return this;
}

void VertexBufferEmu::_createBuffer()
{
	bool sysMem = isInSystemMem();
	if (!_device->contextCaps()->VBOsAreSupported() || sysMem)
		_imp = new GLBufferImpGeneric(0, _length);
	else
		_imp = new GLBufferImpVBO(0, _length, GL_ARRAY_BUFFER_ARB, _usage);
}

void VertexBufferEmu::_deleteBuffer()
{
	_device->ValidateContext();
	delete _imp;
	_imp = 0;
}

void VertexBufferEmu::dealloc()
{
	_deleteBuffer();
	ResourceEmu::dealloc();
}

// (The original ignored the offset, returning the start of the buffer.)
HRESULT VertexBufferEmu::Lock(unsigned int offset, unsigned int, void **data, unsigned int flags)
{
	char *p = (char *)_imp->lock(flags);
	*data = p ? p + offset : 0;
	return D3D_OK;
}

HRESULT VertexBufferEmu::Unlock()
{
	_imp->unlock();
	if (_notifyDevice)
		_device->_vbWasChanged(this);
	return D3D_OK;
}

char *VertexBufferEmu::bind()
{
	if (_device->contextCaps()->VBOsAreSupported())
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	return _imp->bind();
}

HRESULT VertexBufferEmu::GetDesc(D3DVERTEXBUFFER_DESC *desc)
{
	if (_device == 0 || _length == 0)
		return D3DERR_INVALIDCALL;
	memset(desc, 0, sizeof(*desc));
	desc->Pool = _pool;
	desc->Usage = _usage;
	desc->Type = GetType();
	desc->Format = D3DFMT_VERTEXDATA;
	desc->Size = _length;
	return D3D_OK;
}

// ---------------------------------------------------------------------------

IndexBufferEmu *IndexBufferEmu::initWithParams(D3DFORMAT format, unsigned int length, unsigned int usage,
                                               D3DPOOL pool, DeviceEmu *device)
{
	if (ResourceEmu::initWithParams(usage, pool, device) == 0)
		return 0;
	_length = length;
	_format = format;
	_createBuffer();
	return this;
}

void IndexBufferEmu::_createBuffer()
{
	if (!isInSystemMem() && _device->contextCaps()->VBOsAreSupported())
		_imp = new GLBufferImpVBO(0, _length, GL_ELEMENT_ARRAY_BUFFER_ARB, _usage);
	else
		_imp = new GLBufferImpGeneric(0, _length);
}

void IndexBufferEmu::_deleteBuffer()
{
	_device->ValidateContext();
	delete _imp;
	_imp = 0;
}

void IndexBufferEmu::dealloc()
{
	_deleteBuffer();
	ResourceEmu::dealloc();
}

HRESULT IndexBufferEmu::Lock(unsigned int offset, unsigned int, void **data, unsigned int flags)
{
	char *p = (char *)_imp->lock(flags);
	*data = p ? p + offset : 0;
	return D3D_OK;
}

HRESULT IndexBufferEmu::Unlock()
{
	_imp->unlock();
	if (_notifyDevice)
		_device->_ibWasChanged(this);
	return D3D_OK;
}

char *IndexBufferEmu::bind()
{
	if (_device->contextCaps()->VBOsAreSupported())
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	return _imp->bind();
}

HRESULT IndexBufferEmu::GetDesc(D3DINDEXBUFFER_DESC *desc)
{
	if (_device == 0 || _length == 0)
		return D3DERR_INVALIDCALL;
	memset(desc, 0, sizeof(*desc));
	desc->Pool = _pool;
	desc->Usage = _usage;
	desc->Type = GetType();
	desc->Size = _length;
	desc->Format = _format;
	return D3D_OK;
}
