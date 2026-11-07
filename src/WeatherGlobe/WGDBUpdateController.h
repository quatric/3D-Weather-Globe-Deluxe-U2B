#import "WeatherGlobePrefix.h"
#import "WGNibOwnerController.h"


@interface WGDBUpdateController : WGNibOwnerController
{
    NSWindow *_mainWindow;
    NSTableView *_descriptionsTV;
    NSWindow *_downloadWnd;
    NSTextField *_downloadStatus;
    NSProgressIndicator *_downloadProgr;
    NSButton *_downloadButt;
    int _allUpdateFilesCount;
    int _curDwnldFileIdx;
    unsigned int _curDwnldFileSize;
    NSString *_currentDownloadedFile;
    int _buttAction;
    LMap *_descriptions;
    LComparableSet *_excludedFiles;
}
+ (id)sharedInstance;
+ (void)_appWillTerminate;
- (void)tableView:(id)aView willDisplayCell:(id)aCell forTableColumn:(id)aColumn row:(NSInteger)aRow;
- (id)tableView:(id)aView objectValueForTableColumn:(id)aColumn row:(NSInteger)aRow;
- (NSInteger)numberOfRowsInTableView:(id)aView;
- (void)cancelAction:(id)aAction;
- (void)updateAction:(id)aAction;
- (void)checkboxColumnAction:(id)aAction;
- (void)downloadWindowAction:(id)aAction;
- (NSInteger)errorWithCodeCatched:(NSInteger)aCatched data:(LData *)aData;
- (NSInteger)fileWasDownloadedWithName:(id)aName error:(NSInteger)aError;
- (BOOL)fileWillBeDownloadedWithName:(id)aName connection:(LConnection *)aConnection;
- (void)updateWasFinished:(BOOL)aFinished;
- (void)updateWillStartWithFilesCount:(NSInteger)aCount;
- (void)_onContentDataDownloaded:(id)aDownloaded;
- (void)_onContentLengthUpdated:(id)aUpdated;
- (void)_bgrDBRescanComplete;
- (void)updateWithDescriptions:(LMap *)aDescriptions;
- (id)nibName;
- (void)dealloc;
- (void)awakeFromNib;
@end

