// CPointsContainer: a shape of points, declared by the original and unused.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CPointsContainer, LF_CLASSOF(CShapeContainer))

CPointsContainer *CPointsContainer::init()
{
	return LObject::init() ? this : 0;
}
