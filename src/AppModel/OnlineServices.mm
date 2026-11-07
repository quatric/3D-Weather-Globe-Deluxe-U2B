// OnlineServices: the services that take more than one request, which the URL
// creator plug-in gives URLs of their own schemes:
//
//   accuweather:report?type=N&lat=..&lon=..
//       A report (N is the plug-in's report number: 0 current conditions,
//       1 extended forecast, 2 expanded (seven days), 3 astronomy, 4 severe
//       warnings) from AccuWeather instead of CustomWeather, whose feeds went
//       away with MacKiev's subscription server. The location key of the point
//       comes first; the answer is turned into the CustomWeather XML that
//       CWManager parses. The API key is the AccuWeatherAPIKey user default.
//   livecloudmaps:clouds?side=west|east&size=N
//       Half of the live global cloud map of clouds.matteason.co.uk (CC0,
//       from EUMETSAT data), as the grayscale JPEG the old satellite feed sent.
//
// DownloadManager hands them a connection and gets its closed notification
// back, as from any other download.

#include "AppModel.h"
#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>

static NSString *const _kAPIKeyDefault = @"AccuWeatherAPIKey";
static NSString *const _kLocationsDefault = @"AccuWeatherLocations";
static NSString *const _kAppDomain = @"com.mackiev.3dweatherglobev2";
static NSString *const _kHost = @"https://api.accuweather.com";
// The 2048 pixel hemispheres are cut from the 4096 pixel map, so that they
// are not just the 1024 pixel ones made larger.
static NSString *const _kCloudMapURL = @"https://clouds.matteason.co.uk/images/2048x1024/clouds.jpg";
static NSString *const _kCloudMapURLLarge = @"https://clouds.matteason.co.uk/images/4096x2048/clouds.jpg";

// Answers are kept on disk for a while so that going between cities doesn't
// fetch them again.
static const NSTimeInterval _kCurrentLifetime = 10.0 * 60.0;
static const NSTimeInterval _kForecastLifetime = 60.0 * 60.0;
// The cloud map changes every three hours.
static const NSTimeInterval _kCloudMapLifetime = 30.0 * 60.0;

// What CustomWeather said for each of AccuWeather's icons (1-44): the icon
// of the application (1-35), the sky code and the precipitation code the
// globe draws the weather with.
struct SIconInfo {
	int icon;
	int sky;
	int precip;
};
static const SIconInfo _kIcons[45] = {
	{35, 0, 0},
	{1, 1, 0}, {2, 3, 0}, {4, 7, 0}, {4, 10, 0}, {3, 12, 0},	// 1-5: sunny .. hazy sunshine
	{6, 17, 0}, {7, 18, 0}, {7, 19, 0}, {35, 0, 0}, {35, 0, 0},	// 6-10: mostly cloudy, cloudy, dreary
	{9, 20, 0}, {19, 18, 9}, {18, 17, 8}, {33, 7, 4}, {22, 24, 19},	// 11-15: fog, showers, t-storms
	{22, 24, 17}, {21, 10, 16}, {20, 18, 11}, {29, 18, 41}, {29, 17, 40},	// 16-20: storms, rain, flurries
	{29, 7, 40}, {30, 18, 45}, {30, 17, 43}, {27, 18, 28}, {27, 18, 30},	// 21-25: flurries, snow, ice, sleet
	{27, 18, 32}, {35, 0, 0}, {35, 0, 0}, {28, 18, 34}, {1, 1, 0},	// 26-30: freezing rain, rain and snow, hot
	{1, 1, 0}, {11, 3, 0}, {13, 1, 0}, {14, 3, 0}, {15, 7, 0},	// 31-35: cold, windy, clear .. partly cloudy
	{15, 10, 0}, {14, 12, 0}, {17, 17, 0}, {34, 7, 4}, {34, 17, 8},	// 36-40: night clouds and showers
	{34, 10, 16}, {22, 24, 17}, {29, 17, 40}, {30, 17, 43},	// 41-44: night storms, flurries, snow
};

static NSMutableDictionary *_sPending;	// request URL -> completion blocks
static NSData *_sCloudMap;
static bool _sCloudMapLarge;
static NSDate *_sCloudMapDate;
static NSMutableArray *_sCloudMapWaiting;

static NSString *_apiKey()
{
	NSString *key = [[NSUserDefaults standardUserDefaults] stringForKey:_kAPIKeyDefault];
	if ([key length] == 0) {
		// The screen saver runs in another application's domain.
		id value = [(id)CFPreferencesCopyAppValue((CFStringRef)_kAPIKeyDefault, (CFStringRef)_kAppDomain) autorelease];
		key = [value isKindOfClass:[NSString class]] ? value : nil;
	}
	return [key length] != 0 ? key : nil;
}

static NSString *_escaped(id value)
{
	NSMutableString *string = [NSMutableString stringWithFormat:@"%@", value != nil ? value : @""];
	[string replaceOccurrencesOfString:@"&" withString:@"&amp;" options:0 range:NSMakeRange(0, [string length])];
	[string replaceOccurrencesOfString:@"<" withString:@"&lt;" options:0 range:NSMakeRange(0, [string length])];
	[string replaceOccurrencesOfString:@">" withString:@"&gt;" options:0 range:NSMakeRange(0, [string length])];
	[string replaceOccurrencesOfString:@"\"" withString:@"&quot;" options:0 range:NSMakeRange(0, [string length])];
	return string;
}

// Appends name="value" for each pair whose value is there.
static void _appendAttributes(NSMutableString *xml, NSArray *pairs)
{
	for (NSUInteger i = 0; i + 1 < [pairs count]; i += 2) {
		id value = [pairs objectAtIndex:i + 1];
		if (value != [NSNull null])
			[xml appendFormat:@" %@=\"%@\"", [pairs objectAtIndex:i], _escaped(value)];
	}
}

// The value at a dotted path of dictionaries, or NSNull (so that it can go
// in an array).
static id _path(id object, NSString *path)
{
	id value = object;
	for (NSString *part in [path componentsSeparatedByString:@"."]) {
		if (![value isKindOfClass:[NSDictionary class]])
			return [NSNull null];
		value = [value objectForKey:part];
	}
	return value != nil ? value : [NSNull null];
}

static bool _has(id value)
{
	return value != nil && value != [NSNull null];
}

static const SIconInfo *_iconInfo(id number)
{
	int icon = _has(number) ? [number intValue] : 0;
	return &_kIcons[icon >= 1 && icon <= 44 ? icon : 0];
}

// "2026-10-22T07:58:00-05:00" -> "07:58AM", the local time of the place.
static id _clockTime(id isoDate)
{
	if (![isoDate isKindOfClass:[NSString class]] || [isoDate length] < 16)
		return [NSNull null];
	int hour = [[isoDate substringWithRange:NSMakeRange(11, 2)] intValue];
	int minute = [[isoDate substringWithRange:NSMakeRange(14, 2)] intValue];
	return [NSString stringWithFormat:@"%02d:%02d%@", hour % 12 == 0 ? 12 : hour % 12, minute, hour < 12 ? @"AM" : @"PM"];
}

// 1 is Sunday, from the date part of an ISO date.
static id _dayOfWeek(id isoDate)
{
	if (![isoDate isKindOfClass:[NSString class]] || [isoDate length] < 10)
		return [NSNull null];
	NSCalendar *calendar = [[[NSCalendar alloc] initWithCalendarIdentifier:NSCalendarIdentifierGregorian] autorelease];
	[calendar setTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0]];
	NSDateComponents *components = [[[NSDateComponents alloc] init] autorelease];
	[components setYear:[[isoDate substringWithRange:NSMakeRange(0, 4)] intValue]];
	[components setMonth:[[isoDate substringWithRange:NSMakeRange(5, 2)] intValue]];
	[components setDay:[[isoDate substringWithRange:NSMakeRange(8, 2)] intValue]];
	return [NSString stringWithFormat:@"%ld",
	                 (long)[calendar component:NSCalendarUnitWeekday fromDate:[calendar dateFromComponents:components]]];
}

static NSString *_cacheFolder()
{
	NSString *caches = [NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES) firstObject];
	NSString *folder = [caches stringByAppendingPathComponent:[_kAppDomain stringByAppendingPathComponent:@"AccuWeather"]];
	[[NSFileManager defaultManager] createDirectoryAtPath:folder withIntermediateDirectories:YES attributes:nil error:NULL];
	return folder;
}

// Calls completion on the main thread with the answer, or with nil.
static void _fetchJSON(NSString *path, NSString *cacheName, NSTimeInterval lifetime, void (^completion)(id json))
{
	NSString *cachePath = cacheName ? [_cacheFolder() stringByAppendingPathComponent:cacheName] : nil;
	NSDate *modified =
	    cachePath ? [[[NSFileManager defaultManager] attributesOfItemAtPath:cachePath error:NULL] fileModificationDate] : nil;
	if (modified && -[modified timeIntervalSinceNow] < lifetime) {
		id json = [NSJSONSerialization JSONObjectWithData:[NSData dataWithContentsOfFile:cachePath] options:0 error:NULL];
		if (json) {
			dispatch_async(dispatch_get_main_queue(), ^{
				completion(json);
			});
			return;
		}
	}
	NSString *url = [NSString stringWithFormat:@"%@/%@%@apikey=%@", _kHost, path,
	                                           [path rangeOfString:@"?"].location == NSNotFound ? @"?" : @"&", _apiKey()];
	if (_sPending == nil)
		_sPending = [NSMutableDictionary new];
	NSMutableArray *waiting = [_sPending objectForKey:url];
	if (waiting) {
		[waiting addObject:[[completion copy] autorelease]];
		return;
	}
	[_sPending setObject:[NSMutableArray arrayWithObject:[[completion copy] autorelease]] forKey:url];
	[[[NSURLSession sharedSession]
	      dataTaskWithURL:[NSURL URLWithString:url]
	    completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
		    NSInteger status =
		        [response isKindOfClass:[NSHTTPURLResponse class]] ? [(NSHTTPURLResponse *)response statusCode] : 0;
		    id json = (status == 200 && data) ? [NSJSONSerialization JSONObjectWithData:data options:0 error:NULL] : nil;
		    if (json == nil)
			    NSLog(@"AccuWeather request %@ failed: HTTP %ld %@", path, (long)status,
			          error ? [error localizedDescription] : @"");
		    else if (cachePath)
			    [data writeToFile:cachePath atomically:YES];
		    dispatch_async(dispatch_get_main_queue(), ^{
			    NSArray *blocks = [[[_sPending objectForKey:url] retain] autorelease];
			    [_sPending removeObjectForKey:url];
			    for (void (^block)(id) in blocks)
				    block(json);
		    });
	    }] resume];
}

// The AccuWeather location of a point; it never changes, so it is kept in
// the defaults.
static void _locationForPoint(double lat, double lon, void (^completion)(NSDictionary *location))
{
	NSString *pointKey = [NSString stringWithFormat:@"%.2f,%.2f", lat, lon];
	NSDictionary *known = [[[NSUserDefaults standardUserDefaults] dictionaryForKey:_kLocationsDefault] objectForKey:pointKey];
	if (known) {
		completion(known);
		return;
	}
	_fetchJSON([NSString stringWithFormat:@"locations/v1/cities/geoposition/search.json?q=%@", pointKey], nil, 0,
	           ^(id json) {
		           id key = _path(json, @"Key");
		           if (!_has(key)) {
			           completion(nil);
			           return;
		           }
		           NSMutableDictionary *location = [NSMutableDictionary dictionaryWithObject:key forKey:@"Key"];
		           NSArray *fields = @[ @"City", @"LocalizedName", @"State", @"AdministrativeArea.LocalizedName",
			                            @"Country", @"Country.LocalizedName", @"Latitude", @"GeoPosition.Latitude",
			                            @"Longitude", @"GeoPosition.Longitude" ];
		           for (NSUInteger i = 0; i < [fields count]; i += 2) {
			           id value = _path(json, [fields objectAtIndex:i + 1]);
			           [location setObject:_has(value) ? [value description] : @""
			                        forKey:[fields objectAtIndex:i]];
		           }
		           NSMutableDictionary *all = [NSMutableDictionary
		               dictionaryWithDictionary:[[NSUserDefaults standardUserDefaults] dictionaryForKey:_kLocationsDefault]];
		           [all setObject:location forKey:pointKey];
		           [[NSUserDefaults standardUserDefaults] setObject:all forKey:_kLocationsDefault];
		           completion(location);
	           });
}

static NSMutableString *_reportElement(NSDictionary *location, NSString *name, id epoch)
{
	NSMutableString *xml = [NSMutableString stringWithString:@"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<report"];
	_appendAttributes(xml, @[
		@"unix_date", _has(epoch) ? epoch : [NSString stringWithFormat:@"%.0f", [[NSDate date] timeIntervalSince1970]],
		@"city_name", [location objectForKey:@"City"], @"state_name", [location objectForKey:@"State"],
		@"country_name", [location objectForKey:@"Country"], @"metric", @"true", @"name", name
	]);
	[xml appendString:@">\n"];
	return xml;
}

static NSData *_currentXML(id json, NSDictionary *location)
{
	id now = [json isKindOfClass:[NSArray class]] && [json count] != 0 ? [json objectAtIndex:0] : nil;
	if (now == nil)
		return nil;
	id text = _path(now, @"WeatherText");
	const SIconInfo *info = _iconInfo(_path(now, @"WeatherIcon"));
	NSMutableString *xml = _reportElement(location, @"current_conditions", _path(now, @"EpochTime"));
	[xml appendString:@"<observation"];
	_appendAttributes(xml, @[
		@"city", [location objectForKey:@"Key"], @"city_name", [location objectForKey:@"City"],
		@"latitude", [location objectForKey:@"Latitude"] ?: [NSNull null],
		@"longitude", [location objectForKey:@"Longitude"] ?: [NSNull null],
		@"daylight", [_path(now, @"IsDayTime") boolValue] ? @"D" : @"N", @"sky", text,
		@"description", _has(text) ? [text stringByAppendingString:@"."] : text,
		@"temperature", _path(now, @"Temperature.Metric.Value"),
		@"wind_speed", _path(now, @"Wind.Speed.Metric.Value"), @"wind_dir", _path(now, @"Wind.Direction.Degrees"),
		@"humidity", _path(now, @"RelativeHumidity"), @"dew_point", _path(now, @"DewPoint.Metric.Value"),
		@"comfort", _path(now, @"RealFeelTemperature.Metric.Value"),
		@"baro_pressure", _path(now, @"Pressure.Metric.Value"), @"visibility", _path(now, @"Visibility.Metric.Value"),
		@"icon", [NSString stringWithFormat:@"%d", info->icon], @"sky_desc", [NSString stringWithFormat:@"%d", info->sky],
		@"precip_desc", [NSString stringWithFormat:@"%d", info->precip]
	]);
	[xml appendString:@" />\n</report>\n"];
	return [xml dataUsingEncoding:NSUTF8StringEncoding];
}

static id _uvForDay(id day, NSString *key)
{
	id items = _path(day, @"AirAndPollen");
	if ([items isKindOfClass:[NSArray class]]) {
		for (id item in items)
			if ([_path(item, @"Name") isEqual:@"UVIndex"])
				return _path(item, key);
	}
	return [NSNull null];
}

static NSData *_forecastXML(id json, NSDictionary *location, int report)
{
	NSArray *days = _path(json, @"DailyForecasts");
	if (![days isKindOfClass:[NSArray class]] || [days count] == 0)
		return nil;
	NSMutableString *xml;
	if (report == 2) {
		xml = _reportElement(location, @"expanded_forecast", nil);
		[xml appendString:@"<location"];
		_appendAttributes(xml, @[ @"city", [location objectForKey:@"Key"], @"city_name", [location objectForKey:@"City"] ]);
		[xml appendString:@">\n"];
		for (NSUInteger i = 0; i < [days count] && i < 7; i++) {
			id day = [days objectAtIndex:i];
			id text = _path(day, @"Day.IconPhrase");
			const SIconInfo *info = _iconInfo(_path(day, @"Day.Icon"));
			[xml appendString:@"<forecast"];
			_appendAttributes(xml, @[
				@"day_of_week", _dayOfWeek(_path(day, @"Date")),
				@"low_temp", _path(day, @"Temperature.Minimum.Value"),
				@"high_temp", _path(day, @"Temperature.Maximum.Value"),
				@"precip_prob", _path(day, @"Day.PrecipitationProbability"),
				@"rainfall", _path(day, @"Day.Rain.Value"), @"snowfall", _path(day, @"Day.Snow.Value"),
				@"uv_index", _uvForDay(day, @"Value"), @"uv", _uvForDay(day, @"Category"),
				@"icon", [NSString stringWithFormat:@"%d", info->icon],
				@"sky_desc", [NSString stringWithFormat:@"%d", info->sky],
				@"precip_desc", [NSString stringWithFormat:@"%d", info->precip],
				@"wind_speed", _path(day, @"Day.Wind.Speed.Value"), @"wind_dir", _path(day, @"Day.Wind.Direction.Degrees"),
				@"humidity", _path(day, @"Day.RelativeHumidity.Average"), @"sky", text,
				@"description", _has(text) ? [text stringByAppendingString:@"."] : text,
				@"comfort", _path(day, @"RealFeelTemperature.Maximum.Value")
			]);
			[xml appendString:@" />\n"];
		}
		[xml appendString:@"</location>\n</report>\n"];
	} else if (report == 3) {
		id today = [days objectAtIndex:0];
		id tomorrow = [days count] > 1 ? [days objectAtIndex:1] : nil;
		// The moon can rise or set after midnight; then the next day has it.
		id moonrise = _clockTime(_path(today, @"Moon.Rise"));
		if (!_has(moonrise))
			moonrise = _clockTime(_path(tomorrow, @"Moon.Rise"));
		id moonset = _clockTime(_path(today, @"Moon.Set"));
		if (!_has(moonset))
			moonset = _clockTime(_path(tomorrow, @"Moon.Set"));
		id sunrise = _clockTime(_path(today, @"Sun.Rise"));
		id sunset = _clockTime(_path(today, @"Sun.Set"));
		xml = _reportElement(location, @"astronomy_forecast", nil);
		[xml appendString:@"<forecast"];
		_appendAttributes(xml, @[
			@"city", [location objectForKey:@"Key"], @"city_name", [location objectForKey:@"City"],
			@"sunrise", _has(sunrise) ? sunrise : @"", @"sunset", _has(sunset) ? sunset : @"",
			@"moonrise", _has(moonrise) ? moonrise : @"", @"moonset", _has(moonset) ? moonset : @"",
			@"moon_phase", [NSString stringWithFormat:@"%.2f", [_path(today, @"Moon.Age") doubleValue] / 29.53]
		]);
		[xml appendString:@" />\n</report>\n"];
	} else {
		// Nothing parses the extended forecast and the severe warnings.
		xml = _reportElement(location, report == 1 ? @"extended_forecast" : @"severe_warning", nil);
		[xml appendString:@"</report>\n"];
	}
	return [xml dataUsingEncoding:NSUTF8StringEncoding];
}

static void _loadReport(int report, double lat, double lon, void (^completion)(NSData *xml))
{
	if (_apiKey() == nil) {
		NSLog(@"AccuWeather: no API key in the %@ default", _kAPIKeyDefault);
		dispatch_async(dispatch_get_main_queue(), ^{
			completion(nil);
		});
		return;
	}
	_locationForPoint(lat, lon, ^(NSDictionary *location) {
		NSString *key = [location objectForKey:@"Key"];
		if (key == nil) {
			completion(nil);
			return;
		}
		if (report == 0) {
			_fetchJSON([NSString stringWithFormat:@"currentconditions/v1/%@.json?details=true", key],
			           [key stringByAppendingString:@"-current.json"], _kCurrentLifetime, ^(id json) {
				           completion(_currentXML(json, location));
			           });
		} else {
			_fetchJSON([NSString stringWithFormat:@"forecasts/v1/daily/10day/%@.json?details=true&metric=true", key],
			           [key stringByAppendingString:@"-daily.json"], _kForecastLifetime, ^(id json) {
				           completion(_forecastXML(json, location, report));
			           });
		}
	});
}

// Clear sky is mid gray in these maps and black in the old feed's, which the
// globe draws as cloud opacity; the curve keeps clear sky transparent.
static unsigned char _cloudLevel(int value)
{
	double v = (value - 120) / 135.0;
	if (v <= 0.0)
		return 0;
	return (unsigned char)(v >= 1.0 ? 255 : v * v * 255.0 + 0.5);
}

static NSData *_hemisphere(NSData *map, bool east, NSInteger size)
{
	CGImageSourceRef source = CGImageSourceCreateWithData((CFDataRef)map, NULL);
	CGImageRef image = source ? CGImageSourceCreateImageAtIndex(source, 0, NULL) : NULL;
	CGColorSpaceRef gray = CGColorSpaceCreateDeviceGray();
	CGContextRef context = CGBitmapContextCreate(NULL, size, size, 8, size, gray, (CGBitmapInfo)kCGImageAlphaNone);
	NSMutableData *jpeg = nil;
	if (source)
		CFRelease(source);
	if (image && context) {
		unsigned char *pixels = (unsigned char *)CGBitmapContextGetData(context);
		unsigned char table[256];
		// The whole map drawn twice as wide, so that one half fills the context.
		CGContextSetInterpolationQuality(context, kCGInterpolationHigh);
		CGContextDrawImage(context, CGRectMake(east ? -size : 0, 0, size * 2, size), image);
		for (int i = 0; i < 256; i++)
			table[i] = _cloudLevel(i);
		for (NSInteger i = 0; i < size * size; i++)
			pixels[i] = table[pixels[i]];
		CGImageRef half = CGBitmapContextCreateImage(context);
		jpeg = [NSMutableData data];
		CGImageDestinationRef destination =
		    CGImageDestinationCreateWithData((CFMutableDataRef)jpeg, CFSTR("public.jpeg"), 1, NULL);
		if (destination) {
			CGImageDestinationAddImage(destination, half, (CFDictionaryRef) @{
				(id)kCGImageDestinationLossyCompressionQuality : @0.85
			});
			if (!CGImageDestinationFinalize(destination))
				jpeg = nil;
			CFRelease(destination);
		} else {
			jpeg = nil;
		}
		CGImageRelease(half);
	}
	if (context)
		CGContextRelease(context);
	CGColorSpaceRelease(gray);
	if (image)
		CGImageRelease(image);
	return jpeg;
}

static void _deliverHemisphere(bool east, NSInteger size, void (^completion)(NSData *jpeg))
{
	NSData *map = [[_sCloudMap retain] autorelease];
	if (map == nil) {
		completion(nil);
		return;
	}
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		NSAutoreleasePool *pool = [NSAutoreleasePool new];
		NSData *jpeg = [_hemisphere(map, east, size) retain];
		dispatch_async(dispatch_get_main_queue(), ^{
			completion(jpeg);
			[jpeg release];
		});
		[pool release];
	});
}

static void _loadCloudMap(bool east, NSInteger size, void (^completion)(NSData *jpeg))
{
	bool large = size > 1024;
	void (^request)(void) = [[^{
		_deliverHemisphere(east, size, completion);
	} copy] autorelease];
	if (_sCloudMap && _sCloudMapLarge == large && -[_sCloudMapDate timeIntervalSinceNow] < _kCloudMapLifetime) {
		request();
		return;
	}
	if (_sCloudMapWaiting) {
		[_sCloudMapWaiting addObject:request];
		return;
	}
	_sCloudMapWaiting = [[NSMutableArray alloc] initWithObjects:request, nil];
	[[[NSURLSession sharedSession]
	      dataTaskWithURL:[NSURL URLWithString:large ? _kCloudMapURLLarge : _kCloudMapURL]
	    completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
		    NSInteger status =
		        [response isKindOfClass:[NSHTTPURLResponse class]] ? [(NSHTTPURLResponse *)response statusCode] : 0;
		    dispatch_async(dispatch_get_main_queue(), ^{
			    NSArray *waiting = [_sCloudMapWaiting autorelease];
			    _sCloudMapWaiting = nil;
			    if (status == 200 && [data length] != 0) {
				    [_sCloudMap release];
				    _sCloudMap = [data retain];
				    _sCloudMapLarge = large;
				    [_sCloudMapDate release];
				    _sCloudMapDate = [NSDate new];
			    } else {
				    NSLog(@"Live cloud map request failed: HTTP %ld %@", (long)status,
				          error ? [error localizedDescription] : @"");
			    }
			    for (void (^block)(void) in waiting)
				    block();
		    });
	    }] resume];
}

static NSDictionary *_queryOf(NSURLComponents *components)
{
	NSMutableDictionary *query = [NSMutableDictionary dictionary];
	for (NSURLQueryItem *item in [components queryItems])
		if ([item value])
			[query setObject:[item value] forKey:[item name]];
	return query;
}

bool OnlineServices::handlesURL(LString *url)
{
	return url->hasPrefix(LString::stringWithCString("accuweather:")) ||
	       url->hasPrefix(LString::stringWithCString("livecloudmaps:"));
}

// Posts the connection's end as LConnection does.
static void _finish(LConnection *connection, NSData *data)
{
	LAutoreleasePool::addNewPoolToTop();
	LData *body = data ? LData::dataWithBytes([data bytes], (unsigned int)[data length]) : 0;
	int status = data ? 200 : 404;
	LMap *info = LMap::mapWithObjectsAndKeys(LValue::valueWithInt(status), LConnection::connectionLastErrorKey, body,
	                                         LConnection::connectionDataKey, (LObject *)0);
	LNotificationCenter::center()->postNotification(LConnection::connectionClosedNotification, connection, info);
	connection->release();
	LAutoreleasePool::removeTopPool();
}

void OnlineServices::fetch(LString *url, LConnection *connection)
{
	NSURLComponents *components = [NSURLComponents componentsWithString:(NSString *)url->cfString()];
	NSDictionary *query = _queryOf(components);
	connection->retain();
	if ([[components scheme] isEqualToString:@"accuweather"]) {
		_loadReport([[query objectForKey:@"type"] intValue], [[query objectForKey:@"lat"] doubleValue],
		            [[query objectForKey:@"lon"] doubleValue], ^(NSData *xml) {
			            _finish(connection, xml);
		            });
	} else {
		NSInteger size = [[query objectForKey:@"size"] integerValue];
		_loadCloudMap([[query objectForKey:@"side"] isEqualToString:@"east"], size > 0 ? size : 1024, ^(NSData *jpeg) {
			_finish(connection, jpeg);
		});
	}
}
