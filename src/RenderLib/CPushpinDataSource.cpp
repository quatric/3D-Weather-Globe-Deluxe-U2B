// CPushpinDataSource: the pushpin model's buffers, filled from an .ase plist.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CPushpinDataSource, 0)

CPushpinDataSource::CPushpinDataSource() : _faces(0), _vertices(0), _vb(0), _ib(0), _profile(0) {}

CPushpinDataSource *CPushpinDataSource::initWithASEFileAndRenderProfile(LString *path, CRenderProfile *profile)
{
	if (profile == 0 || !parseASEFile(path, 1, 0, &_vertices, &_faces)) {
		dealloc();
		return 0;
	}
	_profile = LRetain(profile);
	return this;
}

void CPushpinDataSource::dealloc()
{
	releaseAndClear(_profile);
	releaseAndClear(_faces);
	releaseAndClear(_vertices);
	if (_vb) {
		_vb->dataSourceWillDie();
		releaseAndClear(_vb);
	}
	if (_ib) {
		_ib->dataSourceWillDie();
		releaseAndClear(_ib);
	}
	LObject::dealloc();
}

CShader *CPushpinDataSource::renderShaderForBuffer(CVertexBuffer *)
{
	return _profile->_shader;
}

CShader *CPushpinDataSource::renderShaderForBuffer(CIndexBuffer *)
{
	return _profile->_shader;
}

unsigned int CPushpinDataSource::vertCountForBuffer(CVertexBuffer *)
{
	return _vertices->count();
}

bool CPushpinDataSource::use32BitsForBuffer(CIndexBuffer *)
{
	return _vertices->count() > 0xffff;
}

unsigned int CPushpinDataSource::indsCountForBuffer(CIndexBuffer *)
{
	return lobj_cast<LData>(_faces->objectAtIndex(0))->length() / 4;
}

// A vertex is six floats, the position and the normal, both stored z, y, x.
void CPushpinDataSource::fillBuffer(CVertexBuffer *vb)
{
	StVertexBufferLocker *locker = vb->detachLocker(0);
	char *p = (char *)locker->data();
	int pos = _profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	int normal = _profile->drawableElementOffset(D3DDECLUSAGE_NORMAL, 0, 0);
	if ((pos | normal) < 0)
		LF_RAISE(WrongConditionException());
	unsigned int stride = _profile->vertexStructSize(streamNumberForBuffer(vb));
	int count = _vertices->count();
	for (int i = 0; i < count; i++, p += stride) {
		const float *src = (const float *)lobj_cast<LData>(_vertices->objectAtIndex(i))->bytes();
		float *v = (float *)(p + pos);
		v[0] = src[2];
		v[1] = src[1];
		v[2] = src[0];
		float *n = (float *)(p + normal);
		n[0] = src[5];
		n[1] = src[4];
		n[2] = src[3];
	}
	delete locker;
}

void CPushpinDataSource::fillBuffer(CIndexBuffer *ib)
{
	StIndexBufferLocker *locker = ib->detachLocker(0);
	if (_faces->count() > 1)
		LF_RAISE(WrongConditionException());
	LData *faces = lobj_cast<LData>(_faces->objectAtIndex(0));
	if (faces == 0)
		LF_RAISE(NilPtrException());
	const uint32_t *src = (const uint32_t *)faces->bytes();
	unsigned int count = faces->length() / 4;
	if (locker->_desc.Format == D3DFMT_INDEX32) {
		uint32_t *dst = (uint32_t *)locker->data();
		for (unsigned int i = 0; i < count; i++)
			dst[i] = src[i];
	} else {
		uint16_t *dst = (uint16_t *)locker->data();
		for (unsigned int i = 0; i < count; i++)
			dst[i] = (uint16_t)src[i];
	}
	delete locker;
}

CVertexBuffer *CPushpinDataSource::vertexBufferForBufOwner(int, CBuffersOwner *owner)
{
	if (_vb == 0) {
		CHolder *holder = owner->_holder;
		if (holder)
			holder->beginAccumulationBlock();
		_vb = CVertexBuffer::_alloc()->initWithDataSource(this, owner->_holder, 0, 1);
		if (holder)
			holder->endAccumulationBlock();
	}
	return _vb;
}

CIndexBuffer *CPushpinDataSource::indexBufferForBufOwner(CBuffersOwner *owner)
{
	if (_ib == 0) {
		CHolder *holder = owner->_holder;
		if (holder)
			holder->beginAccumulationBlock();
		_ib = CIndexBuffer::_alloc()->initWithDataSource(this, D3DPT_TRIANGLELIST, owner->_holder, 0, 1);
		if (holder)
			holder->endAccumulationBlock();
	}
	return _ib;
}
