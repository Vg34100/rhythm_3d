#define MINIAUDIO_IMPLEMENTATION
#define MA_API

#include "miniaudio.h"
#include "WaveSound.h"
#include <iostream>

WaveSound::WaveSound(const std::string& filepath, ma_engine* engine)
    : filepath(filepath), isLoaded(false), volume(1.0f), engine(engine), isPlaying(false) {
}

WaveSound::~WaveSound() {
    stop();
    if (isLoaded) {
        ma_sound_uninit(&sound);
    }
}

bool WaveSound::load() {
    if (isLoaded) return true;

    if (ma_sound_init_from_file(engine, filepath.c_str(), 0, NULL, NULL, &sound) != MA_SUCCESS) {
        std::cout << "Failed to load sound: " << filepath << std::endl;
        return false;
    }

    isLoaded = true;
    return true;
}

void WaveSound::play() {
    if (!isLoaded && !load()) return;

    if (ma_sound_start(&sound) != MA_SUCCESS) {
        std::cout << "Failed to start sound: " << filepath << std::endl;
        return;
    }

    isPlaying = true;
}

void WaveSound::stop() {
    if (isPlaying) {
        ma_sound_stop(&sound);
        isPlaying = false;
    }
}

void WaveSound::setVolume(float vol) {
    volume = vol;
    if (isLoaded) {
        ma_sound_set_volume(&sound, volume);
    }
}
