// IAlertCallback
//
// LF::IAlertCallback's static part.

#include "IAlertCallback.h"

namespace LF {

IAlertCallback *IAlertCallback::sCallback = 0;

int IAlertCallback::showAlertWithParams(const SAlertParams *params)
{
	return sCallback ? sCallback->_showAlert(params) : kAlertNotShown;
}

int IAlertCallback::showAlert(LString *title, LString *message, LString *defaultButton, LString *alternateButton,
    LString *otherButton, TAlertResult defaultResult, LString *helpAnchor, LString *suppressionKey)
{
	SAlertParams params = {title, message, defaultButton, alternateButton, otherButton, defaultResult,
	    helpAnchor, suppressionKey};
	return showAlertWithParams(&params);
}

int IAlertCallback::showAlertWithCode(int code)
{
	SAlertParams params = {};
	if (sCallback == 0)
		return kAlertNotShown;
	sCallback->_fillParamsFromTheCode(&params, code);
	return showAlertWithParams(&params);
}

} // namespace LF
