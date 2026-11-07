// The stars' particle system: with patches, only those in view are drawn.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(StarsParticleSystem, LF_CLASSOF(CParticleSystem))

LClass *StarsParticleSystem::modelDataClass()
{
	return LF_CLASSOF(StarsModelsDrawableData);
}

void StarsParticleSystem::doImmediateDrawWithData(CDrawableModelData *data, CFrameDrawer *drawer)
{
	if (_usePatches) {
		StarsModelsDrawableData *stars = lobj_cast<StarsModelsDrawableData>(data);
		if (stars)
			stars->fillVisibleEntries(_context);
	}
	CParticleSystem::doImmediateDrawWithData(data, drawer);
}
