
//============================================================================
//----------------------------------------------------------------------------
//									Sound.c
//----------------------------------------------------------------------------
//============================================================================


#include "PLResources.h"
#include "PLSound.h"
#include "DialogManager.h"
#include "Externs.h"
#include "IGpLogDriver.h"
#include "MemoryManager.h"
#include "ResourceManager.h"
#include "SoundSync.h"
#include "VirtualDirectory.h"
#include "WaveFormat.h"

#include "PLDrivers.h"


#define kBaseBufferSoundID			1000
#define kMaxSounds					64


void CallBack0 (PortabilityLayer::AudioChannel *);
void CallBack1 (PortabilityLayer::AudioChannel *);
void CallBack2 (PortabilityLayer::AudioChannel *);
PLError_t LoadBufferSounds (void);
void DumpBufferSounds (void);
PLError_t OpenSoundChannels (void);
void CloseSoundChannels (void);
THandle<void> ParseAndConvertSound(const THandle<void> &handle);

PortabilityLayer::AudioChannel *channel0, *channel1, *channel2;
Ptr					theSoundData[kMaxSounds];
short				numSoundsLoaded;
Boolean				soundLoaded[kMaxSounds], dontLoadSounds;
Boolean				channelOpen, isSoundOn, failedSound;

//==============================================================  Functions
//--------------------------------------------------------------  PlayPrioritySound

void PlayPrioritySound (short which, short priority)
{
	short		lowestPriority, whosLowest;
	
	if (failedSound || dontLoadSounds)
		return;

	SoundSyncState ss = SoundSync_ReadAll();
	
	if ((priority == kTriggerPriority) && 
			((ss.priority0 == kTriggerPriority) ||
			((ss.priority1 == kTriggerPriority)) ||
			((ss.priority2 == kTriggerPriority))))
		return;
	
	whosLowest = 0;
	lowestPriority = ss.priority0;
	
	if (ss.priority1 < lowestPriority)
	{
		lowestPriority = ss.priority1;
		whosLowest = 1;
	}
	
	if (ss.priority2 < lowestPriority)
	{
		lowestPriority = ss.priority2;
		whosLowest = 2;
	}
	
	if (priority >= lowestPriority)
	{
		PlayExclusiveSoundChannel(whosLowest, which, lowestPriority, priority);
	}
}

//--------------------------------------------------------------  FlushAnyTriggerPlaying

void FlushAnyTriggerPlaying (void)
{
	SoundSyncState ss = SoundSync_ReadAll();
	
	if (ss.priority0 == kTriggerPriority)
	{
		channel0->ClearAllCommands();
		channel0->Stop();
	}
	
	if (ss.priority1 == kTriggerPriority)
	{
		channel1->ClearAllCommands();
		channel1->Stop();
	}
	
	if (ss.priority2 == kTriggerPriority)
	{
		channel2->ClearAllCommands();
		channel2->Stop();
	}
}

//--------------------------------------------------------------  CallBack0

void CallBack0(PortabilityLayer::AudioChannel *theChannel)
{
	SoundSync_ClearPriority(0);
}

//--------------------------------------------------------------  CallBack1

void CallBack1(PortabilityLayer::AudioChannel *theChannel)
{
	SoundSync_ClearPriority(1);
}

//--------------------------------------------------------------  CallBack2

void CallBack2(PortabilityLayer::AudioChannel *theChannel)
{
	SoundSync_ClearPriority(2);
}

//--------------------------------------------------------------  PlaySound0

void PlayExclusiveSoundChannel(short channelIndex, short soundID, short oldPriority, short newPriority)
{	
	if (failedSound || dontLoadSounds)
		return;

	PortabilityLayer::AudioChannel *channel = nil;
	PortabilityLayer::AudioChannelCallback_t callback = nil;

	switch (channelIndex)
	{
	case 0:
		channel = channel0;
		callback = CallBack0;
		break;
	case 1:
		channel = channel1;
		callback = CallBack1;
		break;
	case 2:
		channel = channel2;
		callback = CallBack2;
		break;
	default:
		return;
	}

	if (isSoundOn)
	{
		if (oldPriority != 0)
		{
			// Flush the queue and stop the channel, which will remove the pending callback
			channel->ClearAllCommands();
			channel->Stop();

			SoundSync_ClearPriority(channelIndex);
		}

		SoundSync_PutPriority(channelIndex, newPriority);

		bool succeeded = channel->AddBuffer(theSoundData[soundID], false);
		succeeded &= channel->AddCallback(callback, false);
		
		if (!succeeded)
			SoundSync_ClearPriority(channelIndex);
	}
}

//--------------------------------------------------------------  ParseAndConvertSound


//--------------------------------------------------------------  LoadTriggerSound

PLError_t LoadTriggerSound (short soundID)
{
	Handle		theSound;
	long		soundDataSize;
	PLError_t		theErr;
	
	if ((dontLoadSounds) || (theSoundData[kMaxSounds - 1] != nil))
		theErr = PLErrors::kFileNotFound;
	else
	{
//		FlushAnyTriggerPlaying();
		
		theErr = PLErrors::kNone;
		
		theSound = ParseAndConvertSound(LoadHouseResource('snd ', soundID));
		if (theSound == nil)
		{
			theErr = PLErrors::kFileNotFound;
		}
		else
		{
			soundDataSize = GetHandleSize(theSound);
			theSoundData[kMaxSounds - 1] = PortabilityLayer::MemoryManager::GetInstance()->Alloc(soundDataSize);
			if (theSoundData[kMaxSounds - 1] == nil)
			{
				theSound.Dispose();
				theErr = PLErrors::kOutOfMemory;
			}
			else
			{
				memcpy(theSoundData[kMaxSounds - 1], (Byte*)(*theSound), soundDataSize);
				theSound.Dispose();
			}
		}
	}
	
	return (theErr);
}

//--------------------------------------------------------------  DumpTriggerSound

void DumpTriggerSound (void)
{
	if (theSoundData[kMaxSounds - 1] != nil)
		PortabilityLayer::MemoryManager::GetInstance()->Release(theSoundData[kMaxSounds - 1]);
	theSoundData[kMaxSounds - 1] = nil;
}

//--------------------------------------------------------------  LoadBufferSounds

PLError_t LoadBufferSounds (void)
{
	Handle		theSound;
	long		soundDataSize;
	PLError_t		theErr;
	short		i;
	
	theErr = PLErrors::kNone;
	
	for (i = 0; i < kMaxSounds - 1; i++)
	{
		theSound = ParseAndConvertSound(PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('snd ', i + kBaseBufferSoundID));
		if (theSound == nil)
			return (PLErrors::kOutOfMemory);
		
		soundDataSize = GetHandleSize(theSound);
		
		theSoundData[i] = PortabilityLayer::MemoryManager::GetInstance()->Alloc(soundDataSize);
		if (theSoundData[i] == nil)
			return (PLErrors::kOutOfMemory);
		
		memcpy(theSoundData[i], *theSound, soundDataSize);
		theSound.Dispose();
	}
	
	theSoundData[kMaxSounds - 1] = nil;
	
	return (theErr);
}

//--------------------------------------------------------------  DumpBufferSounds

void DumpBufferSounds (void)
{
	short		i;
	
	for (i = 0; i < kMaxSounds; i++)
	{
		if (theSoundData[i] != nil)
			PortabilityLayer::MemoryManager::GetInstance()->Release(theSoundData[i]);
		theSoundData[i] = nil;
	}
}

//--------------------------------------------------------------  OpenSoundChannels

PLError_t OpenSoundChannels (void)
{
	IGpLogDriver *logger = PLDrivers::GetLogDriver();

	if (channelOpen)
	{
		if (logger)
			logger->Printf(IGpLogDriver::Category_Error, "Audio was already opened?");
		return PLErrors::kAudioError;
	}
	
	channel0 = PortabilityLayer::SoundSystem::GetInstance()->CreateChannel();
	if (channel0)
		channelOpen = true;
	else
	{
		if (logger)
			logger->Printf(IGpLogDriver::Category_Error, "Audio channel 0 failed to open");

		return PLErrors::kAudioError;
	}

	channel1 = PortabilityLayer::SoundSystem::GetInstance()->CreateChannel();
	if (channel1)
		channelOpen = true;
	else
	{
		if (logger)
			logger->Printf(IGpLogDriver::Category_Error, "Audio channel 1 failed to open");

		return PLErrors::kAudioError;
	}

	channel2 = PortabilityLayer::SoundSystem::GetInstance()->CreateChannel();
	if (channel2)
		channelOpen = true;
	else
	{
		if (logger)
			logger->Printf(IGpLogDriver::Category_Error, "Audio channel 2 failed to open");

		return PLErrors::kAudioError;
	}
	
	return PLErrors::kNone;
}

//--------------------------------------------------------------  CloseSoundChannels

void CloseSoundChannels (void)
{
	if (!channelOpen)
		return;
	
	if (channel0 != nil)
		channel0->Destroy(false);
	channel0 = nil;
	
	if (channel1 != nil)
		channel1->Destroy(false);
	channel1 = nil;
	
	if (channel2 != nil)
		channel2->Destroy(false);
	channel2 = nil;

	channelOpen = false;
}

//--------------------------------------------------------------  InitSound

void InitSound (void)
{
	PLError_t		theErr;
		
	if (dontLoadSounds)
		return;
	
	failedSound = false;
	
	channel0 = nil;
	channel1 = nil;
	channel2 = nil;

	SoundSync_ClearPriority(0);
	SoundSync_ClearPriority(1);
	SoundSync_ClearPriority(2);
	
	theErr = LoadBufferSounds();
	if (theErr != PLErrors::kNone)
	{
		YellowAlert(kYellowFailedSound, theErr);
		failedSound = true;
	}
	
	if (!failedSound)
	{
		theErr = OpenSoundChannels();
		if (theErr != PLErrors::kNone)
		{
			YellowAlert(kYellowFailedSound, theErr);
			failedSound = true;
		}
	}
}

//--------------------------------------------------------------  KillSound

void KillSound (void)
{
	if (dontLoadSounds)
		return;
	
	CloseSoundChannels();
	DumpBufferSounds();
}

//--------------------------------------------------------------  TellHerNoSounds

void TellHerNoSounds (void)
{
	#define		kNoMemForSoundsAlert	1039
	short		hitWhat;
	
//	CenterAlert(kNoMemForSoundsAlert);
	hitWhat = PortabilityLayer::DialogManager::GetInstance()->DisplayAlert(kNoMemForSoundsAlert, nullptr);
}



//--------------------------------------------------------------  ParseAndConvertSound

THandle<void> ParseAndConvertSoundChecked(const THandle<void> &handle)
{
	const uint8_t *dataStart = static_cast<const uint8_t*>(*handle);
	const size_t size = handle.MMBlock()->m_size;

	if (size < sizeof(PortabilityLayer::RIFFTag))
		return THandle<void>();

	PortabilityLayer::RIFFTag mainRiffTag;
	memcpy(&mainRiffTag, dataStart, sizeof(PortabilityLayer::RIFFTag));

	if (mainRiffTag.m_tag != PortabilityLayer::WaveConstants::kRiffChunkID)
		return THandle<void>();

	const uint32_t riffSize = mainRiffTag.m_chunkSize;
	if (riffSize < 4 || riffSize - 4 > size - sizeof(PortabilityLayer::RIFFTag))
		return THandle<void>();

	const uint8_t *riffStart = dataStart + sizeof(PortabilityLayer::RIFFTag);
	const uint8_t *riffEnd = riffStart + riffSize;

	const uint8_t *formatTagLoc = nullptr;
	const uint8_t *dataTagLoc = nullptr;

	LEUInt32_t waveMarker;
	memcpy(&waveMarker, riffStart, 4);

	if (waveMarker != PortabilityLayer::WaveConstants::kWaveChunkID)
		return THandle<void>();

	const uint8_t *tagSearchLoc = riffStart + 4;

	// Find tags
	while (tagSearchLoc != riffEnd)
	{
		if (riffEnd - tagSearchLoc < sizeof(PortabilityLayer::RIFFTag))
			return THandle<void>();

		PortabilityLayer::RIFFTag riffTag;
		memcpy(&riffTag, tagSearchLoc, sizeof(PortabilityLayer::RIFFTag));

		if (riffTag.m_tag == PortabilityLayer::WaveConstants::kFormatChunkID)
			formatTagLoc = tagSearchLoc;
		else if (riffTag.m_tag == PortabilityLayer::WaveConstants::kDataChunkID)
			dataTagLoc = tagSearchLoc;

		const uint32_t riffTagSizeUnpadded = riffTag.m_chunkSize;

		if (riffTagSizeUnpadded == 0xffffffffU)
			return THandle<void>();

		const uint32_t riffTagSizePadded = riffTagSizeUnpadded + (riffTagSizeUnpadded & 1);

		tagSearchLoc += sizeof(PortabilityLayer::RIFFTag);

		if (riffEnd - tagSearchLoc < riffTagSizePadded)
			return THandle<void>();

		tagSearchLoc += riffTagSizePadded;
	}

	if (formatTagLoc == nullptr || dataTagLoc == nullptr)
		return THandle<void>();

	PortabilityLayer::RIFFTag fmtTag;
	memcpy(&fmtTag, formatTagLoc, sizeof(PortabilityLayer::RIFFTag));

	const uint8_t *formatContents = formatTagLoc + sizeof(PortabilityLayer::RIFFTag);

	PortabilityLayer::RIFFTag dataTag;
	memcpy(&dataTag, dataTagLoc, sizeof(PortabilityLayer::RIFFTag));

	const uint8_t *dataContents = dataTagLoc + sizeof(PortabilityLayer::RIFFTag);

	PortabilityLayer::WaveFormatChunkV3 formatChunkV3;

	memset(&formatChunkV3, 0, sizeof(formatChunkV3));

	int formatChunkVersion = 0;
	size_t copyableSize = 0;
	if (fmtTag.m_chunkSize >= sizeof(PortabilityLayer::WaveFormatChunkV3))
	{
		formatChunkVersion = 3;
		copyableSize = sizeof(PortabilityLayer::WaveFormatChunkV3);
	}
	else if (fmtTag.m_chunkSize >= sizeof(PortabilityLayer::WaveFormatChunkV2))
	{
		formatChunkVersion = 2;
		copyableSize = sizeof(PortabilityLayer::WaveFormatChunkV2);
	}
	else if (fmtTag.m_chunkSize >= sizeof(PortabilityLayer::WaveFormatChunkV1))
	{
		formatChunkVersion = 1;
		copyableSize = sizeof(PortabilityLayer::WaveFormatChunkV1);
	}
	else
		return THandle<void>();

	memcpy(&formatChunkV3, formatContents, copyableSize);

	const PortabilityLayer::WaveFormatChunkV2 formatChunkV2 = formatChunkV3.m_v2;
	const PortabilityLayer::WaveFormatChunkV1 formatChunkV1 = formatChunkV2.m_v1;

	if (formatChunkV1.m_bitsPerSample != 8)
		return THandle<void>();

	if (formatChunkV1.m_formatCode != PortabilityLayer::WaveConstants::kFormatPCM ||
		formatChunkV1.m_numChannels != 1 ||
		formatChunkV1.m_blockAlignmentBytes != 1 ||
		formatChunkV1.m_bitsPerSample != 8)
		return THandle<void>();

	THandle<void> convertedHandle = THandle<void>(PortabilityLayer::MemoryManager::GetInstance()->AllocHandle(4 + dataTag.m_chunkSize));
	if (!convertedHandle)
		return THandle<void>();

	uint8_t *handleData = static_cast<uint8_t*>(*convertedHandle);

	uint32_t dataSize = dataTag.m_chunkSize;
	memcpy(handleData, &dataSize, 4);

	memcpy(handleData + 4, dataContents, dataSize);

	return convertedHandle;
}

THandle<void> ParseAndConvertSound(const THandle<void> &handle)
{
	if (!handle)
		return THandle<void>();

	THandle<void> converted = ParseAndConvertSoundChecked(handle);
	handle.Dispose();

	return converted;
}
