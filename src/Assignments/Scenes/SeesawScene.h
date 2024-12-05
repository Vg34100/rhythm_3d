#pragma once
#include "Scene.h"
#include "AnimationObject.h"
#include "AudioSystem.h"

class SeesawScene : public Scene {
public:
    SeesawScene();
    ~SeesawScene() override;
    void init() override;
    void update(double now, float dt) override;
    void render(const mat4& projection, const mat4& view, bool isShadow) override;
    void renderUI() override;
    ptr_vector<AnimationObject> getObjects() override;

private:
    // Visual objects
    AnimationObject seesaw;
    AnimationObject playerCharacter;  // The right-side character
    AnimationObject npcCharacter;     // The left-side character

    // Animation state
    enum class GameState {
        StartSequence,   // NPC approaches seesaw
        PlayerJumping,   // Player is in the air
        NPCJumping      // NPC is in the air
    };
    GameState currentState;

    // Timing/Animation variables
    const float BPM = 90.0f;  // Slower tempo
    const float BEAT_DURATION = 60.0f / BPM;
    float currentTime;
    float sequenceStartTime;
    float jumpHeight = 5.0f;  // Higher jump
    float characterOffset = 0.8f;  // How far from seesaw ends the characters stand
    float characterBaseHeight = 0.4f;  // Base height above seesaw

    // Initial positions
    vec3 seesawPivotPoint;    // Center point of seesaw
    float seesawLength = 6.0f; // Length of seesaw
    float seesawTiltAngle = 15.0f; // Tilt angle in degrees
    float currentTiltAngle = 0.0f; // Current actual tilt angle
    float tiltSpeed = 360.0f;  // Degrees per second for tilt animation

    // Helper functions
    void updateSeesawTilt(float dt);
    void updateCharacterPositions(float dt);
    vec3 calculateJumpPosition(float t, const vec3& startPos, const vec3& peakPos);
    void initializePositions();
    vec3 calculateSeesawEndPoint(float side, float tiltAngle);

    // Audio system reference
    AudioSystem& audio;
};