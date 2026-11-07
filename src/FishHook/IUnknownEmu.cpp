// IUnknownEmu, ResourceEmu: the base of the emulated D3D objects.

#include "FHDeviceEmu.h"

LF_IMPLEMENT_CLASS(IUnknownEmu, 0)
LF_IMPLEMENT_CLASS(ResourceEmu, LF_CLASSOF(IUnknownEmu))

ResourceEmu *ResourceEmu::initWithParams(unsigned int usage, D3DPOOL pool, DeviceEmu *device)
{
	if (LObject::init() == 0)
		return 0;
	_usage = usage;
	_pool = pool;
	_device = device;
	return this;
}
