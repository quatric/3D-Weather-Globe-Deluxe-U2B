#import "WeatherGlobePrefix.h"


@interface IPhotoAlbum : NSObject
{
    NSMutableArray *_photos;
    NSString *_name;
    NSString *_type;
    float _libraryVersion;
    BOOL _isMaster;
    NSImage *_lastImportImage;
    NSImage *_libraryImage;
    NSImage *_library4Image;
    NSImage *_smartAlbumImage;
    NSImage *_albumImage;
    NSImage *_specialMonthImage;
    NSImage *_specialRollImage;
    NSImage *_folderImage;
    NSImage *_bookImage;
    NSImage *_slideshowImage;
}
- (BOOL)isSlideshow;
- (BOOL)isBook;
- (BOOL)isFolder;
- (BOOL)isSmart;
- (BOOL)isSpecialMonth;
- (BOOL)isSpecialRoll;
- (BOOL)isLastImport;
- (NSInteger)compareDecrease:(id)aDecrease;
- (NSInteger)compareIncrease:(id)aIncrease;
- (id)filterPhotos:(id)aPhotos byCategory:(id)aCategory;
- (BOOL)isMaster;
- (NSInteger)sortWeight;
- (id)photoAtIndex:(NSUInteger)aIndex;
- (NSUInteger)numberOfPhotos;
- (id)subitemAtIndex:(NSUInteger)aIndex;
- (NSUInteger)numberOfSubitems;
- (id)displayName;
- (id)icon;
- (void)dealloc;
- (id)initWithDictionaryRepresentation:(id)aRepresentation photos:(id)aPhotos version:(NSInteger)aVersion;
- (id)initWithName:(id)aName dictionaryRepresentation:(id)aRepresentation photos:(id)aPhotos version:(NSInteger)aVersion;
- (id)init;
@end

