#import "WeatherGlobePrefix.h"


@interface WGiMovieExporter : NSObject
{
    int _exportType;
}
+ (id)iMovieExporterWithParams:(NSInteger)aParams;
- (BOOL)exportMovieToFileAtPath:(id)aPath fromTmpPath:(id)aPath2;
- (id)initWithParams:(NSInteger)aParams;
@end

