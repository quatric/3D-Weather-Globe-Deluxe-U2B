// CPushpinModel: the pushpin, the model a CPushpinParticleSystem instances.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CPushpinModel, LF_CLASSOF(CBuffersOwner))

LClass *CPushpinModel::modelDataClass()
{
	return LF_CLASSOF(ModelDataWithMatrix);
}
