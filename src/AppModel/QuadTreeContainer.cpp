// QuadTreeContainer: flags over the unit square at every level of detail,
// used for the outline lookup grids and the visible-area searches.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(QuadTreeContainer, 0)
LF_IMPLEMENT_CLASS(CProjection, 0)

void CProjection::dealloc()
{
	LObject::dealloc();
}

// Where each level starts in the buffer: the sum of the smaller levels' sizes.
static const int _sLevelOffsets[13] = { 0,      1,      5,       0x15,    0x55,     0x155,
	                                0x555,  0x1555, 0x5555, 0x15555, 0x55555, 0x155555, 0x555555 };

typedef QuadTreeContainer::EntryID EntryID;

QuadTreeContainer::QuadTreeContainer() : _logSize(0), _buffer(0), _downsampled(false)
{
}

QuadTreeContainer *QuadTreeContainer::initWithLogSize(int logSize)
{
	if (!LObject::init())
		return 0;
	_logSize = logSize;
	return this;
}

void QuadTreeContainer::dealloc()
{
	delete _buffer;
	_buffer = 0;
	LObject::dealloc();
}

int QuadTreeContainer::bufferOffset(const EntryID &e)
{
	return e.x() + _sLevelOffsets[e.level()] + (e.y() << e.level());
}

unsigned char *QuadTreeContainer::elementPtr(const EntryID &e)
{
	if (_buffer == 0)
		_buffer = new StPointerBlock(_sLevelOffsets[_logSize + 1], true);
	return (unsigned char *)_buffer->bytes() + bufferOffset(e);
}

static inline float clampUnit(float v)
{
	if (!(v >= 0.0f))
		return 0.0f;
	return v <= 0.99999f ? v : 0.99999f;
}

unsigned char *QuadTreeContainer::elementPtr(const LPoint &p)
{
	float n = (float)(1 << _logSize);
	EntryID e(_logSize, (int)(clampUnit(p.x) * n), (int)(n * clampUnit(p.y)));
	return elementPtr(e);
}

// Grows a rectangle of visible cells around one: along its row, then along
// its column.
LRect QuadTreeContainer::findLargestVisibleRect(const EntryID &entry, IEntryVisibilityTester *tester,
                                                const void *context)
{
	int level = entry.level();
	int n = 1 << level;
	int max = n - 1;
	int x = entry.x(), y = entry.y();
	struct Visible {
		QuadTreeContainer *tree;
		IEntryVisibilityTester *tester;
		const void *context;
		bool operator()(const EntryID &e)
		{
			return *tree->elementPtr(e) != 0 && (tester == 0 || tester->isEntryVisible(e, context));
		}
	} visible = { this, tester, context };
	int left = x;
	while (left > 0 && visible(EntryID(level, left - 1, y)))
		left--;
	int right = x;
	while (right < max && visible(EntryID(level, right + 1, y)))
		right++;
	int top = y;
	while (top >= 1 && visible(EntryID(level, x, top - 1)))
		top--;
	int bottom = y;
	while (bottom < max && visible(EntryID(level, x, bottom + 1)))
		bottom++;
	float cell = 1.0f / (float)n;
	LRect r;
	r.origin.x = cell * (float)left;
	r.origin.y = (float)top * cell;
	r.size.width = (float)(right - left + 1) * cell;
	r.size.height = (float)(bottom - top + 1) * cell;
	return r;
}

// The squared distance, in target-level cells, from the target to the nearest
// visible cell inside an area; -1 when there is none.
int QuadTreeContainer::_nearestDistInElemArea(const EntryID &target, const EntryID &area, EntryID &found,
                                              IEntryVisibilityTester *tester, const void *context)
{
	if (*elementPtr(area) == 0 || (tester != 0 && !tester->isEntryVisible(area, context)))
		return -1;
	if (area.level() >= target.level()) {
		int dx = area.x() - target.x();
		int dy = area.y() - target.y();
		found = area;
		return dx * dx + dy * dy;
	}
	int level = area.level() + 1;
	int x = area.x() * 2, y = area.y() * 2;
	EntryID children[4] = { EntryID(level, x, y), EntryID(level, x + 1, y), EntryID(level, x, y + 1),
		                EntryID(level, x + 1, y + 1) };
	EntryID founds[4];
	int dists[4];
	for (int i = 0; i < 4; i++)
		dists[i] = _nearestDistInElemArea(target, children[i], founds[i], tester, context);
	int best = -1, bestDist = 0x7fffffff;
	for (int i = 0; i < 4; i++)
		if (dists[i] > -1 && dists[i] < bestDist) {
			best = i;
			bestDist = dists[i];
		}
	if (best < 0)
		return -1;
	found = founds[best];
	return dists[best];
}

int QuadTreeContainer::findNearestVisibleElemID(const EntryID &target, EntryID &area, IEntryVisibilityTester *tester,
                                                const void *context)
{
	if (*elementPtr(area) == 0)
		return -1;
	if (area.level() >= target.level())
		return 0;
	EntryID found;
	int dist = _nearestDistInElemArea(target, area, found, tester, context);
	area = dist > -1 ? found : target;
	return dist;
}

// Sets the cells the rectangle covers at the finest level.
void QuadTreeContainer::setElementsInRect(LRect r, unsigned char value)
{
	int level = _logSize;
	float n = (float)(1 << level);
	float x0 = (r.origin.x >= 0.0f || r.origin.x != r.origin.x) ? (r.origin.x > 0.99999f ? 0.99999f : r.origin.x)
	                                                            : 0.0f;
	float y0 = r.origin.y >= 0.0f ? (r.origin.y > 0.99999f ? 0.99999f : r.origin.y) : 0.0f;
	float y1 = r.size.height + y0 <= 0.99999f ? r.size.height + y0 : 0.99999f;
	float x1 = r.size.width + x0 <= 0.99999f ? r.size.width + x0 : 0.99999f;
	int base = _sLevelOffsets[level];
	if (_buffer == 0)
		elementPtr(EntryID());
	unsigned char *bytes = (unsigned char *)_buffer->bytes();
	int cx0 = (int)(x0 * n), cy0 = (int)(y0 * n);
	for (int j = 0; j < (int)(y1 * n) - cy0 + 1; j++)
		for (int i = 0; i < (int)(x1 * n) - cx0 + 1; i++)
			bytes[i + cx0 + base + (cy0 << level) + (j << level)] = value;
}

// Fills the coarser levels: a cell is set when any of its four children is.
void QuadTreeContainer::downsampleIfNeeded()
{
	if (_downsampled)
		return;
	for (int level = _logSize; level > 0; level--) {
		int parentSize = 1 << (level - 1);
		int childSize = 1 << level;
		for (int y = 0; y < parentSize; y++)
			for (int x = 0; x < parentSize; x++) {
				unsigned char *child = elementPtr(EntryID(level, x * 2, y * 2));
				unsigned char *parent = elementPtr(EntryID(level - 1, x, y));
				*parent = (child[0] || child[1] || child[childSize] || child[childSize + 1]) ? 1 : 0;
			}
	}
	_downsampled = true;
}

void QuadTreeContainer::setAllChildrenTo(const EntryID &e, unsigned char value)
{
	*elementPtr(e) = value;
	if (e.level() < _logSize) {
		int level = e.level() + 1;
		int x = e.x() * 2, y = e.y() * 2;
		setAllChildrenTo(EntryID(level, x, y), value);
		setAllChildrenTo(EntryID(level, x + 1, y), value);
		setAllChildrenTo(EntryID(level, x, y + 1), value);
		setAllChildrenTo(EntryID(level, x + 1, y + 1), value);
	}
}
