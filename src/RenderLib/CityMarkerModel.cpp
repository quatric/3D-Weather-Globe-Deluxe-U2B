// The flat map's route point markers.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CityMarkerModel, LF_CLASSOF(CParticleSystem))

LClass *CityMarkerModel::modelDataClass()
{
	return CityMarkerModelData::classObject();
}
