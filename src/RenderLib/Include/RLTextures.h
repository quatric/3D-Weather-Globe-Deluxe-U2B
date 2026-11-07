// The globe's and flat map's earth textures: the patches' DDS files, read
// in the background and copied into recycled textures, and the city lights.

#ifndef RLTEXTURES_H
#define RLTEXTURES_H

#include "AppModel.h"
#include "FHTextures.h"

// Set while a large picture is grabbed: every patch is read at once.
extern bool sLargeBBGrabbingIsActive;

// The rows of patches at a level (twice as many columns).
static inline int textureRowsAtLevel(int level)
{
	static const int rows[6] = {1, 3, 9, 18, 54, 162};
	return (unsigned)level < 6 ? rows[level] : 1;
}

// How the texture coordinates of a patch map into the texture of the patch
// (or of a coarser patch containing it) that is loaded.
struct STextureUnitTransforms {
	STextureUnitTransforms() : scaleX(0.0f), scaleY(0.0f), offsetX(0.0f), offsetY(0.0f)
	{
		textureID.x = 0;
		textureID.y = 0;
		textureID.level = 0;
	}

	// Scale and offset for the vertex shader.
	void fillVSScaleOffsConstant(Vector4 *out) const;
	// Takes the texture of `available` scaled down to `maxLevel` at most;
	// `local` is for coordinates within the patch rather than the globe's.
	// Returns the row of `id` in the texture.
	int adjust(const STextureID &id, const STextureID &available, int maxLevel, unsigned char local);

	STextureID textureID;	// the texture used
	float scaleX;
	float scaleY;	// negative: the textures are stored from the top
	float offsetX;
	float offsetY;
};

class CTextureDecompressionTask;

class CTextureLoader : public LObject, public CHolderObjectDelegate {
	LF_DECLARE_CLASS(CTextureLoader)
public:
	CTextureLoader();
	virtual void dealloc();
	virtual CTextureLoader *getOwnerLObject() { return this; }
	virtual intptr_t _textureWasDecompressed(char *args);
	virtual intptr_t _contextWasDetached(char *args);
	virtual void holderWillDie(CHolder *holder);

	CTextureLoader *initWithHolder(CHolder *holder);
	// Stops the reading and releases the loader.
	void invalidate();
	// Starts reading the patches requested in the last frame, and forgets
	// the oldest unused ones.
	void startNextFrame();
	// Marks the patch as needed; returns the finest level of it (or of a
	// patch containing it) that can be drawn now.
	int requestTextureBeforeDraw(const STextureID &id);
	static int requestGlowsTextureBeforeDraw(const STextureID &) { return 3; }
	// The patch's texture, read now if `force` and not read yet; 0 if not read.
	BaseTextureEmu *textureAtLevel(const STextureID &id, unsigned char force);
	BaseTextureEmu *nightGlowsTextureAtLevel(const STextureID &id);
	void postTextureDecompressionTask(const STextureID &id, unsigned char now);
	void freeOldestTextureData();
	LComparableSet *_waterFilesFromResourceData(LData *data);

	static LString *texturePathForID(const STextureID &id);
	static LString *nightGlowsTexturePathForID(const STextureID &id);
	static intptr_t keyForID(const STextureID &id) { return STextureIDKey(id.x, id.y, id.level); }

	static LString *kTextureIDAsInt;
	static LString *loadedSetWasUpdatedNotification;

	int _maxDecompressedCount;
	CHashSet _postedTasks;	// patches being read
	CHashSet *_requested;	// patches drawn this frame
	CHashTable _unusedSince;	// read patches not drawn, -> frame stamp
	TLIntPtrMap<CTextureDecompressionTask *> *_decompressed;
	TLIntPtrMap<C2DTexture *> *_textures;
	CHashSet *_unusedTextures;	// textures not requested since the frame began
	TLIntPtrMap<C2DTexture *> *_glowTextures;
	TLIntPtrMap<LValue *> *_glowsUnusedSince;	// -> time
	CHashSet *_usedGlows;
	C2DTexture *_emptyGlows;
	CHolder *_holder;
	LTaskQueue *_queue;
	bool _postNotifications;
	CHashTable _fileExists;	// patch -> its file is readable
};

#endif
