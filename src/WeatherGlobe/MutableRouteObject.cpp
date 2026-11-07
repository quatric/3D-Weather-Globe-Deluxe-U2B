// MutableRouteObject: the app's copy of AppModel's constructor (AMRoutes.h),
// not translated again. Also sign(), which ended up in this object file.

#include "WGAppClasses.h"

int sign(int value)
{
	if (value > 0)
		return 1;
	return value >> 31;
}
