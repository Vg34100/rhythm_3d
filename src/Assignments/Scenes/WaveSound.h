#pragma once

#ifndef MA_API
#define MA_API extern
#endif
#include "miniaudio.h"

#include <string>

class WaveSound {
public:
    WaveSound(const std::string& filepath, ma_engine* engine);
    ~WaveSound();

    bool load();
    void play();
    void stop();
    void setVolume(float volume);

private:
    std::string filepath;
    bool isLoaded;
    float volume;
    ma_engine* engine;  // Pointer to shared engine

    ma_sound sound;     // Handle to the sound
    bool isPlaying;
};
