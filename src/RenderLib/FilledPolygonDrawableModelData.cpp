// FilledPolygonDrawableModelData: a fill's draw data; it draws stage 3.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(FilledPolygonDrawableModelData, LF_CLASSOF(ShapeModelData))

void FilledPolygonDrawableModelData::setupWithUsageKey(const LString *key)
{
	if (key->isEqualTo(ShapeModelData::kUsageAllShapesHighlight))
		_hitShape = false;
	else if (key->isEqualTo(ShapeModelData::kUsageHitShapeHighlight))
		_hitShape = true;
}

bool FilledPolygonDrawableModelData::_isStageOn(CShaderStage *stage, unsigned char *on)
{
	*on = stage->_objectID == 3;
	return true;
}
