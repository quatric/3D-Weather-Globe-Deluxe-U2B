// VertexDeclEmu, DeviceShaderEmu, DXMeshEmu, StateBlockEmu and the resource
// lockers.

#include "FHDeviceEmu.h"
#include <string.h>

LF_IMPLEMENT_CLASS(VertexDeclEmu, LF_CLASSOF(IUnknownEmu))
LF_IMPLEMENT_CLASS(DeviceShaderEmu, LF_CLASSOF(IUnknownEmu))
LF_IMPLEMENT_CLASS(DXMeshEmu, LF_CLASSOF(IUnknownEmu))
LF_IMPLEMENT_CLASS(StateBlockEmu, LF_CLASSOF(IUnknownEmu))

VertexDeclEmu *VertexDeclEmu::initWithParams(const D3DVERTEXELEMENT9 *elements, DeviceEmu *device)
{
	if (LObject::init() == 0)
		return 0;
	_device = device;
	int n = 0;
	while (n <= MAXD3DDECLLENGTH && elements[n].Type != D3DDECLTYPE_UNUSED)
		n++;
	// The elements and the end marker.
	_elements.assign(elements, elements + n + 1);
	return this;
}

// ---------------------------------------------------------------------------

DeviceShaderEmu *DeviceShaderEmu::initWithParams(LString *source, GLenum target, DeviceEmu *device)
{
	if (LObject::init() == 0)
		return 0;
	_target = target;
	_device = device;
	if (_target == 0) {
		dealloc();
		return 0;
	}
	if (_program)
		glDeleteProgramsARB(1, &_program);
	glGenProgramsARB(1, &_program);
	glBindProgramARB(_target, _program);
	const char *s = source->cString();
	glProgramStringARB(_target, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)strlen(s), s);
	return this;
}

void DeviceShaderEmu::dealloc()
{
	if (_device)
		_device->ValidateContext();
	glDeleteProgramsARB(1, &_program);
	LObject::dealloc();
}

GLenum DeviceShaderEmu::bind()
{
	glBindProgramARB(_target, _program);
	glEnable(_target);
	return _target;
}

// ---------------------------------------------------------------------------

DXMeshEmu *DXMeshEmu::initWithParams(DeviceEmu *device, const D3DVERTEXELEMENT9 *decl, unsigned int numVertices,
                                     unsigned int numFaces, unsigned int options)
{
	if (LObject::init() == 0)
		return 0;
	_numVertices = numVertices;
	_numFaces = numFaces;
	unsigned int vertexSize = FHGetDeclVertexSize(decl, 0);

	D3DPOOL pool = (options & D3DXMESH_VB_SYSTEMMEM) ? D3DPOOL_SYSTEMMEM
	               : (options & D3DXMESH_VB_MANAGED) ? D3DPOOL_MANAGED
	                                                  : D3DPOOL_DEFAULT;
	unsigned int usage = (options & D3DXMESH_VB_WRITEONLY) ? D3DUSAGE_WRITEONLY : 0;
	if (options & D3DXMESH_VB_DYNAMIC)
		usage |= D3DUSAGE_DYNAMIC;
	if (options & D3DXMESH_VB_SOFTWARE)
		usage |= D3DUSAGE_SOFTWAREPROCESSING;
	if (device->CreateVertexBuffer(vertexSize * _numVertices, usage, 0, pool, &_vb) >= 0) {
		unsigned int indexSize = (options & D3DXMESH_32BIT) ? 4 : 2;
		D3DFORMAT format = (options & D3DXMESH_32BIT) ? D3DFMT_INDEX32 : D3DFMT_INDEX16;
		pool = (options & D3DXMESH_IB_SYSTEMMEM) ? D3DPOOL_SYSTEMMEM
		       : (options & D3DXMESH_IB_MANAGED) ? D3DPOOL_MANAGED
		                                          : D3DPOOL_DEFAULT;
		usage = (options & D3DXMESH_IB_WRITEONLY) ? D3DUSAGE_WRITEONLY : 0;
		if (options & D3DXMESH_IB_DYNAMIC)
			usage |= D3DUSAGE_DYNAMIC;
		if (options & D3DXMESH_IB_SOFTWARE)
			usage |= D3DUSAGE_SOFTWAREPROCESSING;
		if (device->CreateIndexBuffer(indexSize * _numFaces * 3, usage, format, pool, &_ib) >= 0)
			return this;
	}
	dealloc();
	return 0;
}

void DXMeshEmu::dealloc()
{
	FHSafeRelease(_vb);
	FHSafeRelease(_ib);
	LObject::dealloc();
}

HRESULT DXMeshEmu::GetVertexBuffer(VertexBufferEmu **vb)
{
	*vb = _vb;
	if (_vb)
		_vb->AddRef();
	return D3D_OK;
}

HRESULT DXMeshEmu::GetIndexBuffer(IndexBufferEmu **ib)
{
	*ib = _ib;
	if (_ib)
		_ib->AddRef();
	return D3D_OK;
}

// ---------------------------------------------------------------------------

StateBlockEmu::StateBlockEmu()
    : _device(0), _indicesSet(false), _declSet(false), _vsSet(false), _psSet(false), _indices(0), _vs(0), _ps(0),
      _decl(0)
{
	memset(_streams, 0, sizeof(_streams));
	memset(_textures, 0, sizeof(_textures));
}

StateBlockEmu *StateBlockEmu::initWithParams(DeviceEmu *device)
{
	if (LObject::init() == 0)
		return 0;
	_device = device;
	return this;
}

void StateBlockEmu::dealloc()
{
	for (int i = 0; i < 8; i++)
		FHSafeRelease(_streams[i]);
	for (int i = 0; i < 8; i++)
		FHSafeRelease(_textures[i]);
	FHSafeRelease(_indices);
	FHSafeRelease(_decl);
	FHSafeRelease(_vs);
	FHSafeRelease(_ps);
	_items.clear();
	_data.clear();
	LObject::dealloc();
}

HRESULT StateBlockEmu::Apply()
{
	if (_device->isRecordingStateBlock())
		return D3DERR_INVALIDCALL;
	if (_indicesSet)
		_device->SetIndices(_indices);
	if (_vsSet)
		_device->SetVertexShader(_vs);
	if (_psSet)
		_device->SetPixelShader(_ps);
	if (_declSet)
		_device->SetVertexDeclaration(_decl);
	for (size_t i = 0; i < _items.size(); i++) {
		const SStateItemDescr &d = _items[i];
		switch (d.type) {
		case kRenderState:
			_device->SetRenderState((D3DRENDERSTATETYPE)d.a, d.b);
			break;
		case kTextureStageState:
			_device->SetTextureStageState(d.a, (D3DTEXTURESTAGESTATETYPE)d.b, d.c);
			break;
		case kSamplerState:
			_device->SetSamplerState(d.a, (D3DSAMPLERSTATETYPE)d.b, d.c);
			break;
		case kTexture:
			_device->SetTexture(d.a, _textures[d.a]);
			break;
		case kStreamSource:
			_device->SetStreamSource(d.a, _streams[d.a], d.b, d.c);
			break;
		case kTransform:
			_device->SetTransform((D3DTRANSFORMSTATETYPE)d.a, (const _SMatrix *)&_data[d.b]);
			break;
		case kVSConstant:
			_device->SetVertexShaderConstantF(d.a, (const float *)&_data[d.b], d.c);
			break;
		case kPSConstant:
			_device->SetPixelShaderConstantF(d.a, (const float *)&_data[d.b], d.c);
			break;
		}
	}
	return D3D_OK;
}

unsigned int StateBlockEmu::_appendData(const void *data, unsigned int size)
{
	unsigned int offset = (unsigned int)_data.size();
	_data.insert(_data.end(), (const char *)data, (const char *)data + size);
	return offset;
}

void StateBlockEmu::setTexture(unsigned int stage, BaseTextureEmu *texture)
{
	FHAssign(_textures[stage], texture);
	_add(kTexture, stage, 0, 0);
}

void StateBlockEmu::setStreamSource(unsigned int stream, VertexBufferEmu *vb, unsigned int offset,
                                    unsigned int stride)
{
	FHAssign(_streams[stream], vb);
	_add(kStreamSource, stream, offset, stride);
}

void StateBlockEmu::setTransform(D3DTRANSFORMSTATETYPE type, const _SMatrix *m)
{
	_add(kTransform, type, _appendData(m, sizeof(_SMatrix)), 0);
}

void StateBlockEmu::setVertexShaderConstantF(unsigned int start, const float *data, unsigned int count)
{
	_add(kVSConstant, start, _appendData(data, count * 16), count);
}

void StateBlockEmu::setPixelShaderConstantF(unsigned int start, const float *data, unsigned int count)
{
	_add(kPSConstant, start, _appendData(data, count * 16), count);
}

void StateBlockEmu::setIndices(IndexBufferEmu *ib)
{
	FHAssign(_indices, ib);
	_indicesSet = true;
}

void StateBlockEmu::setVertexDeclaration(VertexDeclEmu *decl)
{
	FHAssign(_decl, decl);
	_declSet = true;
}

void StateBlockEmu::setVertexShader(DeviceShaderEmu *shader)
{
	FHAssign(_vs, shader);
	_vsSet = true;
}

void StateBlockEmu::setPixelShader(DeviceShaderEmu *shader)
{
	FHAssign(_ps, shader);
	_psSet = true;
}

// ---------------------------------------------------------------------------

StSurfaceLocker::StSurfaceLocker(SurfaceEmu *surface, unsigned int flags) : _surface(surface), _bytesPerPixel(4)
{
	memset(&_desc, 0, sizeof(_desc));
	memset(&_locked, 0, sizeof(_locked));
	if (_surface) {
		_surface->GetDesc(&_desc);
		_surface->LockRect(&_locked, 0, flags);
		_bytesPerPixel = (_desc.Format >= D3DFMT_R5G6B5 && _desc.Format <= D3DFMT_A4R4G4B4) ? 2 : 4;
	}
}

StSurfaceLocker::~StSurfaceLocker()
{
	if (_surface)
		_surface->UnlockRect();
}

StVertexBufferLocker::StVertexBufferLocker(VertexBufferEmu *vb, unsigned int flags)
{
	if (vb)
		vb->AddRef();
	init(vb, flags);
}

StVertexBufferLocker::StVertexBufferLocker(DXMeshEmu *mesh, unsigned int flags)
{
	mesh->GetVertexBuffer(&_vb);
	init(_vb, flags);
}

void StVertexBufferLocker::init(VertexBufferEmu *vb, unsigned int flags)
{
	_data = 0;
	_vb = vb;
	memset(&_desc, 0, sizeof(_desc));
	if (_vb) {
		_vb->Lock(0, 0, &_data, flags);
		_vb->GetDesc(&_desc);
	}
}

StVertexBufferLocker::~StVertexBufferLocker()
{
	if (_vb) {
		_vb->Unlock();
		FHSafeRelease(_vb);
	}
}

StIndexBufferLocker::StIndexBufferLocker(IndexBufferEmu *ib, unsigned int flags)
{
	if (ib)
		ib->AddRef();
	init(ib, flags);
}

StIndexBufferLocker::StIndexBufferLocker(DXMeshEmu *mesh, unsigned int flags)
{
	mesh->GetIndexBuffer(&_ib);
	init(_ib, flags);
}

void StIndexBufferLocker::init(IndexBufferEmu *ib, unsigned int flags)
{
	_data = 0;
	_ib = ib;
	memset(&_desc, 0, sizeof(_desc));
	if (_ib) {
		HRESULT hr = _ib->Lock(0, 0, &_data, flags);
		if (hr < 0)
			LF_RAISE(FailedWithCodeException(hr));
		_ib->GetDesc(&_desc);
	}
}

StIndexBufferLocker::~StIndexBufferLocker()
{
	if (_ib) {
		_ib->Unlock();
		FHSafeRelease(_ib);
	}
}
