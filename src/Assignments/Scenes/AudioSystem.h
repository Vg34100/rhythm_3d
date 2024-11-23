#pragma once
#include <vector>
#include <Windows.h>
#include <mmsystem.h>
#include <cmath>
#include <array>
#include <algorithm>

#pragma comment(lib, "winmm.lib")

class AudioSystem {
public:
    static AudioSystem& get() {
        static AudioSystem instance;
        return instance;
    }

    ~AudioSystem();
    void init();
    void playNote(int noteIndex);
    void stopNote(int noteIndex);
    void update();

private:
    AudioSystem();
    // Increased sample rate and buffer size for better quality
    static const int SAMPLE_RATE = 44100;    // Back to 44.1kHz
    static const int BUFFER_SIZE = 2048;     // Smaller buffer for less latency
    static const int NUM_BUFFERS = 4;        // More buffers for stability
    static const int NUM_CHANNELS = 1;

    struct Note {
        float frequency;
        bool isPlaying;
        float phase;
        float velocity;    // For envelope
        float envelope;    // Current envelope value
        float fadeTarget;  // Target for smooth transitions
    };

    std::vector<Note> notes = {
        {261.63f, false, 0.0f, 0.0f, 0.0f, 0.0f},  // C4
        {293.66f, false, 0.0f, 0.0f, 0.0f, 0.0f},  // D4
        {329.63f, false, 0.0f, 0.0f, 0.0f, 0.0f},  // E4
        {349.23f, false, 0.0f, 0.0f, 0.0f, 0.0f}   // F4
    };

    const float PI = 3.14159265358979323846f;
    const float BASE_AMPLITUDE = 0.3f;       // Increased amplitude

    // Envelope parameters
    const float ATTACK_RATE = 0.3f;         // Faster attack
    const float RELEASE_RATE = 0.5f;       // Faster release
    const float SMOOTHING_FACTOR = 0.9f;    // Less smoothing for more immediate response


    HWAVEOUT hWaveOut;
    std::array<WAVEHDR, NUM_BUFFERS> waveHeaders;
    std::array<std::vector<short>, NUM_BUFFERS> audioBuffers;
    int currentBuffer;
    bool isInitialized;

    // Anti-aliasing state
    float lastSample;
    float filterState;

    void generateAudio(short* buffer, int numSamples);
    static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    float antiAliasFilter(float input);
    float softClip(float input);
};