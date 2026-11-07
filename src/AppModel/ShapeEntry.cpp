// ShapeEntry: a region with an outline: a country, state, county or time zone.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(ShapeEntry, LF_CLASSOF(GenericEntry))

ShapeEntry::ShapeEntry() : _shapeID(0)
{
}

void ShapeEntry::interfaceRetain()
{
	retain();
}

void ShapeEntry::interfaceRelease()
{
	release();
}

void ShapeEntry::dealloc()
{
	if (_shapeID) {
		_shapeID->release();
		_shapeID = 0;
	}
	GenericEntry::dealloc();
}

float ShapeEntry::visualWeight() const
{
	return 100.0f;
}

LString *ShapeEntry::shapeID() const
{
	if (_shapeID == 0) {
		if (_ownType->isEqualTo(kGeoTypeCounty))
			_shapeID = CShapeManager::kShapeIDCounties;
		if (_ownType->isEqualTo(kGeoTypeState))
			_shapeID = CShapeManager::kShapeIDStates;
		if (_ownType->isEqualTo(kGeoTypeCountry))
			_shapeID = CShapeManager::kShapeIDCountries;
		if (_ownType->isEqualTo(kGeoTypeTimezone))
			_shapeID = CShapeManager::kShapeIDTimezones;
		if (_shapeID)
			_shapeID->retain();
	}
	return _shapeID;
}

CShapeContainer *ShapeEntry::shapeContainer() const
{
	return CShapeManager::instance()->shapeForObject(uid(0), shapeID());
}

bool ShapeEntry::getEntryGeoRect(LRect *rect)
{
	if (rect == 0)
		return true;
	CShapeContainer *shape = shapeContainer();
	if (shape == 0)
		return false;
	*rect = shape->allPathsBoundsGeoRect();
	return true;
}
