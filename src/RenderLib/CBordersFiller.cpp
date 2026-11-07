// The border vertices of an adaptive mesh LOD: the top row, a left and right
// vertex for each inner row, then the bottom row.

#include "RenderLib.h"

void CBordersFiller::copyGeometryToBorders(int size, unsigned char centers, char *data, int stride, int zOffset,
                                           float borderZ)
{
	size_t rowBytes = stride * (size + 1);
	char *borders = data + (size + 1) * rowBytes;
	if (centers)
		borders += size * stride * size;
	char *right = borders + size * stride;
	char *left = right - stride;
	for (int y = 0; y <= size; y++) {
		if (y == 0 || y == size) {
			char *row = y == 0 ? borders : borders + (size * 3 - 1) * stride;
			memcpy(row, data, rowBytes);
			if (zOffset >= 0)
				for (int x = 0; x <= size; x++)
					*(float *)(row + x * stride + zOffset) = borderZ;
		} else {
			memcpy(left, data, stride);
			memcpy(right, data + size * stride, stride);
			if (zOffset >= 0) {
				*(float *)(left + zOffset) = borderZ;
				*(float *)(right + zOffset) = borderZ;
			}
		}
		data += rowBytes;
		if (y < size && centers)
			data += size * stride;
		left += stride * 2;
		right += stride * 2;
	}
}
