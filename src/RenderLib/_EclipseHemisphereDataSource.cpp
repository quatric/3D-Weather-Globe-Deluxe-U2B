// The globe's eclipse hemisphere: a little above the Earth, from 9 degrees
// below the equator to the pole; below the equator it fades out.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(_EclipseHemisphereDataSource, 0)

void _EclipseHemisphereDataSource::meshWasInstantiated(CMesh *mesh)
{
	{
		StVertexBufferLocker *locker = mesh->detachVBLocker(0, 0);
		CRenderProfile *profile = mesh->renderProfile();
		if (profile->_shader == 0)
			LF_RAISE(WrongConditionException());
		char *p = (char *)locker->data();
		unsigned int stride = profile->vertexStructSize(0);
		int pos = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
		int color = profile->drawableElementOffset(D3DDECLUSAGE_COLOR, 0, 0);
		int uv = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
		for (int r = 0; r < 0x21; r++) {
			float a = ((float)r * 0.03125f * 1.1f - 0.1f) * 1.5707964f;
			float ca = cosf(a), sa = sinf(a);
			for (int c = 0; c < 0x81; c++, p += stride) {
				float b = (float)c * 0.0078125f * 6.2831855f;
				Vector3 v(cosf(b) * ca * 1.001f, sa * 1.001f, sinf(b) * ca * 1.001f);
				if (pos >= 0)
					memcpy(p + pos, &v, sizeof(Vector3));
				if (uv >= 0) {
					float *t = (float *)(p + uv);
					if (v.y >= 0.0f) {
						t[0] = v.x * 0.5f + 0.5f;
						t[1] = v.z * 0.5f + 0.5f;
					} else {
						float k = 1.0f / sqrtf(v.x * v.x + v.z * v.z);
						t[0] = fabsf(v.x * k * 0.5f + 0.5f);
						t[1] = fabsf(k * v.z * 0.5f + 0.5f);
					}
				}
				if (color >= 0) {
					float f = v.y >= 0.0f ? 0.05f : -v.y / (sinf(0.15707964f) * 1.001f);
					*(unsigned int *)(p + color) = (unsigned int)Color(f, f, f, f);
				}
			}
		}
		delete locker;
	}
	StIndexBufferLocker *locker = mesh->detachIBLocker(0);
	if (locker->_desc.Format == D3DFMT_INDEX32) {
		uint32_t *p = (uint32_t *)locker->data();
		for (uint32_t row = 0; row != 0x1020; row += 0x81)
			for (uint32_t v = row; v < row + 0x80; v++, p += 6) {
				p[0] = v;
				p[1] = v + 1;
				p[2] = v + 0x82;
				p[3] = v;
				p[4] = v + 0x82;
				p[5] = v + 0x81;
			}
	} else {
		uint16_t *p = (uint16_t *)locker->data();
		for (uint16_t row = 0; row != 0x1020; row += 0x81)
			for (uint16_t v = row; v < row + 0x80; v++, p += 6) {
				p[0] = v;
				p[1] = v + 1;
				p[2] = v + 0x82;
				p[3] = v;
				p[4] = v + 0x82;
				p[5] = v + 0x81;
			}
	}
	delete locker;
}
