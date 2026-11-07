// A buffer of stars: drawn by the visible catalog patches it holds.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(StarsParticleSubSystem, LF_CLASSOF(CParticleSubsystem))

void StarsParticleSubSystem::drawSubset(CDrawableModelData *data, long, long)
{
	StarsModelsDrawableData *stars = lobj_cast<StarsModelsDrawableData>(((CParticleSubsystemData *)data)->_systemData);
	TArray<AtlasDatabase::SStarsPathInfo> *patches = AtlasDatabase::instance()->starsPatches();
	int count = (int)stars->_visiblePatches.count();
	int n = (int)(_end - _first);
	for (int i = 0; i < count; i++) {
		const AtlasDatabase::SStarsPathInfo &info = (*patches)[stars->_visiblePatches[i].patchInfoOffset()];
		int start = info.start - (int)_first;
		int end = start + info.count;
		if (start < 0)
			start = 0;
		if (end > n)
			end = n;
		if (start < end)
			_context->drawCache()->drawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, n * 4, start * 6, (end - start) * 2);
	}
}
