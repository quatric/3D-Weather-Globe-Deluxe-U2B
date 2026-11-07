// SmartBillboardData: an entry of a SmartBillboardSystem.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(SmartBillboardData, 0)

SmartBillboardData *SmartBillboardData::initWithParams(LPoint position, int orientation, unsigned char showAlways,
                                                       unsigned int kind)
{
	if (!LObject::init())
		return 0;
	_orientation = orientation;
	_position = position;
	_showAlways = showAlways;
	_kind = kind;
	return this;
}

LComparable *SmartBillboardData::uniqueKey()
{
	return 0;
}
