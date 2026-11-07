#import "WGTranslate.h"

void WGUntranslated(const char *function)
{
	if (getenv("WG_QUIET_UNTRANSLATED") == NULL) {
		fprintf(stderr, "untranslated: %s\n", function);
	}
}
