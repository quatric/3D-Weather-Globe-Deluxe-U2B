#import "MapDrawView.h"

@implementation MapDrawView

- (void)dealloc
{
	if (_drawer) {
		_drawer->release();
		_drawer = 0;
	}
	[super dealloc];
}

- (BaseDrawing *)drawer
{
	return _drawer;
}

- (void)setDrawer:(BaseDrawing *)aDrawer
{
	if (aDrawer)
		aDrawer->retain();
	if (_drawer) {
		_drawer->release();
		_drawer = 0;
	}
	_drawer = aDrawer;
}

- (void)drawRect:(NSRect)aRect
{
	if (_drawer)
		_drawer->drawRect(LRectFromNS(aRect), 0);
}

@end
