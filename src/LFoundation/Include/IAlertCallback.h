// IAlertCallback: how LFoundation code shows alerts. The application installs
// one callback, which turns error codes into texts and runs the alerts.
#pragma once

#include "LString.h"

namespace LF {

class IAlertCallback {
public:
	enum TAlertResult {
		kAlertNotShown = 0x100	// no callback is installed
	};

	struct SAlertParams {
		LString *title;
		LString *message;
		LString *defaultButton;
		LString *alternateButton;
		LString *otherButton;
		int defaultResult;	// a TAlertResult
		LString *helpAnchor;
		LString *suppressionKey;	// a user default that turns the alert off
	};

	virtual void _fillParamsFromTheCode(SAlertParams *params, int code) = 0;
	virtual int _showAlert(const SAlertParams *params) = 0;

	static void setAlertCallback(IAlertCallback *callback) { sCallback = callback; }
	static int showAlertWithParams(const SAlertParams *params);
	static int showAlert(LString *title, LString *message, LString *defaultButton, LString *alternateButton,
	    LString *otherButton, TAlertResult defaultResult, LString *helpAnchor, LString *suppressionKey);
	// The callback fills in the texts for the code.
	static int showAlertWithCode(int code);

	static IAlertCallback *sCallback;
};

} // namespace LF
