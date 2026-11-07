#import "SMKControlsPrefix.h"


@interface SMKSearchField : NSTextField
{
    unsigned int _reserved1;
    unsigned int _reserved2;
    unsigned int _reserved3;
    unsigned int _reserved4;
}
+ (void)initialize;
- (void)encodeWithCoder:(id)aCoder;
- (id)initWithCoder:(id)aCoder;
- (void)mouseDown:(id)aDown;
- (id)nsSearchField;
- (void)setRecentSearches:(id)aSearches;
- (id)recentSearches;
- (void)setSearchMenuTemplate:(id)aTemplate;
- (id)searchMenuTemplate;
@end

