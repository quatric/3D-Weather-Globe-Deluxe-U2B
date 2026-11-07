// Sounds played with OpenAL: buffers loaded from files and the sources
// playing them, and the listener.

#ifndef FHSOUND_H
#define FHSOUND_H

#include "FHDeviceEmu.h"

namespace CSWrapper {
struct _opaqueBuffer;
struct _opaqueSource;

bool initRenderingContext();
void closeRenderingContext();
void setListenerPosition(const Vector3 *position);
void setListenerOrientation(const Vector3 *orientation);
void setListenerVolume(float volume);
_opaqueBuffer *createBufferFromFile(LString *path);
void deleteBuffer(_opaqueBuffer *buffer);
_opaqueSource *createSource();
void deleteSource(_opaqueSource *source);
void setBufferForSource(_opaqueSource *source, _opaqueBuffer *buffer);
void setSourceVolume(_opaqueSource *source, float volume);
void setSourcePitch(_opaqueSource *source, float pitch);
void setSourcePosition(_opaqueSource *source, const Vector3 *position);
void setSourceLooped(_opaqueSource *source, unsigned char looped);
void sourcePlay(_opaqueSource *source);
void sourceStop(_opaqueSource *source);
void sourcePause(_opaqueSource *source);
void sourceResume(_opaqueSource *source);
} // namespace CSWrapper

class SoundBuffer;
class SoundSource;

// The singleton opens the audio device.
class SoundManager : public LObject {
	LF_DECLARE_CLASS(SoundManager)
public:
	virtual void initAsSingleton(void *param);
	virtual void dealloc();

	static SoundManager *manager() { return (SoundManager *)LF_CLASSOF(SoundManager)->singleton(); }
	void setListenerVolume(float volume);
	// Autoreleased.
	SoundBuffer *detachSoundBuffer(LString *path);
};

class SoundBuffer : public LObject {
	LF_DECLARE_CLASS(SoundBuffer)
public:
	SoundBuffer() : _manager(0), _buffer(0) {}
	virtual void dealloc();

	SoundBuffer *initWithPath(LString *path, SoundManager *manager);
	// Autoreleased.
	SoundSource *detachSoundSource();

	SoundManager *_manager;
	CSWrapper::_opaqueBuffer *_buffer;
};

class SoundSource : public LObject {
	LF_DECLARE_CLASS(SoundSource)
public:
	SoundSource() : _source(0), _buffer(0), _playing(false), _pausedByVolume(false) {}
	virtual void dealloc();

	SoundSource *initWithBuffer(SoundBuffer *buffer);
	// A nearly silent source is paused.
	void setVolume(float volume);
	void setPitch(float pitch);
	void setPosition(const Vector3 *position);
	bool playSimple(unsigned char looped);
	void stopPlaying();

	CSWrapper::_opaqueSource *_source;
	SoundBuffer *_buffer;	// retained
	bool _playing;
	bool _pausedByVolume;
};

namespace UMapParsingUtilsFH {
// From arrays of 3 or 4 numbers (alpha 1 when 3).
void loadColorFromObject(Color *color, const LObject *obj);
void loadVectorFromObject(Vector2 *v, const LObject *obj);
void loadVectorFromObject(Vector3 *v, const LObject *obj);
void loadVectorFromObject(Vector4 *v, const LObject *obj);
} // namespace UMapParsingUtilsFH

#endif
