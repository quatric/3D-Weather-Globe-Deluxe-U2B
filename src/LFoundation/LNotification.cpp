// LNotification
//
// LF::LNotification: a notification's name, sending object and user info.

#include "LNotification.h"

namespace LF {

LF_IMPLEMENT_CLASS(LNotification, 0)

LNotification *LNotification::notificationWithParams(LObject *object, const LString *name, LMap *userInfo)
{
	LNotification *ntf = _alloc();
	if (ntf && ntf->initWithNtfParams(object, name, userInfo))
		return LAutorelease(ntf);
	return 0;
}

LNotification *LNotification::initWithNtfParams(LObject *object, const LString *name, LMap *userInfo)
{
	_object = LRetain(object);
	_name = LRetain(const_cast<LString *>(name));
	_userInfo = LRetain(userInfo);
	return this;
}

void LNotification::dealloc()
{
	if (_object) {
		_object->release();
		_object = 0;
	}
	if (_name) {
		_name->release();
		_name = 0;
	}
	if (_userInfo) {
		_userInfo->release();
		_userInfo = 0;
	}
	LObject::dealloc();
}

// The object's and user info's descriptions are cut to 999 characters.
LString *LNotification::internalDescription() const
{
	char objectText[1000];
	char userInfoText[1000];
	char text[3000];
	if (_object) {
		const char *s = _object->internalDescription()->cString();
		size_t n = strlen(s);
		if (n > 999)
			n = 999;
		objectText[n] = 0;
		memcpy(objectText, s, n);
	}
	const char *userInfo = "nil";
	if (_userInfo) {
		const char *s = _userInfo->internalDescription()->cString();
		size_t n = strlen(s);
		if (n > 999)
			n = 999;
		userInfoText[n] = 0;
		memcpy(userInfoText, s, n);
		userInfo = userInfoText;
	}
	snprintf(text, sizeof text, "LNotification(%p)\n{ object = %s\nname = %s\nuserInfo = %s }\n", (const void *)this,
	         _object ? objectText : "nil", _name ? _name->cString() : "nil", userInfo);
	return LString::stringWithCString(text, 0, kLMacRomanEncoding);
}

} // namespace LF
