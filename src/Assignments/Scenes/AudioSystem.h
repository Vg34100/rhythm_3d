#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "WaveSound.h"
#include <miniaudio.h>

class AudioSystem {
public:
    static AudioSystem& get() {
        static AudioSystem instance;
        return instance;
    }

    // Scene Management
    void loadScene(const std::string& sceneName);
    void unloadScene();

    // Sound Control
    void playSound(const std::string& soundId);
    void stopSound(const std::string& soundId);
    void stopAllSounds();
    void setVolume(const std::string& soundId, float volume);
    void update(); // Called every frame to clean up finished sounds

    // Debug helpers
    bool doesFileExist(const std::string& filepath);

private:
    AudioSystem();
    ~AudioSystem();
    static constexpr size_t MAX_CONCURRENT_SOUNDS = 32; // Limit concurrent sounds
    size_t currentPlayingSounds = 0;

    // Prevent copying
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    struct AudioScene {
        std::unordered_map<std::string, std::unique_ptr<WaveSound>> sounds;
    };

    std::unique_ptr<AudioScene> currentScene;
    std::string currentSceneName;

    // Helper function to build filepath
    std::string buildSoundPath(const std::string& soundId);

    // Shared audio engine
    ma_engine engine;
};
