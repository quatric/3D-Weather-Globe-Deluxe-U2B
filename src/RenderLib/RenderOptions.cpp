// RenderOptions and its kinds.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(RenderOptions, 0)
LF_IMPLEMENT_CLASS(RenderOptionsFromPrefs, LF_CLASSOF(RenderOptions))
LF_IMPLEMENT_CLASS(RenderOptionsSaver, LF_CLASSOF(RenderOptionsFromPrefs))
LF_IMPLEMENT_CLASS(RenderOptionsForPrinting, LF_CLASSOF(RenderOptions))

int *RenderOptions::sCurDayNightVarPtr = 0;

RenderOptions::RenderOptions() : _recordedDayNight(false), _recordedClouds(false), _showCaptions(true)
{
}

void RenderOptions::setCurrentOptions(RenderOptions *options)
{
	sCurDayNightVarPtr = options->_dayNightIsEnabledFlagPtr();
}

void RenderOptions::storeSceneStateInStream(LData *stream, unsigned int)
{
	_recordedClouds = showClouds();
	_recordedDayNight = renderWithDayNight();
	stream->appendBytes(&_recordedDayNight, 2);
}

const void *RenderOptions::restoreSceneStateFromData(const void *data, unsigned int)
{
	memcpy(&_recordedDayNight, data, 2);
	return (const char *)data + 2;
}

RenderOptionsSaver *RenderOptionsSaver::options()
{
	return LAutorelease(RenderOptionsSaver::_alloc());
}
