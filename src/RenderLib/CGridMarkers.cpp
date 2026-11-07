// The coordinates grid's base.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CGridMarkers, 0)

CGridMarkers::CGridMarkers() : _scene(0), _gridMesh(0), _longitudeStep(10.0f), _latitudeStep(10.0f)
{
}

CGridMarkers *CGridMarkers::initWithScene(BaseScene *scene)
{
	if (!scene)
		return 0;
	_scene = scene;
	_stateOwnerName = kObjNameGridLinesContainer;
	return this;
}

void CGridMarkers::dealloc()
{
	releaseAndClear(_gridMesh);
	LObject::dealloc();
}

void CGridMarkers::setGridMesh(GridLineMesh *mesh)
{
	LRetain(mesh);
	releaseAndClear(_gridMesh);
	_gridMesh = mesh;
}

void CGridMarkers::setGridSteps(float longitude, float latitude)
{
	_longitudeStep = longitude;
	_latitudeStep = latitude;
}

void CGridMarkers::storeSceneStateInStream(LData *stream, unsigned int)
{
	storeFloatInStream(_longitudeStep, stream);
	storeFloatInStream(_latitudeStep, stream);
}

const void *CGridMarkers::restoreSceneStateFromData(const void *data, unsigned int)
{
	data = loadFloatFromStream(data, &_longitudeStep);
	return loadFloatFromStream(data, &_latitudeStep);
}

void CGridMarkers::renderSelfHolderInScene(CScene *scene, unsigned int)
{
	render(scene->frameDrawer());
}
