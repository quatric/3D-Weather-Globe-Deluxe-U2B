#import "WeatherGlobePrefix.h"


@interface LinkButton : NSButton
{
    NSRect _cursorRect;
    NSURL *_url;
    NSMutableDictionary *_titleAttr;
    NSMutableDictionary *_altTitleAttr;
}
- (void)setAlternateTitle:(id)aTitle;
- (void)setTitle:(id)aTitle;
- (void)setLineBreakMode:(NSInteger)aMode;
- (void)setAlternateColor:(id)aColor;
- (void)setColor:(id)aColor;
- (id)getURL;
- (void)setURL:(id)aURL;
- (BOOL)sendAction:(SEL)aAction to:(id)aTo;
- (void)resetCursorRects;
- (void)dealloc;
- (void)awakeFromNib;
- (id)initWithFrame:(NSRect)aFrame;
- (id)init;
- (void)_preInit;
@end

@interface CompanyLinkButton : LinkButton
{
    NSString *_link;
}
- (void)setBundleValue;
- (void)dealloc;
- (void)_preInit;
@end

@interface VersionTextField : NSTextField
{
    NSString *_version;
    NSString *_build;
}
- (void)setBundleValue;
- (void)mouseDown:(id)aDown;
- (void)dealloc;
- (void)awakeFromNib;
- (id)init;
- (void)_preInit;
@end

@interface CopyrightTextField : NSTextField
{
    NSString *_copyright;
}
- (void)setBundleValue;
- (void)awakeFromNib;
- (id)init;
- (void)_preInit;
@end

@interface AppNameTextField : NSTextField
{
    NSString *_appName;
}
- (void)setBundleValue;
- (void)dealloc;
- (void)awakeFromNib;
- (id)init;
- (void)_preInit;
@end

@interface NSBundle (BundleInfoStrings)
- (id)companyLink;
- (id)copyrightString;
- (id)buildString;
- (id)versionString;
- (id)bundleName;
@end

