// A holder's ground elements, their loading queue and the shared buffers.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(GroundElementsManagerData, 0)

GroundElementsManagerData::GroundElementsManagerData()
	: _postedCount(0), _profile(0), _waterTexture(0), _darkWaterTexture(0), _mins(0), _maxs(0), _queue(0),
	  _cameraKey(0), _drawn(0)
{
	memset(_xyBuffers, 0, sizeof(_xyBuffers));
	memset(_zNrmBuffers, 0, sizeof(_zNrmBuffers));
	_toDraw.setComparator(_compareGEDatasByTexture);
	_toDraw.setKeepsSorted(true);
}

void GroundElementsManagerData::dealloc()
{
	LRelease(_waterTexture);
	LRelease(_darkWaterTexture);
	_toDraw.removeAllObjects();
	if (LTaskQueue *queue = _queue) {
		_queue = 0;
		queue->terminateQueue(1);
	}
	for (int i = 0; i < 15; i++)
		LRelease(_xyBuffers[i]);
	for (int i = 0; i < 15; i++)
		LRelease(_zNrmBuffers[i]);
	LRelease(_profile);
	delete[] _mins;
	_mins = 0;
	delete[] _maxs;
	_maxs = 0;
	delete _drawn;
	_drawn = 0;
	// The original made the manager here if there was none.
	if (GroundElementsManager::classObject()->hasSingleton()) {
		GroundElementsManager *manager = GroundElementsManager::instance();
		if (manager->_data == this) {
			manager->_holder = 0;
			manager->_data = 0;
		}
	}
	TLIntPtrMap<GroundElement *>::hashTableIterator it(&_elements);
	while (it.next())
		it.entry()->value->_holder = 0;
	LObject::dealloc();
}

int GroundElementsManagerData::_compareGEDatasByTexture(const void *a, const void *b, const void *)
{
	const STextureID &ta = (*(GlobeAdaptiveMeshData *const *)a)->_drawInfo.texture.textureID;
	const STextureID &tb = (*(GlobeAdaptiveMeshData *const *)b)->_drawInfo.texture.textureID;
	return (int)(CTextureLoader::keyForID(tb) - CTextureLoader::keyForID(ta));
}
