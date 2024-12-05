// AudioSystem.cpp
// Implementation of the Audio System using Windows Multimedia API
// Each scene manages its own collection of sounds, and the system
// ensures proper loading/unloading when switching scenes.

#include "AudioSystem.h"
#include <iostream>
#include <filesystem>

AudioSystem::AudioSystem() {
    // Initialize the audio engine
    if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
        std::cout << "Failed to initialize audio engine." << std::endl;
    }
}

AudioSystem::~AudioSystem() {
    unloadScene();
    ma_engine_uninit(&engine);
}

void AudioSystem::loadScene(const std::string& sceneName) {
    std::cout << "Loading audio scene: " << sceneName << std::endl;

    // Unload current scene if any
    unloadScene();

    currentScene = std::make_unique<AudioScene>();
    currentSceneName = sceneName;

    // Load all sounds for the scene
    std::vector<std::string> soundIds;
    if (sceneName == "tambourine") {
        soundIds = {
            "tamb_monkey_a",
            "tamb_monkey_a_2",
            "tamb_monkey_ab",
            "tamb_monkey_happy",
            "tamb_monkey_squeal",
            "tamb_player_a",
            "tamb_player_a_2",
            "tamb_player_ab"
        };
    }
    else if (sceneName == "builttoscale") {
        soundIds = {
            "builttoscale_impactA",
            "builttoscale_impactB",
            "builttoscale_impactThrow"
        };
    }
    else if (sceneName == "seesaw") {
        soundIds = {
            "seesaw_see_normal",
            "seesaw_saw_normal",
            "seesaw_see_quick",
            "seesaw_saw_quick",

        };
    }

    bool anyLoadFailed = false;
    for (const auto& soundId : soundIds) {
        std::string filepath = buildSoundPath(soundId);
        currentScene->sounds[soundId] = std::make_unique<WaveSound>(filepath, &engine);
        if (!currentScene->sounds[soundId]->load()) {
            std::cout << "Failed to load sound: " << soundId << std::endl;
            anyLoadFailed = true;
        }
    }

    if (anyLoadFailed) {
        std::cout << "Some sounds failed to load! Check file paths and formats." << std::endl;
    }
    else {
        std::cout << "All sounds loaded successfully!" << std::endl;
    }
}


void AudioSystem::unloadScene() {
    if (currentScene) {
        stopAllSounds();
        currentScene.reset();
        currentSceneName.clear();
    }
}

void AudioSystem::playSound(const std::string& soundId) {
    if (currentScene && currentScene->sounds.count(soundId) > 0) {
        currentScene->sounds[soundId]->play();
    }
}


void AudioSystem::stopSound(const std::string& soundId) {
    if (currentScene && currentScene->sounds.count(soundId) > 0) {
        currentScene->sounds[soundId]->stop();
    }
}

void AudioSystem::stopAllSounds() {
    if (currentScene) {
        for (auto& [_, sound] : currentScene->sounds) {
            sound->stop();
        }
    }
}

void AudioSystem::setVolume(const std::string& soundId, float volume) {
    if (currentScene && currentScene->sounds.count(soundId) > 0) {
        currentScene->sounds[soundId]->setVolume(volume);
    }
}

void AudioSystem::update() {
    // No update needed for this implementation
}

std::string AudioSystem::buildSoundPath(const std::string& soundId) {
    std::string path = "../assets/audio/" + currentSceneName + "/" + soundId + ".wav";
    std::cout << "Attempting to load sound: " << soundId << std::endl;
    std::cout << "Full path: " << std::filesystem::absolute(path).string() << std::endl;
    if (std::filesystem::exists(path)) {
        std::cout << "File exists!" << std::endl;
        return path;
    }
    else {
        std::cout << "File not found at: " << path << std::endl;
        // Try alternative paths
        std::vector<std::string> altPaths = {
            "assets/audio/" + currentSceneName + "/" + soundId + ".wav",
            "../../assets/audio/" + currentSceneName + "/" + soundId + ".wav",
            "../../../assets/audio/" + currentSceneName + "/" + soundId + ".wav"
        };

        for (const auto& altPath : altPaths) {
            std::cout << "Trying alternative path: " << altPath << std::endl;
            if (std::filesystem::exists(altPath)) {
                std::cout << "Found file at: " << altPath << std::endl;
                return altPath;
            }
        }
        std::cout << "Could not find sound file anywhere!" << std::endl;
        return path; // Return original path even though it doesn't exist
    }
}

bool AudioSystem::testAudioPlayback() {
    // First try a simple beep using Windows API
    //std::cout << "Testing basic Windows sound..." << std::endl;
    //Beep(440, 500); // 440Hz for 500ms

    //// Try to play a test WAV file
    //std::string testPath = "test.wav";
    //std::cout << "Attempting to play test WAV at: " << std::filesystem::absolute(testPath).string() << std::endl;


    //std::wstring wtestPath(testPath.begin(), testPath.end());
    //bool success = PlaySoundW(wtestPath.c_str(), NULL, SND_FILENAME | SND_SYNC);
    //if (success) {
    //    std::cout << "Successfully played test WAV" << std::endl;
    //}
    //else {
    //    std::cout << "Failed to play test WAV" << std::endl;
    //}

    return true;
}

void AudioSystem::debugPrintPaths() {
    // Print current working directory
    //wchar_t wbuffer[MAX_PATH];
    //GetCurrentDirectoryW(MAX_PATH, wbuffer);
    //char buffer[MAX_PATH];
    //wcstombs(buffer, wbuffer, MAX_PATH);
    //std::cout << "Current working directory: " << buffer << std::endl;

    //// Print module path (exe location)
    //GetModuleFileNameW(NULL, wbuffer, MAX_PATH);
    //wcstombs(buffer, wbuffer, MAX_PATH);
    //std::cout << "Executable path: " << buffer << std::endl;

    //// Print assets path
    //std::string assetsPath = "assets/audio/tambourine/";
    //std::cout << "Trying to access assets at: " << std::filesystem::absolute(assetsPath).string() << std::endl;

    //// List all files in current directory
    //std::cout << "\nFiles in current directory:" << std::endl;
    //try {
    //    for (const auto& entry : std::filesystem::directory_iterator(".")) {
    //        std::cout << entry.path().string() << std::endl;
    //    }
    //}
    //catch (const std::filesystem::filesystem_error& e) {
    //    std::cout << "Error listing directory: " << e.what() << std::endl;
    //}
}

bool AudioSystem::doesFileExist(const std::string& filepath) {
    return std::filesystem::exists(filepath);
}