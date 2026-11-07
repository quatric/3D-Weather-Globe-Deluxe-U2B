// The constellation lines: AtlasDatabase's star points and line indices.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(ConstellLinesDataSource, 0)

// A constellationsStarsArray() record.
struct SConstellStar {
	Vector3 position;
	unsigned int color;
};

ConstellLinesDataSource *ConstellLinesDataSource::init()
{
	if (LObject::init() == 0)
		return 0;
	return this;
}

void ConstellLinesDataSource::dealloc()
{
	releaseAndClear(_indices);
	releaseAndClear(_stars);
	LObject::dealloc();
}

// The original divided by 12, reading past the 16-byte records.
unsigned int ConstellLinesDataSource::numVerticesForMesh(CMesh *)
{
	if (_stars == 0) {
		_stars = LRetain(AtlasDatabase::instance()->constellationsStarsArray());
		if (_stars == 0)
			return 0;
	}
	return _stars->length() / sizeof(SConstellStar);
}

// Two 16-bit indices a line.
unsigned int ConstellLinesDataSource::numPrimitivesForMesh(CMesh *)
{
	if (_indices == 0) {
		_indices = LRetain(AtlasDatabase::instance()->constellationsIndsArray());
		if (_indices == 0)
			return 0;
	}
	return _indices->length() >> 2;
}

void ConstellLinesDataSource::meshWasInstantiated(CMesh *mesh)
{
	{
		StVertexBufferLocker *locker = mesh->detachVBLocker(0, 0);
		CRenderProfile *profile = mesh->renderProfile();
		if (profile->_shader == 0)
			LF_RAISE(WrongConditionException());
		char *p = (char *)locker->data();
		unsigned int stride = profile->vertexStructSize(0);
		int count = (int)numVerticesForMesh(mesh);
		const SConstellStar *star = (const SConstellStar *)_stars->bytes();
		int pos = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
		int color = profile->drawableElementOffset(D3DDECLUSAGE_COLOR, 0, 0);
		for (int i = 0; i < count; i++, p += stride, star++) {
			memcpy(p + pos, &star->position, sizeof(Vector3));
			*(unsigned int *)(p + color) = star->color;
		}
		delete locker;
	}
	StIndexBufferLocker *locker = mesh->detachIBLocker(0);
	memcpy(locker->data(), _indices->bytes(), numPrimitivesForMesh(mesh) * 4);
	delete locker;
}
