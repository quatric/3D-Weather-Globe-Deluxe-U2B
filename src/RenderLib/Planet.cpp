// A planet: a sphere mesh made on first use, drawn with its own data.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(Planet, 0)

Planet::Planet() : _profile(0), _mesh(0)
{
}

void Planet::dealloc()
{
	releaseAndClear(_mesh);
	releaseAndClear(_profile);
	LObject::dealloc();
}

Planet *Planet::initWithRenderProfile(CRenderProfile *profile)
{
	if (LObject::init() == 0)
		return 0;
	_profile = LRetain(profile);
	return this;
}

PlanetDrawableData *Planet::detachDrawableData()
{
	if (_mesh == 0) {
		SStdGeometry geometry;
		geometry.type = kStdGeometrySphere;
		geometry.cols = 0x20;
		geometry.rows = 0x18;
		geometry.radius = 1.0f;
		geometry.partsX = 1;
		geometry.partsY = 1;
		geometry.partX = 0;
		geometry.partY = 0;
		CHolder *holder = _profile->holder();
		if (holder)
			holder->beginAccumulationBlock();
		_mesh = PlanetMesh::_alloc()->initWithStdGeometry(&geometry, _profile);
		if (holder)
			holder->endAccumulationBlock();
		if (_mesh == 0)
			return 0;
	}
	return lobj_cast<PlanetDrawableData>(_mesh->detachModelDataInstance());
}
