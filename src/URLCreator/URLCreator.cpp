// URLCreator: the plug-in that makes the URLs of the weather service. The
// application loads it as a bundle (DynamicEnvironment) and looks its
// functions up by name.
//
// The original asked CustomWeather (xml.customweather.com for the reports,
// maps.customweather.com for the clouds) with MacKiev's client account. Both
// feeds are gone, so this version gives the URLs of the replacements, which
// OnlineServices (AppModel) fetches:
//   accuweather:report?type=N&lat=..&lon=..   a report from AccuWeather
//   livecloudmaps:clouds?side=west|east&size=N   half of the live cloud map

#include "LString.h"
#include "LData.h"
#include "LDate.h"

using namespace LF;

// The version DynamicEnvironment compares with a downloaded plug-in's. The
// original was 1002.
static const int kVersion = 1100;

// The reports, by the plug-in's numbers (the application's report type - 1),
// as CustomWeather named them.
static const char *const kReportStrings[] = {
	"current_conditions",
	"extended_forecast",
	"expanded_forecast",
	"astronomy_forecast",
	"severe_warning",
};
static const int kReportsCount = sizeof kReportStrings / sizeof kReportStrings[0];

extern "C" {

__attribute__((visibility("default"))) LString *GetURLForReportDownload(LPoint geoPoint, int report)
{
	if (report < 0 || report >= kReportsCount)
		return 0;
	return LString::stringWithFormat("accuweather:report?type=%d&lat=%f&lon=%f", report, (double)geoPoint.y,
	                                 (double)geoPoint.x);
}

// AccuWeather knows places by their coordinates (and its own location keys),
// not by CustomWeather's city IDs: the application then asks by coordinates.
__attribute__((visibility("default"))) LString *GetURLForReportDownloadByID(int, int)
{
	return 0;
}

// The live map is of the last hours only: the date isn't used.
__attribute__((visibility("default"))) LString *GetURLForCloudsDownload(unsigned char west, LDate *, int size)
{
	return LString::stringWithFormat("livecloudmaps:clouds?side=%s&size=%d", west ? "west" : "east", size);
}

__attribute__((visibility("default"))) LData *GetXMLForReportData(LData *data)
{
	return data;
}

__attribute__((visibility("default"))) LData *GetJPGForCloudsData(LData *data)
{
	return data;
}

__attribute__((visibility("default"))) int GetURLDLLVersion()
{
	return kVersion;
}

// The accounts of MacKiev's servers: none any more.
__attribute__((visibility("default"))) LString *GetReportUN()
{
	return 0;
}

__attribute__((visibility("default"))) LString *GetReportP()
{
	return 0;
}

__attribute__((visibility("default"))) LString *GetMapsUN()
{
	return 0;
}

__attribute__((visibility("default"))) LString *GetMapsP()
{
	return 0;
}

}
