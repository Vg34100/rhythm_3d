// AudioSystem.h
// This header defines a singleton audio system that manages sound loading and playback
// using the Windows Multimedia API (winmm.lib). It supports:
// - Scene-based sound organization
// - WAV file loading
// - Multiple simultaneous sound playback
// - Basic volume control
// Key components:
// - WaveSound: Represents a loaded WAV file
// - AudioScene: Manages sounds for a specific game scene
// - AudioSystem: Main singleton managing everything

#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

class WaveSound {
public:
    WaveSound(const std::string& filepath);
    ~WaveSound() = default;

    bool load();
    void play();
    void stop();
    void setVolume(float volume); // 0.0 to 1.0

private:
    std::string filepath;
    bool isLoaded;
    float volume;
};

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
    bool testAudioPlayback();
    void debugPrintPaths();
    bool doesFileExist(const std::string& filepath);

private:
    AudioSystem() = default;
    ~AudioSystem();

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
};