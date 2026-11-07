// The coarser LOD's value of each vertex field, for the LOD morph: each grid
// vertex takes the field of the vertex at the offset its position (mod 4) in
// the grid gives; the border vertices keep their own.

#include "RenderLib.h"

template <unsigned char XY, unsigned char Z, unsigned char Color, unsigned char UV>
static inline void copyDegradedFields(char *&z, char *&color, char *&uv, int from, long stride)
{
	if (Z) {
		*(int *)(z + 4) = *(int *)(z + from);
		z += stride;
	}
	if (Color) {
		*(int *)(color + 4) = *(int *)(color + from);
		color += stride;
	}
	if (UV) {
		*(int *)(uv + 8) = *(int *)(uv + from);
		*(int *)(uv + 12) = *(int *)(uv + from + 4);
		uv += stride;
	}
}

template <unsigned char XY, unsigned char Z, unsigned char Color, unsigned char UV>
static inline void copyBorderFields(char *z, char *color, char *uv, int size, long stride)
{
	for (int i = 0; i < size * 4; i++)
		copyDegradedFields<XY, Z, Color, UV>(z, color, uv, 0, stride);
}

template <unsigned char XY, unsigned char Z, unsigned char Color, unsigned char UV>
void TAdaptiveVBDegradedFiller<XY, Z, Color, UV>::fillVertices_LOD_2n(int lod, long stride, char *, char *z,
                                                                     char *color, char *uv)
{
	static_assert(!XY, "no LOD is filled with positions");
	int size = 1 << (lod >> 1);
	int s = (int)stride;
	int offsets[16] = {
		0, s, 0, -s,
		(size + 1) * s, (size + 2) * s, ~size * s, size * s,
		0, -s, 0, s,
		~size * s, -s * size, (size + 1) * s, (-2 - size) * s,
	};
	for (int y = 0; y <= size; y++)
		for (int x = 0; x <= size; x++)
			copyDegradedFields<XY, Z, Color, UV>(z, color, uv, offsets[(x & 3) + (y & 3) * 4], stride);
	copyBorderFields<XY, Z, Color, UV>(z, color, uv, size, stride);
}

template <unsigned char XY, unsigned char Z, unsigned char Color, unsigned char UV>
void TAdaptiveVBDegradedFiller<XY, Z, Color, UV>::fillVertices_LOD_2n_plus_1(int lod, long stride, char *, char *z,
                                                                            char *color, char *uv)
{
	static_assert(!XY, "no LOD is filled with positions");
	int size = 1 << (lod >> 1);
	int s = (int)stride;
	int row = (size * 2 + 1) * s;	// a row and its centers
	int offsets[16] = {
		0, s, 0, -s,
		row, 0, -row, 0,
		0, -s, 0, s,
		-row, 0, row, 0,
	};
	int centerOffsets[4] = {(size + 1) * s, size * s, -s * size, ~size * s};
	for (int y = 0; y <= size; y++) {
		for (int x = 0; x <= size; x++)
			copyDegradedFields<XY, Z, Color, UV>(z, color, uv, offsets[(x & 3) + (y & 3) * 4], stride);
		if (y < size)
			for (int x = 0; x < size; x++)
				copyDegradedFields<XY, Z, Color, UV>(z, color, uv, centerOffsets[(x & 1) + (y & 1) * 2], stride);
	}
	copyBorderFields<XY, Z, Color, UV>(z, color, uv, size, stride);
}

template class TAdaptiveVBDegradedFiller<0, 0, 0, 1>;
template class TAdaptiveVBDegradedFiller<0, 1, 1, 0>;
