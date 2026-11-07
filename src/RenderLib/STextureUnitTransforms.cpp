// STextureUnitTransforms: where a patch lies in the texture drawn for it.

#include "RenderLib.h"

void STextureUnitTransforms::fillVSScaleOffsConstant(Vector4 *out) const
{
	out->x = scaleX;
	out->y = scaleY;
	out->z = offsetX;
	out->w = offsetY;
}

int STextureUnitTransforms::adjust(const STextureID &id, const STextureID &available, int maxLevel,
                                   unsigned char local)
{
	textureID = available;
	while (textureID.level > maxLevel && textureID.scaleDownIfPossible())
		;
	int level = textureID.level;
	// Rows of `id` in a row of the texture.
	int n = 1;
	for (int l = id.level; l > level; l--) {
		if (l == 1 || l == 2 || l == 4 || l == 5)
			n *= 3;
		else if (l == 3)
			n *= 2;
	}
	int merged = STextureID::columnsMergedInRow(level, textureID.y);
	if (local) {
		float fn = (float)n;
		scaleY = -1.0f / fn;
		scaleX = 1.0f / ((float)merged * fn);
		offsetX = (float)(id.x % (merged * n)) / ((float)merged * fn);
		offsetY = 1.0f - (float)(id.y % n) / fn;
		return id.y / n;
	}
	float rows = (float)textureRowsAtLevel(level);
	scaleY = -rows;
	scaleX = (rows + rows) / (float)merged;
	offsetX = -(float)(id.x / (merged * n));
	offsetY = (float)(id.y / n) + 1.0f;
	return id.y / n;
}
