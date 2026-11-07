// LKeyRemap: LFoundation's own key codes and modifiers, converted from Mac
// virtual key codes and NSEvent flags, and named by lowercase strings
// ("a", "f1", "left", "pgup"...) for keyboard shortcuts in resources.
#pragma once

#include "LString.h"

namespace LF {

class LKeyRemap {
public:
	enum TKeyCode {
		kNoKey = 0,
		kSpaceKey = 0x01,
		kEnterKey = 0x02,
		kEscKey = 0x03,
		kTabKey = 0x04,
		kPlusKey = 0x05,
		kMinusKey = 0x06,
		kLeftArrowKey = 0x07,
		kRightArrowKey = 0x08,
		kUpArrowKey = 0x09,
		kDownArrowKey = 0x0a,
		kHomeKey = 0x0b,
		kEndKey = 0x0c,
		kPgUpKey = 0x0d,
		kPgDownKey = 0x0e,
		kDelKey = 0x0f,
		kInsKey = 0x10,
		kBackKey = 0x11,
		kF1Key = 0x12,
		kF2Key = 0x13,
		kF3Key = 0x14,
		kF4Key = 0x15,
		kF5Key = 0x16,
		kF6Key = 0x17,
		kF7Key = 0x18,
		kF8Key = 0x19,
		kF9Key = 0x1a,
		kF10Key = 0x1b,
		kF11Key = 0x1c,
		kF12Key = 0x1d,
		kF13Key = 0x1e,
		kF14Key = 0x1f,
		kF15Key = 0x20,
		k0Key = 0x21,
		k1Key = 0x22,
		k2Key = 0x23,
		k3Key = 0x24,
		k4Key = 0x25,
		k5Key = 0x26,
		k6Key = 0x27,
		k7Key = 0x28,
		k8Key = 0x29,
		k9Key = 0x2a,
		kAKey = 0x2b,
		kBKey = 0x2c,
		kCKey = 0x2d,
		kDKey = 0x2e,
		kEKey = 0x2f,
		kFKey = 0x30,
		kGKey = 0x31,
		kHKey = 0x32,
		kIKey = 0x33,
		kJKey = 0x34,
		kKKey = 0x35,
		kLKey = 0x36,
		kMKey = 0x37,
		kNKey = 0x38,
		kOKey = 0x39,
		kPKey = 0x3a,
		kQKey = 0x3b,
		kRKey = 0x3c,
		kSKey = 0x3d,
		kTKey = 0x3e,
		kUKey = 0x3f,
		kVKey = 0x40,
		kWKey = 0x41,
		kXKey = 0x42,
		kYKey = 0x43,
		kZKey = 0x44,
		// Mouse buttons pass through unchanged.
		kMouseButton1 = 0x1000,
		kMouseButton2 = 0x1001,
		kMouseButton3 = 0x1002,
		kMouseButton4 = 0x1003
	};

	enum TModifiers {
		kControlModifier = 0x01,
		kOptionModifier = 0x02,
		kShiftModifier = 0x04,
		kModifierKeyGroup1 = 0x08,	// key codes 1 and 6
		kModifierKeyGroup2 = 0x10,	// key codes 3 and 7
		kModifierKeyGroup3 = 0x20,	// key codes 25 and 27
		kCommandModifier = 0x40
	};

	static unsigned int modifiersFromSystemModifiers(unsigned int modifiers) { return modifiers; }
	// flags are NSEvent modifier flags; keyCode is a virtual key code.
	static unsigned int modifiersFromNSEventParameters(unsigned int flags, unsigned int keyCode);
	static int keycodeFromSystemKeycode(unsigned int keyCode);
	static LString *lowercaseKeyStringFromCode(int code);
	static int codeFromKeyString(LString *str);
};

} // namespace LF
