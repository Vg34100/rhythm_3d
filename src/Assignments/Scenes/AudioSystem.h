#pragma once
#include <vector>
#include <Windows.h>
#include <mmsystem.h>
#include <cmath>
#include <array>

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
    static const int SAMPLE_RATE = 44100;
    static const int BUFFER_SIZE = 4096;
    static const int NUM_BUFFERS = 2;
    static const int NUM_CHANNELS = 1;

    std::vector<float> frequencies = {
        261.63f,  // C4
        293.66f,  // D4
        329.63f,  // E4
        349.23f   // F4
    };

    std::vector<bool> activeNotes;
    std::vector<float> phases;
    const float PI = 3.14159265358979323846f;
    const float amplitude = 0.2f;

    HWAVEOUT hWaveOut;
    std::array<WAVEHDR, NUM_BUFFERS> waveHeaders;
    std::array<std::vector<short>, NUM_BUFFERS> audioBuffers;
    int currentBuffer;
    bool isInitialized;

    void generateAudio(short* buffer, int numSamples);
    static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
};
