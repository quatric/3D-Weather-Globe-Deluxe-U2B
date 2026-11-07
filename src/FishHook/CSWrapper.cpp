// CSWrapper: OpenAL, with sound files decoded by Audio Toolbox (OpenAL's
// alutLoadWAVFile is gone), and the sound classes.

#include "FHSound.h"
#include "LFileManager.h"
#include "LScanner.h"

#include <AudioToolbox/AudioToolbox.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <vector>

LF_IMPLEMENT_CLASS(SoundManager, 0)
LF_IMPLEMENT_CLASS(SoundBuffer, 0)
LF_IMPLEMENT_CLASS(SoundSource, 0)

namespace CSWrapper {

struct _opaqueBuffer {
	_opaqueBuffer(LString *path);
	~_opaqueBuffer();
	ALuint buffer;
};

struct _opaqueSource {
	_opaqueSource();
	~_opaqueSource();
	ALuint source;
};

static ALCdevice *gDevice = 0;
static ALCcontext *gContext = 0;

bool initRenderingContext()
{
	gDevice = alcOpenDevice(0);
	if (gDevice == 0)
		return false;
	gContext = alcCreateContext(gDevice, 0);
	if (gContext == 0)
		return false;
	alcMakeContextCurrent(gContext);
	return true;
}

void closeRenderingContext()
{
	if (gDevice && gContext) {
		alcDestroyContext(gContext);
		alcCloseDevice(gDevice);
	}
}

void setListenerPosition(const Vector3 *position)
{
	alListener3f(AL_POSITION, position->x, position->y, position->z);
}

void setListenerOrientation(const Vector3 *orientation)
{
	alListener3f(AL_ORIENTATION, orientation->x, orientation->y, orientation->z);
}

void setListenerVolume(float volume)
{
	alListenerf(AL_GAIN, volume);
}

// Decodes the whole file to 16-bit PCM in its own channels and rate.
static bool _loadPCMFile(const char *path, std::vector<char> *pcm, ALenum *format, ALsizei *frequency)
{
	CFURLRef url =
	    CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)path, strlen(path), false);
	if (url == 0)
		return false;
	ExtAudioFileRef file = 0;
	OSStatus err = ExtAudioFileOpenURL(url, &file);
	CFRelease(url);
	if (err != noErr)
		return false;
	AudioStreamBasicDescription in;
	UInt32 size = sizeof(in);
	bool ok = false;
	if (ExtAudioFileGetProperty(file, kExtAudioFileProperty_FileDataFormat, &size, &in) == noErr) {
		UInt32 channels = in.mChannelsPerFrame > 1 ? 2 : 1;
		AudioStreamBasicDescription out = {};
		out.mSampleRate = in.mSampleRate;
		out.mFormatID = kAudioFormatLinearPCM;
		out.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
		out.mChannelsPerFrame = channels;
		out.mBitsPerChannel = 16;
		out.mBytesPerFrame = 2 * channels;
		out.mFramesPerPacket = 1;
		out.mBytesPerPacket = out.mBytesPerFrame;
		if (ExtAudioFileSetProperty(file, kExtAudioFileProperty_ClientDataFormat, sizeof(out), &out) == noErr) {
			char chunk[32768];
			for (;;) {
				AudioBufferList list;
				list.mNumberBuffers = 1;
				list.mBuffers[0].mNumberChannels = channels;
				list.mBuffers[0].mDataByteSize = sizeof(chunk);
				list.mBuffers[0].mData = chunk;
				UInt32 frames = sizeof(chunk) / out.mBytesPerFrame;
				if (ExtAudioFileRead(file, &frames, &list) != noErr)
					break;
				if (frames == 0) {
					ok = true;
					break;
				}
				pcm->insert(pcm->end(), chunk, chunk + frames * out.mBytesPerFrame);
			}
			*format = channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
			*frequency = (ALsizei)in.mSampleRate;
		}
	}
	ExtAudioFileDispose(file);
	return ok;
}

_opaqueBuffer::_opaqueBuffer(LString *path) : buffer(0)
{
	alcMakeContextCurrent(gContext);
	alGenBuffers(1, &buffer);
	if (alGetError() != AL_NO_ERROR)
		return;
	LFileManager *manager = LFileManager::manager();
	LString *systemPath = manager ? manager->systemPathFromInternalPath(path) : path;
	std::vector<char> pcm;
	ALenum format = 0;
	ALsizei frequency = 0;
	if (!_loadPCMFile(systemPath->UTF8String(), &pcm, &format, &frequency))
		return;
	alBufferData(buffer, format, pcm.data(), (ALsizei)pcm.size(), frequency);
	alGetError();
}

_opaqueBuffer::~_opaqueBuffer()
{
	if (buffer) {
		alDeleteBuffers(1, &buffer);
		alGetError();
	}
}

_opaqueSource::_opaqueSource() : source(0)
{
	alGenSources(1, &source);
	alGetError();
}

_opaqueSource::~_opaqueSource()
{
	if (source) {
		alDeleteSources(1, &source);
		alGetError();
	}
}

_opaqueBuffer *createBufferFromFile(LString *path)
{
	return new _opaqueBuffer(path);
}

void deleteBuffer(_opaqueBuffer *buffer)
{
	delete buffer;
}

_opaqueSource *createSource()
{
	return new _opaqueSource;
}

void deleteSource(_opaqueSource *source)
{
	delete source;
}

void setBufferForSource(_opaqueSource *source, _opaqueBuffer *buffer)
{
	if (source->source)
		alSourcei(source->source, AL_BUFFER, buffer ? buffer->buffer : 0);
}

void setSourceVolume(_opaqueSource *source, float volume)
{
	if (source->source)
		alSourcef(source->source, AL_GAIN, volume);
}

void setSourcePitch(_opaqueSource *source, float pitch)
{
	if (source->source)
		alSourcef(source->source, AL_PITCH, pitch);
}

void setSourcePosition(_opaqueSource *source, const Vector3 *position)
{
	if (source->source)
		alSource3f(source->source, AL_POSITION, position->x, position->y, position->z);
}

void setSourceLooped(_opaqueSource *source, unsigned char looped)
{
	if (source->source)
		alSourcei(source->source, AL_LOOPING, looped != 0);
}

void sourcePlay(_opaqueSource *source)
{
	if (source->source)
		alSourcePlay(source->source);
}

void sourceStop(_opaqueSource *source)
{
	if (source->source)
		alSourceStop(source->source);
}

void sourcePause(_opaqueSource *source)
{
	if (source->source)
		alSourcePause(source->source);
}

void sourceResume(_opaqueSource *source)
{
	if (source->source)
		alSourcePlay(source->source);
}

} // namespace CSWrapper

// ---------------------------------------------------------------------------
// SoundManager

void SoundManager::initAsSingleton(void *)
{
	CSWrapper::initRenderingContext();
	Vector3 origin(0, 0, 0);
	CSWrapper::setListenerPosition(&origin);
	CSWrapper::setListenerVolume(1.0f);
}

void SoundManager::dealloc()
{
	CSWrapper::closeRenderingContext();
	LObject::dealloc();
}

void SoundManager::setListenerVolume(float volume)
{
	CSWrapper::setListenerVolume(volume);
}

SoundBuffer *SoundManager::detachSoundBuffer(LString *path)
{
	SoundBuffer *buffer = SoundBuffer::_alloc()->initWithPath(path, this);
	return buffer ? (SoundBuffer *)LAutorelease(buffer) : 0;
}

// ---------------------------------------------------------------------------
// SoundBuffer

SoundBuffer *SoundBuffer::initWithPath(LString *path, SoundManager *manager)
{
	if (LObject::init() == 0)
		return 0;
	_manager = manager;
	_buffer = CSWrapper::createBufferFromFile(path);
	return this;
}

void SoundBuffer::dealloc()
{
	if (_buffer)
		CSWrapper::deleteBuffer(_buffer);
	LObject::dealloc();
}

SoundSource *SoundBuffer::detachSoundSource()
{
	SoundSource *source = SoundSource::_alloc()->initWithBuffer(this);
	return (SoundSource *)LAutorelease(source);
}

// ---------------------------------------------------------------------------
// SoundSource

SoundSource *SoundSource::initWithBuffer(SoundBuffer *buffer)
{
	if (LObject::init() == 0)
		return 0;
	_buffer = buffer;
	LRetain(buffer);
	_source = CSWrapper::createSource();
	CSWrapper::setBufferForSource(_source, _buffer->_buffer);
	CSWrapper::setSourceVolume(_source, 1.0f);
	Vector3 origin(0, 0, 0);
	CSWrapper::setSourcePosition(_source, &origin);
	return this;
}

void SoundSource::dealloc()
{
	if (_source) {
		if (_playing) {
			_playing = false;
			CSWrapper::sourceStop(_source);
		}
		CSWrapper::deleteSource(_source);
	}
	LRelease(_buffer);
	_buffer = 0;
	LObject::dealloc();
}

void SoundSource::setVolume(float volume)
{
	if (_source == 0)
		return;
	CSWrapper::setSourceVolume(_source, volume);
	if (volume < 0.01f && !_pausedByVolume) {
		_pausedByVolume = true;
		CSWrapper::sourcePause(_source);
	} else if (volume > 0.015f && _pausedByVolume) {
		_pausedByVolume = false;
		CSWrapper::sourceResume(_source);
	}
}

void SoundSource::setPitch(float pitch)
{
	if (_source)
		CSWrapper::setSourcePitch(_source, pitch);
}

void SoundSource::setPosition(const Vector3 *position)
{
	if (_source)
		CSWrapper::setSourcePosition(_source, position);
}

bool SoundSource::playSimple(unsigned char looped)
{
	if (_source) {
		CSWrapper::setSourceLooped(_source, looped);
		CSWrapper::sourcePlay(_source);
		_playing = true;
	}
	return true;
}

void SoundSource::stopPlaying()
{
	if (_source) {
		_playing = false;
		CSWrapper::sourceStop(_source);
	}
}

// ---------------------------------------------------------------------------
// UMapParsingUtilsFH

static float _item(const LArray *a, int i)
{
	return UMapParsingUtils::loadFloatFromObject(a->objectAtIndex(i));
}

void UMapParsingUtilsFH::loadColorFromObject(Color *color, const LObject *obj)
{
	const LArray *a = lobj_cast<LArray>(obj);
	if (a == 0)
		return;
	int count = a->count();
	if (count != 3 && count != 4)
		return;
	color->r = _item(a, 0);
	color->g = _item(a, 1);
	color->b = _item(a, 2);
	color->a = count == 4 ? _item(a, 3) : 1.0f;
}

void UMapParsingUtilsFH::loadVectorFromObject(Vector2 *v, const LObject *obj)
{
	const LArray *a = lobj_cast<LArray>(obj);
	if (a && a->count() > 1) {
		v->x = _item(a, 0);
		v->y = _item(a, 1);
	}
}

void UMapParsingUtilsFH::loadVectorFromObject(Vector3 *v, const LObject *obj)
{
	const LArray *a = lobj_cast<LArray>(obj);
	if (a && a->count() > 2) {
		v->x = _item(a, 0);
		v->y = _item(a, 1);
		v->z = _item(a, 2);
	}
}

void UMapParsingUtilsFH::loadVectorFromObject(Vector4 *v, const LObject *obj)
{
	const LArray *a = lobj_cast<LArray>(obj);
	if (a && a->count() > 3) {
		v->x = _item(a, 0);
		v->y = _item(a, 1);
		v->z = _item(a, 2);
		v->w = _item(a, 3);
	}
}
