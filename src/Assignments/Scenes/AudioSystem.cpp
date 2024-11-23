#include "AudioSystem.h"
#include <algorithm>

AudioSystem::AudioSystem()
    : currentBuffer(0)
    , isInitialized(false)
    , lastSample(0.0f)
    , filterState(0.0f) {
    init();
}

AudioSystem::~AudioSystem() {
    if (isInitialized) {
        waveOutReset(hWaveOut);

        for (auto& header : waveHeaders) {
            waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        }

        waveOutClose(hWaveOut);
    }
}

void AudioSystem::init() {
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = NUM_CHANNELS;
    wfx.nSamplesPerSec = SAMPLE_RATE;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)waveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
        return;
    }

    // Initialize and immediately queue all buffers
    for (int i = 0; i < NUM_BUFFERS; i++) {
        audioBuffers[i].resize(BUFFER_SIZE);
        waveHeaders[i].lpData = (LPSTR)audioBuffers[i].data();
        waveHeaders[i].dwBufferLength = BUFFER_SIZE * sizeof(short);
        waveHeaders[i].dwFlags = 0;
        waveHeaders[i].dwLoops = 0;

        if (waveOutPrepareHeader(hWaveOut, &waveHeaders[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
            // Handle error
            continue;
        }

        // Generate initial silent buffer
        generateAudio((short*)waveHeaders[i].lpData, BUFFER_SIZE);
        if (waveOutWrite(hWaveOut, &waveHeaders[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
            // Handle error
            continue;
        }
    }

    isInitialized = true;
}

float AudioSystem::antiAliasFilter(float input) {
    // Simple one-pole lowpass filter
    filterState = filterState * SMOOTHING_FACTOR + input * (1.0f - SMOOTHING_FACTOR);
    return filterState;
}

float AudioSystem::softClip(float input) {
    // Soft clipping to reduce harsh distortion
    if (input > 1.0f) return 1.0f - expf(-input);
    if (input < -1.0f) return -1.0f + expf(input);
    return input;
}

void AudioSystem::generateAudio(short* buffer, int numSamples) {
    std::vector<float> mixBuffer(numSamples, 0.0f);

    bool anyNotePlaying = false;
    for (auto& note : notes) {
        if (note.isPlaying) {
            anyNotePlaying = true;
            note.envelope = (std::min)(1.0f, note.envelope + ATTACK_RATE);
        }
        else {
            note.envelope = (std::max)(0.0f, note.envelope - RELEASE_RATE);
        }

        if (note.envelope > 0.0f) {
            for (int i = 0; i < numSamples; i++) {
                float sample = std::sin(note.phase) * note.envelope * BASE_AMPLITUDE;
                note.phase += 2.0f * PI * note.frequency / SAMPLE_RATE;
                if (note.phase > 2.0f * PI) {
                    note.phase -= 2.0f * PI;
                }
                mixBuffer[i] += sample;
            }
        }
    }

    // If no notes are playing, fill with silence
    if (!anyNotePlaying) {
        std::memset(buffer, 0, numSamples * sizeof(short));
        return;
    }

    for (int i = 0; i < numSamples; i++) {
        float sample = softClip(mixBuffer[i]);
        buffer[i] = static_cast<short>(sample * 32767.0f);
    }
}

void AudioSystem::playNote(int noteIndex) {
    if (noteIndex >= 0 && noteIndex < notes.size()) {
        notes[noteIndex].isPlaying = true;
        // Don't reset phase to maintain continuity
        notes[noteIndex].velocity = 1.0f;
    }
}

void AudioSystem::stopNote(int noteIndex) {
    if (noteIndex >= 0 && noteIndex < notes.size()) {
        notes[noteIndex].isPlaying = false;
        // Let the envelope handle the fade out
    }
}


void AudioSystem::update() {
    if (!isInitialized) return;

    // Check all buffers
    for (int i = 0; i < NUM_BUFFERS; i++) {
        WAVEHDR* header = &waveHeaders[i];
        if ((header->dwFlags & WHDR_DONE) && !(header->dwFlags & WHDR_INQUEUE)) {
            generateAudio((short*)header->lpData, BUFFER_SIZE);
            waveOutWrite(hWaveOut, header, sizeof(WAVEHDR));
        }
    }
}

void CALLBACK AudioSystem::waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WOM_DONE) {
        AudioSystem* audio = (AudioSystem*)dwInstance;
        audio->update();
    }
}