// A ground element's DEM load on the manager's queue; the element gets
// _demLoadComplete when it is done.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(LoadDEMTask, LF_CLASSOF(LTask))

static int sNumExecutions;	// for profiling

void LoadDEMTask::dealloc()
{
	LRelease(_element);
	LTask::dealloc();
}

LoadDEMTask *LoadDEMTask::initWithElement(GroundElement *element)
{
	if (!initWithTargetAndSelector(element, "_demLoadComplete"))
		return 0;
	_element = element ? (GroundElement *)element->retain() : 0;
	_taskID = (int)CTextureLoader::keyForID(_element->_id);
	return this;
}

void LoadDEMTask::execute()
{
	GroundElement::loadDEMToMemory(_element);
	sNumExecutions++;
}

int LoadDEMTask::compare(const void *a, const void *b, const void *context)
{
	unsigned int camera = context ? *(const unsigned int *)context : 0;
	int cx = camera & 0xffff, cy = camera >> 16;
	const STextureID &ia = (*(LoadDEMTask *const *)a)->_element->_id;
	const STextureID &ib = (*(LoadDEMTask *const *)b)->_element->_id;
	int d = ia.level - ib.level;
	if (d != 0)
		return d;
	int ax = ia.x - cx, ay = ia.y - cy, bx = ib.x - cx, by = ib.y - cy;
	return (ax * ax + ay * ay) - (bx * bx + by * by);
}
