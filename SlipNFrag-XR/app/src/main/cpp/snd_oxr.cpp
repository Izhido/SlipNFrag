#include "quakedef.h"
#include <oboe/Oboe.h>
#include "Locks.h"

std::shared_ptr<oboe::AudioStream> snd_audioStream;

qboolean snd_forceclear;

extern int sound_started;

class SNDDMA_CallbackClass : public oboe::AudioStreamDataCallback
{
public:
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream*/* audioStream*/, void *audioData, int32_t numFrames)
    {
        std::lock_guard<std::mutex> lock(Locks::SoundMutex);

        if (snd_forceclear)
        {
            S_ClearBuffer();
            snd_forceclear = false;
        }

        size_t first = numFrames;
        size_t second = 0;
        int newSamplePos = shm->samplepos + (numFrames << 1);
        if (newSamplePos > shm->samples)
        {
            size_t remaining = (newSamplePos - shm->samples) >> 1;
            first -= remaining;
            second = remaining;
        }

        memcpy(audioData, shm->buffer.data() + (shm->samplepos << 2), first << 3);
        shm->samplepos += (first << 1);
        if (shm->samplepos >= shm->samples)
        {
            shm->samplepos = 0;
        }

        if (second > 0)
        {
            memcpy(audioData, shm->buffer.data(), second << 3);
            shm->samplepos += (second << 1);
        }

        return oboe::DataCallbackResult::Continue;
    }
};

SNDDMA_CallbackClass snd_audioCallback;

qboolean SNDDMA_Init(void)
{
	shm = new dma_t;
	shm->splitbuffer = 0;
	shm->samplebits = 32;
	shm->speed = 44100;
	shm->channels = 2;
	shm->samples = 65536;
	shm->samplepos = (shm->samples >> 3);
	shm->soundalive = true;
	shm->gamealive = true;
	shm->submission_chunk = 1;
	shm->buffer.resize(shm->samples * sizeof(float));

    oboe::AudioStreamBuilder builder;

    auto result = builder.setChannelCount(shm->channels)
            ->setSampleRate(shm->speed)
            ->setFormat(oboe::AudioFormat::Float)
            ->setDataCallback(&snd_audioCallback)
            ->openStream(snd_audioStream);
    if (result != oboe::Result::OK)
    {
        return false;
    }

    result = snd_audioStream->requestStart();
    if (result != oboe::Result::OK)
    {
        snd_audioStream->close();
        snd_audioStream.reset();
        return false;
    }

	return true;
}

int SNDDMA_GetDMAPos(void)
{
	return shm->samplepos;
}

void SNDDMA_Submit(void)
{
}

void SNDDMA_Shutdown(void)
{
	std::lock_guard<std::mutex> lock(Locks::SoundMutex);

    if (snd_audioStream)
    {
        snd_audioStream->stop();
        snd_audioStream->close();
        snd_audioStream.reset();
    }
}
