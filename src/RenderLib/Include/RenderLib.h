// RenderLib: the globe and map renderer, built on FishHook and AppModel:
// the scenes, the earth, sky, clouds and weather objects drawn in them, and
// the panel controllers the views drive.

#ifndef RENDERLIB_H
#define RENDERLIB_H

#include "AppModel.h"
#include "FHMeshes.h"
#include "FHParticles.h"
#include "FHSprites.h"
#include "FHScene.h"
#include "RLOptions.h"
#include "RLObjNames.h"
#include "RLCinematic.h"
#include "RLWeather.h"
#include "RLTextures.h"
#include "RLSky.h"
#include "RLShapes.h"
#include "RLScenes.h"
#include "RLGlobe.h"
#include "RLPanels.h"
#include "RLMovie.h"
#include "RLFFT.h"
#include "RenderLibStrings.h"

// The RenderLib bundle, for its localized string tables.
void setRenderLibModule(CFBundleRef bundle);
CFBundleRef getRenderLibModule();

template <typename T> static inline void releaseAndClear(T *&p)
{
	LRelease(p);
	p = 0;
}

// A new string with `s`'s characters (empty for 0), as the original's
// `LString::_alloc()` + `setValue`.
static inline LString *copyString(const LString *s)
{
	return s ? s->copy() : (LString *)LString::_alloc()->init();
}

static inline float clamp01(float f)
{
	return f >= 0.0f ? (f <= 1.0f ? f : 1.0f) : 0.0f;
}

#endif
