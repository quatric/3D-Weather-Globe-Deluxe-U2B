// The flat grid of an adaptive mesh LOD: z and an up normal (the ground
// elements' shared buffer) or the texture coordinates (the manager's XY
// buffer).

#include "RenderLib.h"

template <unsigned char XY, unsigned char Z, unsigned char Color, unsigned char UV>
void TAdaptiveVBFiller<XY, Z, Color, UV>::FillerExecutor::startFilling(int size, int centers, SFillerContext &c)
{
	float fsize = (float)size;
	c.size = fsize;
	c.halfStep = 0.5f / fsize;
	if (UV)
		for (int i = 0; i <= size; i++) {
			c.u[i] = (float)i / fsize;
			if (centers)
				c.uCenter[i] = (float)i / fsize + 0.5f / fsize;
		}
}

template <unsigned char XY, unsigned char Z, unsigned char Color, unsigned char UV>
void TAdaptiveVBFiller<XY, Z, Color, UV>::fillVertices_LOD(char *data, long stride, int lod, int xyOffset,
                                                           int zOffset, int colorOffset, int uvOffset, float borderZ)
{
	static_assert(!XY, "no LOD is filled with positions");
	SFillerContext context;
	context.xyOffset = xyOffset;
	context.zOffset = zOffset;
	context.colorOffset = colorOffset;
	context.uvOffset = uvOffset;
	unsigned char centers = lod & 1;
	int size = 1 << (lod >> 1);
	TAdaptiveVBCustomFiller<FillerExecutor, SFillerContext &> filler;
	filler.fillAdaptiveVertexBufferCustom(size, centers, data, stride, context);
	CBordersFiller::copyGeometryToBorders(size, centers, data, stride, Z ? zOffset : -1, borderZ);
}

template class TAdaptiveVBFiller<0, 0, 0, 1>;
template class TAdaptiveVBFiller<0, 1, 1, 0>;
