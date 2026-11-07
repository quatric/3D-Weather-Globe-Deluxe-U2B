// LProfiler: named observation points that sum the time spent between
// startPointProfiling and endPointProfiling, reported when the profiler goes.
// LSingleProfilerPoint logs the time of its own life. Times are in ticks
// (60ths of a second).
#pragma once

#include "LContainers.h"

namespace LF {

class LProfiler {
public:
	struct SObservationPointRecord {
		const char *name;
		uint64_t hits;
		uint64_t totalTicks;
		uint64_t startTicks;
	};

	LProfiler();
	~LProfiler();

	static LProfiler *profiler();
	static uint64_t ticksPerSecond() { return 60; }
	static uint32_t currentTicksCount();

	// Points are numbered 0..1023.
	void addObservationPoint(unsigned int point, const char *name);
	void startPointProfiling(unsigned int point);
	void endPointProfiling(unsigned int point);

	TArray<SObservationPointRecord> *_points;
	uint64_t _ticksPerSecond;
	CSharableResource _lock;

	static LProfiler *sProfiler;
};

class LSingleProfilerPoint {
public:
	LSingleProfilerPoint(const char *name);
	~LSingleProfilerPoint();

	const char *_name;
	uint64_t _startTicks;
};

} // namespace LF
