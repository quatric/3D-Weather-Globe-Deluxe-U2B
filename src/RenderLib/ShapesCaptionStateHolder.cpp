// ShapesCaptionStateHolder: the shapes caption of a recorded frame, drawn
// again on playback.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(ShapesCaptionStateHolder, 0)

ShapesCaptionStateHolder *ShapesCaptionStateHolder::initWithScene(BaseScene *scene)
{
	if (LObject::init() == 0)
		return 0;
	_scene = scene;
	return this;
}

void ShapesCaptionStateHolder::dealloc()
{
	releaseAndClear(_shapeID);
	releaseAndClear(_objectUID);
	releaseAndClear(_caption);
	LObject::dealloc();
}

void ShapesCaptionStateHolder::storeSceneStateInStream(LData *stream, unsigned int)
{
	storeStringInStream(_shapeID, stream);
	storeStringInStream(_objectUID, stream);
	storeStringInStream(_caption, stream);
}

const void *ShapesCaptionStateHolder::restoreSceneStateFromData(const void *data, unsigned int)
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

// Only polygon shapes have captions.
void ShapesCaptionStateHolder::renderSelfHolderInScene(CScene *scene, unsigned int)
{
	CShapeContainer *container = CShapeManager::instance()->shapeForObject(_objectUID, _shapeID);
	if (container && lobj_cast<CPolysContainer>(container) && (CScene *)_scene == scene)
		_scene->_renderShapesCaption(_scene->frameDrawer(), _caption, container, 0);
}

void ShapesCaptionStateHolder::setShapesInfo(CShapeContainer *container, LString *caption)
{
	releaseAndClear(_shapeID);
	_shapeID = copyString(container->_shapeID);
	releaseAndClear(_objectUID);
	_objectUID = copyString(container->_objectUID);
	releaseAndClear(_caption);
	_caption = copyString(caption);
}
