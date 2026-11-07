// LProfiler
//
// LF::LProfiler and LF::LSingleProfilerPoint.

#include "LProfiler.h"
#include <mach/mach_time.h>

namespace LF {

LProfiler *LProfiler::sProfiler = 0;

// TickCount() of the original.
uint32_t LProfiler::currentTicksCount()
{
	static mach_timebase_info_data_t timebase;
	if (timebase.denom == 0)
		mach_timebase_info(&timebase);
	uint64_t ns = mach_absolute_time() * timebase.numer / timebase.denom;
	return (uint32_t)(ns * 60 / 1000000000ull);
}

LProfiler::LProfiler() : _points(0), _ticksPerSecond(60)
{
	sProfiler = this;
}

// Reports the average time of each point.
LProfiler::~LProfiler()
{
	sProfiler = 0;
	{
		StSharableLocker locker(&_lock);
		int n = _points ? _points->count() : 0;
		for (int i = 0; i < n; i++) {
			SObservationPointRecord &r = (*_points)[i];
			if (r.name == 0)
				continue;
			if (r.hits == 0)
				LLog("%s: num hits=0\n", r.name);
			else
				LLog("%s: num hits=%d, average time=%.02f mSecs\n", r.name, (int)r.hits,
				    (double)(float)((double)r.totalTicks / (double)_ticksPerSecond / (double)r.hits) * 1000.0);
		}
	}
	delete _points;
}

LProfiler *LProfiler::profiler()
{
	static LProfiler prof;
	return &prof;
}

void LProfiler::addObservationPoint(unsigned int point, const char *name)
{
	StSharableLocker locker(&_lock);
	if (_points == 0) {
		_points = new TArray<SObservationPointRecord>;
		_points->preserveCapacity(1024);
		SObservationPointRecord empty = {0, 0, 0, 0};
		for (int i = 0; i < 1024; i++)
			_points->addObject(empty);
	}
	if ((int)point >= 0 && (int)point < (int)_points->count())
		(*_points)[point].name = name;
}

void LProfiler::startPointProfiling(unsigned int point)
{
	StSharableLocker locker(&_lock);
	if (_points && (int)point >= 0 && (int)point < (int)_points->count())
		(*_points)[point].startTicks = currentTicksCount();
}

// (The original never counted the hits, so it always reported none.)
void LProfiler::endPointProfiling(unsigned int point)
{
	StSharableLocker locker(&_lock);
	if (_points && (int)point >= 0 && (int)point < (int)_points->count()) {
		SObservationPointRecord &r = (*_points)[point];
		r.totalTicks += (uint32_t)(currentTicksCount() - (uint32_t)r.startTicks);
		r.hits++;
	}
}

LSingleProfilerPoint::LSingleProfilerPoint(const char *name) : _name(name), _startTicks(LProfiler::currentTicksCount())
{
}

LSingleProfilerPoint::~LSingleProfilerPoint()
{
	uint32_t elapsed = LProfiler::currentTicksCount() - (uint32_t)_startTicks;
	LLog("PROFILING: %s - time spent: %02f mSecs\n", _name, (double)elapsed * 1000.0 / 60.0);
}

} // namespace LF
