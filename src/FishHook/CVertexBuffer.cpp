// CVertexBuffer and CIndexBuffer: device buffers a data source fills.

#include "FHModels.h"

LF_IMPLEMENT_CLASS(CVertexBuffer, LF_CLASSOF(CContextObject))
LF_IMPLEMENT_CLASS(CIndexBuffer, LF_CLASSOF(CContextObject))

// ---------------------------------------------------------------------------
// CVertexBuffer

CVertexBuffer::CVertexBuffer() : CContextObject(7), _vb(0), _dsOwner(0), _ds(0), _stride(0), _count(0), _flag(false)
{
}

CVertexBuffer *CVertexBuffer::initWithDataSource(IVertexBufferDataSource *source, CHolder *holder,
                                                 unsigned char retainSource, unsigned char flag)
{
	if (initWithHolder(holder) == 0)
		return 0;
	_flag = flag;
	_ds = source;
	if (retainSource) {
		_dsOwner = source ? dynamic_cast<LObject *>(source) : 0;
		if (_dsOwner)
			_dsOwner->retain();
	}
	return this;
}

CVertexBuffer *CVertexBuffer::initWithDeviceBuffer(VertexBufferEmu *vb, unsigned int stride)
{
	if (initWithHolder(0) == 0)
		return 0;
	_vb = vb;
	_stride = stride;
	D3DVERTEXBUFFER_DESC desc;
	vb->GetDesc(&desc);
	_count = desc.Size / _stride;
	return this;
}

void CVertexBuffer::dealloc()
{
	LRelease(_dsOwner);
	_dsOwner = 0;
	if (_vb) {
		_vb->Release();
		_vb = 0;
	}
	CContextObject::dealloc();
}

void CVertexBuffer::dataSourceWillDie()
{
	LRelease(_dsOwner);
	_dsOwner = 0;
	_ds = 0;
}

// Software-processed and dynamic buffers live in the default pool and are
// rebuilt around a context reset; the others are managed.
void CVertexBuffer::instantiateSelf()
{
	if (_ds) {
		DeviceEmu *device = _context->_device;
		LF_CHECK_PTR(device);
		CShader *shader = _ds->renderShaderForBuffer(this);
		if (shader) {
			unsigned int stream = _ds->streamNumberForBuffer(this);
			bool software = shader->_useSWVP || _ds->isSoftwareVertexBuffer(this);
			unsigned int usage = software ? D3DUSAGE_SOFTWAREPROCESSING : 0;
			bool dynamic = shader->_dynamicBuffer[stream & 0xffff];
			if (dynamic)
				usage |= D3DUSAGE_DYNAMIC;
			_stride = FHGetDeclVertexSize(shader->_decl, stream);
			_count = _ds->vertCountForBuffer(this);
			_managesSelf = dynamic || software;
			HRESULT hr = device->CreateVertexBuffer(_count * _stride, usage, 0,
			                                        !dynamic && !software ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT, &_vb);
			if (hr < 0)
				LF_RAISE(FailedWithCodeException(hr));
			_ds->fillBuffer(this);
			return;
		}
	}
	releaseContextDependedPart();
}

void CVertexBuffer::releaseSelf()
{
	if (_vb) {
		_vb->Release();
		_vb = 0;
	}
	_stride = 0;
}

StVertexBufferLocker *CVertexBuffer::detachLocker(unsigned int flags)
{
	return _vb ? new StVertexBufferLocker(_vb, flags) : 0;
}

// ---------------------------------------------------------------------------
// CIndexBuffer

CIndexBuffer::CIndexBuffer()
    : CContextObject(8), _ib(0), _dsOwner(0), _ds(0), _primType(D3DPT_TRIANGLELIST), _primCount(0), _is32bit(false),
      _flag(false)
{
}

CIndexBuffer *CIndexBuffer::initWithDataSource(IIndexBufferDataSource *source, D3DPRIMITIVETYPE type, CHolder *holder,
                                               unsigned char retainSource, unsigned char flag)
{
	if (initWithHolder(holder) == 0)
		return 0;
	_primType = type;
	_ds = source;
	_flag = flag;
	if (retainSource) {
		_dsOwner = source ? dynamic_cast<LObject *>(source) : 0;
		if (_dsOwner)
			_dsOwner->retain();
	}
	return this;
}

CIndexBuffer *CIndexBuffer::initWithDeviceBuffer(IndexBufferEmu *ib, D3DPRIMITIVETYPE type)
{
	if (initWithHolder(0) == 0)
		return 0;
	_ib = ib;
	_primType = type;
	D3DINDEXBUFFER_DESC desc;
	ib->GetDesc(&desc);
	_is32bit = desc.Format == D3DFMT_INDEX32;
	_primCount = (desc.Size >> (_is32bit ? 2 : 1)) / CBuffersOwner::numVertsForPrimitive(_primType);
	return this;
}

void CIndexBuffer::dealloc()
{
	LRelease(_dsOwner);
	_dsOwner = 0;
	if (_ib) {
		_ib->Release();
		_ib = 0;
	}
	CContextObject::dealloc();
}

void CIndexBuffer::dataSourceWillDie()
{
	LRelease(_dsOwner);
	_dsOwner = 0;
	_ds = 0;
}

void CIndexBuffer::instantiateSelf()
{
	if (_ds == 0) {
		releaseContextDependedPart();
		return;
	}
	DeviceEmu *device = _context->_device;
	LF_CHECK_PTR(device);
	CShader *shader = _ds->renderShaderForBuffer(this);
	bool software = shader->_useSWVP;
	unsigned int usage = software ? D3DUSAGE_SOFTWAREPROCESSING : 0;
	unsigned int count = _ds->indsCountForBuffer(this);
	_primCount = count / CBuffersOwner::numVertsForPrimitive(_primType);
	_is32bit = _ds->use32BitsForBuffer(this);
	_managesSelf = software;
	HRESULT hr = device->CreateIndexBuffer(count << (_is32bit ? 2 : 1), usage,
	                                       _is32bit ? D3DFMT_INDEX32 : D3DFMT_INDEX16,
	                                       software ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &_ib);
	if (hr < 0)
		LF_RAISE(FailedWithCodeException(hr));
	_ds->fillBuffer(this);
}

void CIndexBuffer::releaseSelf()
{
	if (_ib) {
		_ib->Release();
		_ib = 0;
	}
}

StIndexBufferLocker *CIndexBuffer::detachLocker(unsigned int flags)
{
	return _ib ? new StIndexBufferLocker(_ib, flags) : 0;
}
