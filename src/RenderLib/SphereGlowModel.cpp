// A glow's mesh: rings of a cone about the z axis.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(SphereGlowModel, LF_CLASSOF(CMesh))

SphereGlowModel::SphereGlowModel() : _slices(0), _rings(0), _radii(0), _texParams(0), _texGenMode(kTexGenRadial)
{
}

void SphereGlowModel::dealloc()
{
	_texParams = 0;
	delete[] _radii;
	_radii = 0;
	CMesh::dealloc();
}

SphereGlowModel *SphereGlowModel::initWithParams(unsigned int slices, unsigned int rings, float *radii, float *texParams,
                                                 TexGenMode mode, CRenderProfile *profile)
{
	if (CMesh::initWithDataSource(this, profile, 0) == 0)
		return 0;
	_rings = rings;
	_slices = slices;
	_radii = new float[rings * 2];
	_texParams = _radii + rings;
	memcpy(_radii, radii, rings * sizeof(float));
	memcpy(_texParams, texParams, rings * sizeof(float));
	_texGenMode = mode;
	return this;
}

bool SphereGlowModel::evalVisibilityParams(CDrawableModelData *data, Vector3 *, float *)
{
	return data && ((GlowDrawableData *)data)->_visibility > 0.01f;
}

LClass *SphereGlowModel::modelDataClass()
{
	return LF_CLASSOF(GlowDrawableData);
}

unsigned int SphereGlowModel::numPrimitivesForMesh(CMesh *)
{
	return (_rings * 2 - 2) * _slices;
}

// Three times the vertices filled, as in the original.
unsigned int SphereGlowModel::numVerticesForMesh(CMesh *)
{
	return (_slices + 1) * _rings * 3;
}

void SphereGlowModel::meshWasInstantiated(CMesh *mesh)
{
	{
		StVertexBufferLocker *locker = mesh->detachVBLocker(0, 0);
		CRenderProfile *profile = mesh->renderProfile();
		if (profile->_shader == 0)
			LF_RAISE(WrongConditionException());
		int pos = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
		int uv = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
		unsigned int stride = profile->vertexStructSize(0);
		char *p = (char *)locker->data();
		for (unsigned int r = 0; r < _rings; r++) {
			float t = _texParams[r], radius = _radii[r];
			for (unsigned int j = 0; j <= _slices; j++, p += stride) {
				float f = (float)(int)j / (float)_slices;
				float a = f * 6.2831855f;
				float c = cosf(a), s = sinf(a);
				if (pos >= 0) {
					float *v = (float *)(p + pos);
					v[0] = radius * c;
					v[1] = radius * s;
					v[2] = radius;
				}
				if (uv >= 0) {
					float *v = (float *)(p + uv);
					if (_texGenMode == kTexGenRadial) {
						v[0] = c * 0.5f * t + 0.5f;
						v[1] = s * 0.5f * t + 0.5f;
					} else if (_texGenMode == kTexGenCylindric) {
						v[0] = f;
						v[1] = t;
					}
				}
			}
		}
		delete locker;
	}
	StIndexBufferLocker *locker = mesh->detachIBLocker(0);
	unsigned int row = _slices + 1;
	if (locker->_desc.Format == D3DFMT_INDEX32) {
		uint32_t *p = (uint32_t *)locker->data();
		for (unsigned int r = 0; r + 1 < _rings; r++) {
			uint32_t a = r * row, b = a + row;
			for (unsigned int j = 0; j < _slices; j++, a++, b++, p += 6) {
				p[0] = a;
				p[1] = a + 1;
				p[2] = b + 1;
				p[3] = a;
				p[4] = b + 1;
				p[5] = b;
			}
		}
	} else {
		uint16_t *p = (uint16_t *)locker->data();
		for (unsigned int r = 0; r + 1 < _rings; r++) {
			uint16_t a = (uint16_t)(r * row), b = (uint16_t)(a + row);
			for (unsigned int j = 0; j < _slices; j++, a++, b++, p += 6) {
				p[0] = a;
				p[1] = a + 1;
				p[2] = b + 1;
				p[3] = a;
				p[4] = b + 1;
				p[5] = b;
			}
		}
	}
	delete locker;
}
