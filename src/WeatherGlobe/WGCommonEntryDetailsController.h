#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsController.h"


@interface WGCommonEntryDetailsController : WGEntryDetailsController
{
}
- (void)setIsEditing:(BOOL)aEditing;
- (void)setCurrentEntry:(GenericEntry *)aEntry openPane:(BOOL)aPane mode:(NSInteger)aMode;
- (BOOL)isGotoLocationEnabled;
- (id)infoController;
- (id)panesControllersClasses;
@end

