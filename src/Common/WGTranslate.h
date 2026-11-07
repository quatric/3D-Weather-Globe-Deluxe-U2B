// Marks code that hasn't been translated from the i386 original yet.
// WG_UNTRANSLATED() logs the method the first time it runs, so a session
// shows which missing pieces the app actually reaches.
#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C" {
#endif
void WGUntranslated(const char *function);
#ifdef __cplusplus
}
#endif

#define WG_UNTRANSLATED() do { \
		static int wg_seen; \
		if (!wg_seen) { wg_seen = 1; WGUntranslated(__PRETTY_FUNCTION__); } \
	} while (0)
