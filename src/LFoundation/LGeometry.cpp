// Ranges, points, sizes and rectangles (the constants and the one range
// function; the rest are inline in the headers).

#include "LString.h"

namespace LF {

const LRange LZeroRange = { 0, 0 };
const LPoint LZeroPoint = { 0, 0 };
const LSize LZeroSize = { 0, 0 };
const LRect LZeroRect = { { 0, 0 }, { 0, 0 } };

// The overlap of two ranges, or LZeroRange. The original computes the ends
// in float, which is exact for the values used.
LRange LIntersectionRange(LRange a, LRange b)
{
	unsigned int loc = a.location < b.location ? b.location : a.location;
	unsigned int end = b.location + b.length;
	if (a.location + a.length < end)
		end = a.location + a.length;
	if ((float)end <= (float)loc)
		return LZeroRange;
	return LMakeRange(loc, end - loc);
}

} // namespace LF
