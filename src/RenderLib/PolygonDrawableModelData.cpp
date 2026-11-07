// PolygonDrawableModelData: an outline's draw data. Stage 2 is always on; the
// others only for the hit shape's highlight.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(PolygonDrawableModelData, LF_CLASSOF(ShapeModelData))

void PolygonDrawableModelData::setupWithUsageKey(const LString *key)
{
	if (key->isEqualTo(ShapeModelData::kUsageAllShapesHighlight))
		_hitShape = false;
	else if (key->isEqualTo(ShapeModelData::kUsageHitShapeHighlight))
		_hitShape = true;
}

bool PolygonDrawableModelData::_isStageOn(CShaderStage *stage, unsigned char *on)
{
	*on = _hitShape || stage->_objectID == 2;
	return true;
}
