// LKeyRemap
//
// LF::LKeyRemap and the LF::LKeyRemapInitializer that fills its tables.

#include "LKeyRemap.h"

namespace LF {

static const struct {
	int code;
	const char *name;
} kKeyNames[] = {
	{LKeyRemap::kSpaceKey, " "},
	{LKeyRemap::kEnterKey, "\n"},
	{LKeyRemap::kEscKey, "esc"},
	{LKeyRemap::kTabKey, "\t"},
	{LKeyRemap::kPlusKey, "+"},
	{LKeyRemap::kMinusKey, "-"},
	{LKeyRemap::kLeftArrowKey, "left"},
	{LKeyRemap::kRightArrowKey, "right"},
	{LKeyRemap::kUpArrowKey, "up"},
	{LKeyRemap::kDownArrowKey, "down"},
	{LKeyRemap::kHomeKey, "home"},
	{LKeyRemap::kEndKey, "end"},
	{LKeyRemap::kPgUpKey, "pgup"},
	{LKeyRemap::kPgDownKey, "pgdown"},
	{LKeyRemap::kDelKey, "delete"},
	{LKeyRemap::kInsKey, "insert"},
	{LKeyRemap::kBackKey, "backspace"},
	{LKeyRemap::kF1Key, "f1"},
	{LKeyRemap::kF2Key, "f2"},
	{LKeyRemap::kF3Key, "f3"},
	{LKeyRemap::kF4Key, "f4"},
	{LKeyRemap::kF5Key, "f5"},
	{LKeyRemap::kF6Key, "f6"},
	{LKeyRemap::kF7Key, "f7"},
	{LKeyRemap::kF8Key, "f8"},
	{LKeyRemap::kF9Key, "f9"},
	{LKeyRemap::kF10Key, "f10"},
	{LKeyRemap::kF11Key, "f11"},
	{LKeyRemap::kF12Key, "f12"},
	{LKeyRemap::kF13Key, "f13"},
	{LKeyRemap::kF14Key, "f14"},
	{LKeyRemap::kF15Key, "f15"},
	{LKeyRemap::k0Key, "0"},
	{LKeyRemap::k1Key, "1"},
	{LKeyRemap::k2Key, "2"},
	{LKeyRemap::k3Key, "3"},
	{LKeyRemap::k4Key, "4"},
	{LKeyRemap::k5Key, "5"},
	{LKeyRemap::k6Key, "6"},
	{LKeyRemap::k7Key, "7"},
	{LKeyRemap::k8Key, "8"},
	{LKeyRemap::k9Key, "9"},
	{LKeyRemap::kAKey, "a"},
	{LKeyRemap::kBKey, "b"},
	{LKeyRemap::kCKey, "c"},
	{LKeyRemap::kDKey, "d"},
	{LKeyRemap::kEKey, "e"},
	{LKeyRemap::kFKey, "f"},
	{LKeyRemap::kGKey, "g"},
	{LKeyRemap::kHKey, "h"},
	{LKeyRemap::kIKey, "i"},
	{LKeyRemap::kJKey, "j"},
	{LKeyRemap::kKKey, "k"},
	{LKeyRemap::kLKey, "l"},
	{LKeyRemap::kMKey, "m"},
	{LKeyRemap::kNKey, "n"},
	{LKeyRemap::kOKey, "o"},
	{LKeyRemap::kPKey, "p"},
	{LKeyRemap::kQKey, "q"},
	{LKeyRemap::kRKey, "r"},
	{LKeyRemap::kSKey, "s"},
	{LKeyRemap::kTKey, "t"},
	{LKeyRemap::kUKey, "u"},
	{LKeyRemap::kVKey, "v"},
	{LKeyRemap::kWKey, "w"},
	{LKeyRemap::kXKey, "x"},
	{LKeyRemap::kYKey, "y"},
	{LKeyRemap::kZKey, "z"},
};

typedef TMap<const LComparable *, int, LComparableKeyCallbacks> TStringToCodeMap;

static const unsigned kKeyCount = sizeof kKeyNames / sizeof kKeyNames[0];
static LConstantString sKeyStrings[kKeyCount];
static TStringToCodeMap *sStringToCodeRemap = 0;
static CHashTable *sCodeToStringRemap = 0;

// Builds both tables at load time, as the original's static initializer did.
static struct LKeyRemapInitializer {
	LKeyRemapInitializer()
	{
		sStringToCodeRemap = new TStringToCodeMap;
		sCodeToStringRemap = new CHashTable;
		for (unsigned i = 0; i < kKeyCount; i++) {
			sKeyStrings[i].initWithCString(kKeyNames[i].name, 0, kLMacRomanEncoding);
			sStringToCodeRemap->setObjectForKey(kKeyNames[i].code, &sKeyStrings[i]);
			sCodeToStringRemap->setValueForKey((intptr_t)&sKeyStrings[i], kKeyNames[i].code);
		}
	}
} sKeyRemapInitializer;

unsigned int LKeyRemap::modifiersFromNSEventParameters(unsigned int flags, unsigned int keyCode)
{
	unsigned int modifiers = 0;
	if (flags & 0x40000)	// NSControlKeyMask
		modifiers |= kControlModifier;
	if (flags & 0x20000)	// NSShiftKeyMask
		modifiers |= kShiftModifier;
	if (flags & 0x80000)	// NSAlternateKeyMask
		modifiers |= kOptionModifier;
	if (flags & 0x100000)	// NSCommandKeyMask
		modifiers |= kCommandModifier;
	// The key itself may be one of the modifier keys.
	if (keyCode < 28) {
		unsigned int bit = 1u << keyCode;
		if (bit & 0x42)
			modifiers |= kModifierKeyGroup1;
		else if (bit & 0x88)
			modifiers |= kModifierKeyGroup2;
		else if (bit & 0xa000000)
			modifiers |= kModifierKeyGroup3;
	}
	return modifiers;
}

int LKeyRemap::keycodeFromSystemKeycode(unsigned int keyCode)
{
	switch (keyCode) {
	case 0x00: return kAKey;
	case 0x01: return kSKey;
	case 0x02: return kDKey;
	case 0x03: return kFKey;
	case 0x04: return kHKey;
	case 0x05: return kGKey;
	case 0x06: return kZKey;
	case 0x07: return kXKey;
	case 0x08: return kCKey;
	case 0x09: return kVKey;
	case 0x0a: return kTabKey;
	case 0x0b: return kBKey;
	case 0x0c: return kQKey;
	case 0x0d: return kWKey;
	case 0x0e: return kEKey;
	case 0x0f: return kRKey;
	case 0x10: return kYKey;
	case 0x11: return kTKey;
	case 0x18: return kPlusKey;
	case 0x1b: return kMinusKey;
	case 0x1f: return kOKey;
	case 0x20: return kUKey;
	case 0x22: return kIKey;
	case 0x23: return kPKey;
	case 0x24: return kEnterKey;
	case 0x25: return kLKey;
	case 0x26: return kJKey;
	case 0x28: return kKKey;
	case 0x2d: return kNKey;
	case 0x2e: return kMKey;
	case 0x31: return kSpaceKey;
	case 0x35: return kEscKey;
	case 0x45: return kPlusKey;
	case 0x4c: return kEnterKey;
	case 0x4e: return kMinusKey;
	case 0x7b: return kLeftArrowKey;
	case 0x7c: return kRightArrowKey;
	case 0x7d: return kDownArrowKey;
	case 0x7e: return kUpArrowKey;
	case kMouseButton1:
	case kMouseButton2:
	case kMouseButton3:
	case kMouseButton4:
		return keyCode;
	default:
		return kNoKey;
	}
}

LString *LKeyRemap::lowercaseKeyStringFromCode(int code)
{
	intptr_t str = 0;
	if (!sCodeToStringRemap->getValueForKey(&str, code))
		return 0;
	return (LString *)str;
}

int LKeyRemap::codeFromKeyString(LString *str)
{
	TStringToCodeMap::Entry **e = sStringToCodeRemap->entryPtrAddrForKey(str);
	return e ? (*e)->value : kNoKey;
}

} // namespace LF
