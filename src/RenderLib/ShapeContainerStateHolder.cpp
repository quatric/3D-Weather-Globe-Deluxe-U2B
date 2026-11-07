// ShapeContainerStateHolder: the highlighted shape of a recorded frame, by
// its shape and object IDs, drawn again on playback.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(ShapeContainerStateHolder, 0)

void ShapeContainerStateHolder::dealloc()
{
	releaseAndClear(_shapeID);
	releaseAndClear(_objectUID);
	releaseAndClear(_caption);
	LObject::dealloc();
}

void ShapeContainerStateHolder::storeSceneStateInStream(LData *stream, unsigned int)
{
	storeStringInStream(_shapeID, stream);
	storeStringInStream(_objectUID, stream);
	storeStringInStream(_caption, stream);
}

const void *ShapeContainerStateHolder::restoreSceneStateFromData(const void *data, unsigned int)
{
	releaseAndClear(_shapeID);
	releaseAndClear(_objectUID);
	releaseAndClear(_caption);
	data = loadStringFromStream(data, &_shapeID);
	data = loadStringFromStream(data, &_objectUID);
	data = loadStringFromStream(data, &_caption);
	LRetain(_shapeID);
	LRetain(_objectUID);
	LRetain(_caption);
	return data;
}

void ShapeContainerStateHolder::renderSelfHolderInScene(CScene *scene, unsigned int)
{
	CShapeContainer *container = CShapeManager::instance()->shapeForObject(_objectUID, _shapeID);
	if (container && (CScene *)_scene == scene)
		_scene->_renderShapeContainer(_scene->frameDrawer(), container, _caption, 0);
}

void ShapeContainerStateHolder::grabInfoFromShapeContainer(CShapeContainer *container, const LString *caption)
{
	releaseAndClear(_shapeID);
	_shapeID = copyString(container->_shapeID);
	releaseAndClear(_objectUID);
	_objectUID = copyString(container->_objectUID);
	releaseAndClear(_caption);
	_caption = copyString(caption);
}
