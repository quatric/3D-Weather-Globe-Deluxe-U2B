// HeightsManager: the terrain heights, loaded on first use from the
// "Heights.zraw" resource (zlib'd 16-bit samples, 2h x h, 0x8000 for the
// sea level).

#include "AppModel.h"

class _hmCleaner : public LCleanupTask {
public:
	virtual void performCleanup() { HeightsManager::manager()->cleanup(); }
};

static _hmCleaner _sHMCleaner;

HeightsManager::~HeightsManager()
{
	cleanup();
}

void HeightsManager::cleanup()
{
	if (_data) {
		_data->release();
		_data = 0;
	}
}

HeightsManager *HeightsManager::manager()
{
	static HeightsManager hm;
	return &hm;
}

int HeightsManager::quickHeightForMapPoint(LPoint mapPoint)
{
	HeightsManager *hm = manager();
	if (hm->_data == 0) {
		hm->_data = LData::_alloc()->initWithResource(LString::stringWithCString("Heights"),
		                                               LString::stringWithCString("zraw"), getAppModelModule());
		if (hm->_data) {
			hm->_data->expand();
			float h = sqrtf((float)hm->_data->length() * 0.25f);
			hm->_height = (int)h;
			hm->_width = (int)(h + h);
			hm->_xScale = (int)(h + h);
			hm->_samples = (const unsigned short *)hm->_data->bytes();
		}
		if (hm->_data == 0)
			return 0;
	}
	int x = (int)(mapPoint.x * (float)hm->_xScale + 0.49f);
	int y = (int)((1.0f - mapPoint.y) * (float)hm->_height + 0.49f) - 1;
	// The original clamped to 2 * width - 1 and width - 1, past the samples.
	if (x < 0)
		x = 0;
	else if (x > hm->_width - 1)
		x = hm->_width - 1;
	if (y < 0)
		y = 0;
	else if (y > hm->_height - 1)
		y = hm->_height - 1;
	return (int)hm->_samples[x + y * hm->_width] - 0x8000;
}

// The mean of the heights above the sea level in a top-level patch (the map
// starts at 180 degrees west, the patches at 0), in globe radii.
float HeightsManager::zOffsetForElementWithID(const STextureID &id)
{
	if (id.level != 5)
		return 0;
	HeightsManager *hm = manager();
	quickHeightForMapPoint(LZeroPoint);
	if (hm->_samples == 0)
		return 0;
	int column = id.x < 162 ? id.x + 162 : id.x - 162;
	int width = hm->_width;
	int n = width / 324;
	int sum = 0;
	for (int i = 0; i < n; i++) {
		const unsigned short *p = hm->_samples + (i + (width * (161 - id.y)) / 324) * width + (column * width) / 324;
		for (int j = 0; j < n; j++, p++) {
			unsigned int v = *p;
			sum += v > 0x7fff ? (int)v - 0x8000 : 0;
		}
	}
	return (float)(sum / (n * n)) * 4.7699996e-07f;
}
