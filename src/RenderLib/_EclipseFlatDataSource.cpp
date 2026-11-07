// The flat map's grid for the eclipse shadow: each vertex carries the
// globe point it shows.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(_EclipseFlatDataSource, 0)

void _EclipseFlatDataSource::meshWasInstantiated(CMesh *mesh)
{
	{
		StVertexBufferLocker *locker = mesh->detachVBLocker(0, 0);
		CRenderProfile *profile = mesh->renderProfile();
		if (profile->_shader == 0)
			LF_RAISE(WrongConditionException());
		char *p = (char *)locker->data();
		unsigned int stride = profile->vertexStructSize(0);
		int pos = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
		int uv = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
		for (int r = 0; r < 0x21; r++) {
			for (int c = 0; c < 0x41; c++, p += stride) {
				if (pos >= 0) {
					float *v = (float *)(p + pos);
					v[0] = (float)c * 0.015625f;
					v[1] = (float)r * 0.03125f;
					v[2] = 0.0f;
				}
				if (uv >= 0) {
					Vector3 e = earthVecFromMapPoint(LMakePoint((float)c * 0.015625f, (float)r * 0.03125f));
					memcpy(p + uv, &e, sizeof(Vector3));
				}
			}
		}
		delete locker;
	}
	StIndexBufferLocker *locker = mesh->detachIBLocker(0);
	if (locker->_desc.Format == D3DFMT_INDEX32) {
		uint32_t *p = (uint32_t *)locker->data();
		for (uint32_t row = 0; row != 0x820; row += 0x41)
			for (uint32_t v = row; v < row + 0x40; v++, p += 6) {
				p[0] = v;
				p[1] = v + 1;
				p[2] = v + 0x42;
				p[3] = v;
				p[4] = v + 0x42;
				p[5] = v + 0x41;
			}
	} else {
		uint16_t *p = (uint16_t *)locker->data();
		for (uint16_t row = 0; row != 0x820; row += 0x41)
			for (uint16_t v = row; v < row + 0x40; v++, p += 6) {
				p[0] = v;
				p[1] = v + 1;
				p[2] = v + 0x42;
				p[3] = v;
				p[4] = v + 0x42;
				p[5] = v + 0x41;
			}
	}
	delete locker;
}
