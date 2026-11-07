// CPushpinParticleSystem: the route points on the globe, a pushpin each,
// optionally jumping.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CPushpinParticleSystem, LF_CLASSOF(CParticleSystem))

LClass *CPushpinParticleSystem::modelDataClass()
{
	return LF_CLASSOF(CPushpinParticleSystemData);
}

void CPushpinParticleSystem::doImmediateDrawWithData(CDrawableModelData *data, CFrameDrawer *drawer)
{
	CPushpinParticleSystemData *pushpins = lobj_cast<CPushpinParticleSystemData>(data);
	if (pushpins)
		pushpins->updateTransformsData();
	CParticleSystem::doImmediateDrawWithData(data, drawer);
}
