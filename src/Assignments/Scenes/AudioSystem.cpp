#include "AudioSystem.h"
#include <algorithm>

AudioSystem::AudioSystem()
    : activeNotes(frequencies.size(), false)
    , phases(frequencies.size(), 0.0f)
    , currentBuffer(0)
    , isInitialized(false) {
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

    // Initialize audio buffers
    for (int i = 0; i < NUM_BUFFERS; i++) {
        audioBuffers[i].resize(BUFFER_SIZE);
        waveHeaders[i].lpData = (LPSTR)audioBuffers[i].data();
        waveHeaders[i].dwBufferLength = BUFFER_SIZE * sizeof(short);
        waveHeaders[i].dwFlags = 0;
        waveHeaders[i].dwLoops = 0;

        waveOutPrepareHeader(hWaveOut, &waveHeaders[i], sizeof(WAVEHDR));
    }

    isInitialized = true;

    // Start playing
    update();
}

void AudioSystem::generateAudio(short* buffer, int numSamples) {
    std::vector<float> tempBuffer(numSamples, 0.0f);

    // Mix all active frequencies
    for (size_t i = 0; i < frequencies.size(); i++) {
        if (activeNotes[i]) {
            float frequency = frequencies[i];
            for (int j = 0; j < numSamples; j++) {
                tempBuffer[j] += amplitude * std::sin(phases[i]);
                phases[i] += 2.0f * PI * frequency / SAMPLE_RATE;
                if (phases[i] > 2.0f * PI) {
                    phases[i] -= 2.0f * PI;
                }
            }
        }
    }

    // Convert to 16-bit PCM
    for (int i = 0; i < numSamples; i++) {
        buffer[i] = static_cast<short>(tempBuffer[i] * 32767.0f);
    }
}

void AudioSystem::playNote(int noteIndex) {
    if (noteIndex >= 0 && noteIndex < frequencies.size()) {
        activeNotes[noteIndex] = true;
    }
}

void AudioSystem::stopNote(int noteIndex) {
    if (noteIndex >= 0 && noteIndex < frequencies.size()) {
        activeNotes[noteIndex] = false;
    }
}

void AudioSystem::update() {
    if (!isInitialized) return;

    WAVEHDR* header = &waveHeaders[currentBuffer];

    if ((header->dwFlags & WHDR_DONE) || !(header->dwFlags & WHDR_INQUEUE)) {
        generateAudio((short*)header->lpData, BUFFER_SIZE);
        waveOutWrite(hWaveOut, header, sizeof(WAVEHDR));
        currentBuffer = (currentBuffer + 1) % NUM_BUFFERS;
    }
}

void CALLBACK AudioSystem::waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WOM_DONE) {
        AudioSystem* audio = (AudioSystem*)dwInstance;
        audio->update();
    }
}