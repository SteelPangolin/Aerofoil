#include "CoreDefs.h"
#include "IGpAudioDriver.h"
#include "IGpAudioChannel.h"
#include "IGpAudioChannelCallbacks.h"
#include "IGpMutex.h"
#include "IGpPrefsHandler.h"
#include "IGpSystemServices.h"
#include "GpAudioDriverProperties.h"
#include "GpSDL.h"

#include "SDL_audio.h"
#include "GpRingBuffer.h"

#include "SDL_atomic.h"

#include <stdlib.h>
#include <string.h>
#include <new>
#include <stdio.h>
#include <chrono>

class GpAudioDriver_SDL2;

typedef std::chrono::high_resolution_clock::duration GpAudioDriver_SDL2_Duration_t;
typedef std::chrono::high_resolution_clock::time_point GpAudioDriver_SDL2_TimePoint_t;
typedef std::chrono::high_resolution_clock::period GpAudioDriver_SDL2_Period_t;
typedef std::chrono::high_resolution_clock::rep GpAudioDriver_SDL2_Rep_t;

static void *AlignedAlloc(size_t size, size_t alignment)
{
	void *storage = malloc(size + alignment);
	if (!storage)
		return nullptr;

	uintptr_t alignedPtr = reinterpret_cast<uintptr_t>(storage);
	size_t padding = alignment - static_cast<size_t>(alignedPtr % alignment);

	uint8_t *storageLoc = static_cast<uint8_t*>(storage);
	uint8_t *objectLoc = storageLoc + padding;
	uint8_t *paddingSizeLoc = storageLoc + padding - 1;

	*reinterpret_cast<uint8_t*>(paddingSizeLoc) = static_cast<uint8_t>(padding);

	return objectLoc;
}

static void AlignedFree(void *ptr)
{
	size_t padding = static_cast<uint8_t*>(ptr)[-1];
	void *storageLoc = static_cast<uint8_t*>(ptr) - padding;

	free(storageLoc);
}

struct GpAudioChannelBufferChain_SDL2 final
{
	GpAudioChannelBufferChain_SDL2();

	static GpAudioChannelBufferChain_SDL2 *Alloc();
	void Release();

	static const size_t kMaxCapacity = 65536;

	size_t m_consumed;
	size_t m_used;
	uint8_t m_data[kMaxCapacity];
	GpAudioChannelBufferChain_SDL2 *m_next;
	bool m_hasTrigger;
};

class GpAudioChannel_SDL2 final : public IGpAudioChannel
{
public:
	friend class GpAudioDriver_SDL2;

	GpAudioChannel_SDL2(GpAudioDriver_SDL2_Duration_t latency, GpAudioDriver_SDL2_Duration_t bufferTime, size_t bufferSamplesMax, uint16_t sampleRate);
	~GpAudioChannel_SDL2();

	void AddRef();
	void Release();

	void SetAudioChannelContext(IGpAudioChannelCallbacks *callbacks) override;
	void PostBuffer(const void *buffer, size_t bufferSize) override;
	void Stop() override;
	void Destroy() override;

	void Consume(uint8_t *output, size_t sz, GpAudioDriver_SDL2_TimePoint_t mixStartTime, GpAudioDriver_SDL2_TimePoint_t mixEndTime);

	static GpAudioChannel_SDL2 *Alloc(GpAudioDriver_SDL2 *driver, GpAudioDriver_SDL2_Duration_t latency, GpAudioDriver_SDL2_Duration_t bufferTime, size_t bufferSamplesMax, uint16_t sampleRate);

private:
	bool Init(GpAudioDriver_SDL2 *driver);

	IGpAudioChannelCallbacks *m_callbacks;
	IGpMutex *m_mutex;
	GpAudioDriver_SDL2 *m_owner;

	SDL_atomic_t m_refCount;

	GpAudioChannelBufferChain_SDL2 *m_firstPendingBuffer;
	GpAudioChannelBufferChain_SDL2 *m_lastPendingBuffer;

	GpAudioDriver_SDL2_TimePoint_t m_timestamp;		// Time that audio will be consumed if posted to the channel, if m_hasTimestamp is true.
	GpAudioDriver_SDL2_Duration_t m_latency;
	GpAudioDriver_SDL2_Duration_t m_bufferTime;
	size_t m_bufferSamplesMax;
	size_t m_leadingSilence;
	uint16_t m_sampleRate;
	bool m_isMixing;
	bool m_hasTimestamp;
};

class GpAudioDriver_SDL2 final : public IGpAudioDriver, public IGpPrefsHandler
{
public:
	friend class GpAudioChannel_SDL2;

	explicit GpAudioDriver_SDL2(const GpAudioDriverProperties &properties);
	~GpAudioDriver_SDL2();

	IGpAudioChannel *CreateChannel() override;
	void SetMasterVolume(uint32_t vol, uint32_t maxVolume) override;
	void Shutdown() override;
	IGpPrefsHandler *GetPrefsHandler() const override;

	void ApplyPrefs(const void *identifier, size_t identifierSize, const void *contents, size_t contentsSize, uint32_t version) override;
	bool SavePrefs(void *context, WritePrefsFunc_t writeFunc) override;

	bool Init();

	static GpAudioDriver_SDL2_TimePoint_t GetCurrentTime();

private:
	void DetachAudioChannel(GpAudioChannel_SDL2 *channel);

	static void SDLCALL StaticMixAudio(void *userdata, Uint8 *stream, int len);

	void MixAudio(void *stream, size_t len);
	void RefillMixChunk(GpAudioChannel_SDL2 *const*channels, size_t numChannels, size_t maxSamplesToFill, GpAudioDriver_SDL2_TimePoint_t mixStartTime, GpAudioDriver_SDL2_TimePoint_t mixEndTime);

	GpAudioDriverProperties m_properties;
	IGpMutex *m_mutex;
	IGpMutex *m_mixState;

	static const size_t kMaxChannels = 16;
	static const size_t kMixChunkSize = 512;
	static const int16_t kMaxAudioVolumeScale = 64;

	GpAudioChannel_SDL2 *m_channels[kMaxChannels];
	size_t m_numChannels;

	unsigned int m_sampleRate;
	GpAudioDriver_SDL2_Duration_t m_latency;
	GpAudioDriver_SDL2_Duration_t m_bufferTime;
	size_t m_bufferSamples;

	bool m_sdlAudioRunning;

	GP_ALIGNED(GP_SYSTEM_MEMORY_ALIGNMENT) int16_t m_mixChunk[kMixChunkSize];
	size_t m_mixChunkReadOffset;

	int16_t m_audioVolumeScale;
};

GpAudioChannelBufferChain_SDL2::GpAudioChannelBufferChain_SDL2()
	: m_used(0)
	, m_consumed(0)
	, m_next(nullptr)
	, m_hasTrigger(false)
{
}


GpAudioChannelBufferChain_SDL2 *GpAudioChannelBufferChain_SDL2::Alloc()
{
	void *storage = AlignedAlloc(sizeof(GpAudioChannelBufferChain_SDL2), GP_SYSTEM_MEMORY_ALIGNMENT);
	return new (storage) GpAudioChannelBufferChain_SDL2();
}

void GpAudioChannelBufferChain_SDL2::Release()
{
	this->~GpAudioChannelBufferChain_SDL2();
	AlignedFree(this);
}

/////////////////////////////////////////////////////////////////////////////////////////
// GpAudioChannel

GpAudioChannel_SDL2::GpAudioChannel_SDL2(GpAudioDriver_SDL2_Duration_t latency, GpAudioDriver_SDL2_Duration_t bufferTime, size_t bufferSamplesMax, uint16_t sampleRate)
	: m_callbacks(nullptr)
	, m_mutex(nullptr)
	, m_owner(nullptr)
	, m_firstPendingBuffer(nullptr)
	, m_lastPendingBuffer(nullptr)
	, m_latency(latency)
	, m_bufferTime(bufferTime)
	, m_bufferSamplesMax(bufferSamplesMax)
	, m_leadingSilence(0)
	, m_sampleRate(sampleRate)
	, m_isMixing(false)
	, m_hasTimestamp(false)
{
	SDL_AtomicSet(&m_refCount, 1);
}

GpAudioChannel_SDL2::~GpAudioChannel_SDL2()
{
	Stop();

	if (m_mutex)
		m_mutex->Destroy();

	while (m_firstPendingBuffer)
	{
		GpAudioChannelBufferChain_SDL2 *buffer = m_firstPendingBuffer;
		m_firstPendingBuffer = buffer->m_next;
		buffer->Release();
	}
}

void GpAudioChannel_SDL2::AddRef()
{
	SDL_AtomicIncRef(&m_refCount);
}

void GpAudioChannel_SDL2::Release()
{
	if (SDL_AtomicDecRef(&m_refCount))
	{
		this->~GpAudioChannel_SDL2();
		AlignedFree(this);
	}
}


void GpAudioChannel_SDL2::SetAudioChannelContext(IGpAudioChannelCallbacks *callbacks)
{
	m_callbacks = callbacks;
}

void GpAudioChannel_SDL2::PostBuffer(const void *buffer, size_t bufferSize)
{
	m_mutex->Lock();

	size_t leadingSilence = 0;
	if (m_firstPendingBuffer == nullptr && m_hasTimestamp && !m_isMixing)
	{
		GpAudioDriver_SDL2_TimePoint_t queueTime = GpAudioDriver_SDL2::GetCurrentTime() + m_latency;
		if (queueTime > m_timestamp)
		{
			const GpAudioDriver_SDL2_Duration_t leadTime = queueTime - m_timestamp;

			if (leadTime > m_bufferTime)
				leadingSilence = m_bufferSamplesMax;
			else
			{
				const GpAudioDriver_SDL2_Rep_t leadTimeRep = leadTime.count();
				leadingSilence = leadTimeRep * static_cast<GpAudioDriver_SDL2_Rep_t>(m_sampleRate) * GpAudioDriver_SDL2_Period_t::num / GpAudioDriver_SDL2_Period_t::den;
			}
		}
	}

	m_leadingSilence = leadingSilence;

	while (bufferSize > 0)
	{
		GpAudioChannelBufferChain_SDL2 *newBuffer = GpAudioChannelBufferChain_SDL2::Alloc();
		if (newBuffer == nullptr)
			break;

		if (m_lastPendingBuffer == nullptr)
			m_firstPendingBuffer = newBuffer;
		else
			m_lastPendingBuffer->m_next = newBuffer;
		m_lastPendingBuffer = newBuffer;

		size_t bufferable = newBuffer->kMaxCapacity;
		if (bufferSize < bufferable)
			bufferable = bufferSize;

		memcpy(newBuffer->m_data, buffer, bufferable);

		buffer = static_cast<const uint8_t*>(buffer) + bufferable;
		bufferSize -= bufferable;
		m_lastPendingBuffer->m_used = bufferable;
		m_lastPendingBuffer->m_hasTrigger = (bufferSize == 0);
	}

	m_mutex->Unlock();
}

void GpAudioChannel_SDL2::Stop()
{
	m_mutex->Lock();

	GpAudioChannelBufferChain_SDL2 *buffer = m_firstPendingBuffer;
	m_firstPendingBuffer = nullptr;
	m_lastPendingBuffer = nullptr;

	while (buffer)
	{
		if (buffer->m_hasTrigger && m_callbacks)
			m_callbacks->NotifyBufferFinished();

		GpAudioChannelBufferChain_SDL2 *nextBuffer = buffer->m_next;
		buffer->Release();

		buffer = nextBuffer;
	}

	m_mutex->Unlock();
}

void GpAudioChannel_SDL2::Destroy()
{
	if (m_owner)
		m_owner->DetachAudioChannel(this);

	this->Release();
}

bool GpAudioChannel_SDL2::Init(GpAudioDriver_SDL2 *driver)
{
	m_owner = driver;
	m_mutex = driver->m_properties.m_systemServices->CreateRecursiveMutex();
	if (!m_mutex)
		return false;

	return true;
}

void GpAudioChannel_SDL2::Consume(uint8_t *output, size_t sz, GpAudioDriver_SDL2_TimePoint_t mixStartTime, GpAudioDriver_SDL2_TimePoint_t mixEndTime)
{
	m_mutex->Lock();

	m_isMixing = true;
	m_hasTimestamp = true;
	m_timestamp = mixEndTime;

	if (sz <= m_leadingSilence)
	{
		memset(output, 0x80, sz);
		m_leadingSilence -= sz;

		m_isMixing = false;
		m_mutex->Unlock();
		return;
	}
	else
	{
		size_t leadingSilence = m_leadingSilence;
		if (leadingSilence > 0)
		{
			memset(output, 0x80, leadingSilence);
			output += leadingSilence;
			sz -= leadingSilence;

			m_leadingSilence = 0;
		}
	}

	while (m_firstPendingBuffer != nullptr)
	{
		GpAudioChannelBufferChain_SDL2 *buffer = m_firstPendingBuffer;
		const size_t available = (buffer->m_used - buffer->m_consumed);
		if (available <= sz)
		{
			memcpy(output, buffer->m_data + buffer->m_consumed, available);
			sz -= available;
			output += available;

			m_firstPendingBuffer = buffer->m_next;
			if (m_firstPendingBuffer == nullptr)
				m_lastPendingBuffer = nullptr;

			if (buffer->m_hasTrigger && m_callbacks)
				m_callbacks->NotifyBufferFinished();

			buffer->Release();

			if (sz == 0)
				break;
		}
		else
		{
			memcpy(output, buffer->m_data + buffer->m_consumed, sz);
			buffer->m_consumed += sz;
			buffer += sz;
			sz = 0;
			break;
		}
	}

	m_isMixing = false;

	m_mutex->Unlock();

	memset(output, 0x80, sz);
}

GpAudioChannel_SDL2 *GpAudioChannel_SDL2::Alloc(GpAudioDriver_SDL2 *driver, GpAudioDriver_SDL2_Duration_t latency, GpAudioDriver_SDL2_Duration_t bufferTime, size_t bufferSamplesMax, uint16_t sampleRate)
{
	void *storage = AlignedAlloc(sizeof(GpAudioChannel_SDL2), GP_SYSTEM_MEMORY_ALIGNMENT);
	if (!storage)
		return nullptr;

	GpAudioChannel_SDL2 *channel = new (storage) GpAudioChannel_SDL2(latency, bufferTime, bufferSamplesMax, sampleRate);
	if (!channel->Init(driver))
	{
		channel->Destroy();
		return nullptr;
	}

	return channel;
}


/////////////////////////////////////////////////////////////////////////////////////////
// GpAudioDriver_SDL2

GpAudioDriver_SDL2::GpAudioDriver_SDL2(const GpAudioDriverProperties &properties)
	: m_properties(properties)
	, m_mutex(nullptr)
	, m_numChannels(0)
	, m_sampleRate(0)
	, m_latency(GpAudioDriver_SDL2_Duration_t::zero())
	, m_bufferTime(GpAudioDriver_SDL2_Duration_t::zero())
	, m_sdlAudioRunning(false)
	, m_mixChunkReadOffset(kMixChunkSize)
	, m_audioVolumeScale(kMaxAudioVolumeScale)

{
	for (size_t i = 0; i < kMaxChannels; i++)
		m_channels[i] = nullptr;

	for (size_t i = 0; i < kMixChunkSize; i++)
		m_mixChunk[i] = 0;
}

GpAudioDriver_SDL2::~GpAudioDriver_SDL2()
{
	if (m_sdlAudioRunning)
		SDL_CloseAudio();

	if (m_mutex)
		m_mutex->Destroy();
}

IGpAudioChannel *GpAudioDriver_SDL2::CreateChannel()
{
	GpAudioChannel_SDL2 *newChannel = GpAudioChannel_SDL2::Alloc(this, m_latency, m_bufferTime, m_bufferSamples, m_sampleRate);
	if (!newChannel)
		return nullptr;

	m_mutex->Lock();
	if (m_numChannels == kMaxChannels)
	{
		newChannel->Destroy();
		m_mutex->Unlock();
		return nullptr;
	}

	m_channels[m_numChannels] = newChannel;
	m_numChannels++;

	m_mutex->Unlock();

	return newChannel;
}

void GpAudioDriver_SDL2::SetMasterVolume(uint32_t vol, uint32_t maxVolume)
{
	double scale = vol * static_cast<uint64_t>(kMaxAudioVolumeScale) / maxVolume;

	m_audioVolumeScale = static_cast<int16_t>(scale);
}

void GpAudioDriver_SDL2::Shutdown()
{
	this->~GpAudioDriver_SDL2();
	AlignedFree(this);
}

IGpPrefsHandler *GpAudioDriver_SDL2::GetPrefsHandler() const
{
	return const_cast<GpAudioDriver_SDL2*>(this);
}

void GpAudioDriver_SDL2::ApplyPrefs(const void *identifier, size_t identifierSize, const void *contents, size_t contentsSize, uint32_t version)
{
}

bool GpAudioDriver_SDL2::SavePrefs(void *context, WritePrefsFunc_t writeFunc)
{
	return true;
}

bool GpAudioDriver_SDL2::Init()
{
	m_mutex = m_properties.m_systemServices->CreateMutex();
	if (!m_mutex)
		return false;

	SDL_AudioSpec requestedSpec;
	memset(&requestedSpec, 0, sizeof(requestedSpec));

	requestedSpec.callback = GpAudioDriver_SDL2::StaticMixAudio;
	requestedSpec.channels = 1;
	requestedSpec.format = AUDIO_S16;
	requestedSpec.freq = m_properties.m_sampleRate;
	requestedSpec.samples = kMixChunkSize;
	requestedSpec.userdata = this;

	if (SDL_OpenAudio(&requestedSpec, nullptr))
	{
		requestedSpec.freq = 22050;
		if (SDL_OpenAudio(&requestedSpec, nullptr))
			return false;
	}

	SDL_PauseAudio(0);

	m_sdlAudioRunning = true;
	m_sampleRate = requestedSpec.freq;
	m_latency = GpAudioDriver_SDL2_Duration_t(static_cast<GpAudioDriver_SDL2_Rep_t>(GpAudioDriver_SDL2_Period_t::den * requestedSpec.samples / GpAudioDriver_SDL2_Period_t::num / m_sampleRate));
	m_bufferTime = GpAudioDriver_SDL2_Duration_t(static_cast<GpAudioDriver_SDL2_Rep_t>(GpAudioDriver_SDL2_Period_t::den * requestedSpec.samples / GpAudioDriver_SDL2_Period_t::num / m_sampleRate));
	m_bufferSamples = requestedSpec.samples;

	return true;
}

void GpAudioDriver_SDL2::DetachAudioChannel(GpAudioChannel_SDL2 *channel)
{
	m_mutex->Lock();
	const size_t numChannels = m_numChannels;
	for (size_t i = 0; i < numChannels; i++)
	{
		if (m_channels[i] == channel)
		{
			m_numChannels = numChannels - 1;
			m_channels[i] = m_channels[numChannels - 1];
			m_channels[numChannels - 1] = nullptr;
			break;
		}
	}
	m_mutex->Unlock();
}

void GpAudioDriver_SDL2::StaticMixAudio(void *userdata, Uint8 *stream, int len)
{
	static_cast<GpAudioDriver_SDL2*>(userdata)->MixAudio(stream, static_cast<size_t>(len));
}

void GpAudioDriver_SDL2::MixAudio(void *stream, size_t len)
{
	GpAudioChannel_SDL2 *mixingChannels[kMaxChannels];
	size_t numChannels = 0;

	m_mutex->Lock();
	numChannels = m_numChannels;
	for (size_t i = 0; i < numChannels; i++)
	{
		GpAudioChannel_SDL2 *channel = m_channels[i];
		channel->AddRef();

		mixingChannels[i] = channel;
	}
	m_mutex->Unlock();

	const size_t totalSamples = len / sizeof(int16_t);
	size_t samplesRemaining = totalSamples;

	GpAudioDriver_SDL2_TimePoint_t audioMixStartTime = GpAudioDriver_SDL2::GetCurrentTime();
	GpAudioDriver_SDL2_TimePoint_t audioMixBlockStartTime = audioMixStartTime;

	size_t samplesSinceStart = 0;
	for (;;)
	{
		size_t availableInMixChunk = kMixChunkSize - m_mixChunkReadOffset;

		if (availableInMixChunk > samplesRemaining)
		{
			m_mixChunkReadOffset += samplesRemaining;
			memcpy(stream, m_mixChunk + m_mixChunkReadOffset, samplesRemaining * sizeof(int16_t));

			break;
		}
		else
		{
			memcpy(stream, m_mixChunk + m_mixChunkReadOffset, availableInMixChunk * sizeof(int16_t));

			stream = static_cast<int16_t*>(stream) + availableInMixChunk;

			samplesSinceStart += availableInMixChunk;

			GpAudioDriver_SDL2_Duration_t audioMixDurationSinceStart = GpAudioDriver_SDL2_Duration_t(static_cast<GpAudioDriver_SDL2_Rep_t>(GpAudioDriver_SDL2_Period_t::den * samplesSinceStart / GpAudioDriver_SDL2_Period_t::num / m_sampleRate));
			GpAudioDriver_SDL2_TimePoint_t audioMixBlockEndTime = audioMixStartTime + audioMixDurationSinceStart;

			m_mixChunkReadOffset = 0;
			RefillMixChunk(mixingChannels, numChannels, samplesRemaining, audioMixBlockStartTime, audioMixBlockEndTime);
			audioMixBlockStartTime = audioMixBlockEndTime;

			samplesRemaining -= availableInMixChunk;
		}
	}

	for (size_t i = 0; i < numChannels; i++)
		mixingChannels[i]->Release();
}

void GpAudioDriver_SDL2::RefillMixChunk(GpAudioChannel_SDL2 *const*channels, size_t numChannels, size_t maxSamplesToFill, GpAudioDriver_SDL2_TimePoint_t mixStartTime, GpAudioDriver_SDL2_TimePoint_t mixEndTime)
{
	uint8_t audioMixBufferUnaligned[kMixChunkSize + GP_SYSTEM_MEMORY_ALIGNMENT];
	uint8_t *audioMixBuffer = audioMixBufferUnaligned;

	{
		uintptr_t bufferPtr = reinterpret_cast<uintptr_t>(audioMixBuffer);
		size_t alignPadding = GP_SYSTEM_MEMORY_ALIGNMENT - (bufferPtr % GP_SYSTEM_MEMORY_ALIGNMENT);
		audioMixBuffer += alignPadding;
	}

	bool noAudio = true;

	const int16_t audioVolumeScale = m_audioVolumeScale;

	size_t samplesToFill = kMixChunkSize;
	if (samplesToFill > maxSamplesToFill)
	{
		m_mixChunkReadOffset += samplesToFill - maxSamplesToFill;
		samplesToFill = maxSamplesToFill;
	}
	else
		m_mixChunkReadOffset = 0;

	int16_t *mixChunkStart = m_mixChunk + m_mixChunkReadOffset;
	int16_t audioNormalizeShift = 0;

	for (size_t i = 0; i < numChannels; i++)
	{
		channels[i]->Consume(audioMixBuffer, samplesToFill, mixStartTime, mixEndTime);

		if (i == 0)
		{
			noAudio = false;
			audioNormalizeShift = 0x80;
			for (size_t j = 0; j < samplesToFill; j++)
				mixChunkStart[j] = static_cast<int16_t>(audioMixBuffer[j]);
		}
		else
		{
			audioNormalizeShift += 0x80;
			for (size_t j = 0; j < samplesToFill; j++)
				mixChunkStart[j] += static_cast<int16_t>(audioMixBuffer[j]);
		}
	}

	if (noAudio)
		memset(mixChunkStart, 0, samplesToFill * sizeof(mixChunkStart[0]));
	else
	{
		for (size_t i = 0; i < samplesToFill; i++)
			mixChunkStart[i] = (mixChunkStart[i] - audioNormalizeShift) * audioVolumeScale;
	}
}


GpAudioDriver_SDL2_TimePoint_t GpAudioDriver_SDL2::GetCurrentTime()
{
	return std::chrono::high_resolution_clock::now();
}

IGpAudioDriver *GpDriver_CreateAudioDriver_SDL(const GpAudioDriverProperties &properties)
{
	void *storage = AlignedAlloc(sizeof(GpAudioDriver_SDL2), GP_SYSTEM_MEMORY_ALIGNMENT);
	if (!storage)
		return nullptr;

	GpAudioDriver_SDL2 *driver = new (storage) GpAudioDriver_SDL2(properties);
	if (!driver->Init())
	{
		driver->Shutdown();
		return nullptr;
	}

	return driver;
}
