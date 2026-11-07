#import "WeatherGlobePrefix.h"


@interface WGAutoAdjustableTextField : NSTextField
{
    id<WGRebuildLayoutProtocol> _owner;
    float _initialHeight;
    BOOL _wasInitialHeightInit;
    NSText *_currentEditor;
}
- (NSSize)sizeForProposedSize:(NSSize)aSize;
- (void)_calcInitialHeight;
- (void)adjustSize;
- (id)currentEditor;
- (void)textDidChange:(id)aChange;
- (void)setOwner:(id)aOwner;
- (void)dealloc;
- (void)awakeFromNib;
- (id)initWithFrame:(NSRect)aFrame;
@end

