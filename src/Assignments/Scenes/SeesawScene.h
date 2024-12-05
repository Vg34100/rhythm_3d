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
    AnimationObject playerCharacter;
    AnimationObject npcCharacter;
    AnimationObject feedbackCube;
    AnimationObject playerIndicator;

    // Timing result enums
    enum class TimingResult {
        None,
        Perfect,
        Good,
        Bad,
        Miss
    };

    // Animation state
    enum class GameState {
        StartSequence,
        PlayerJumping,
        NPCJumping
    };
    GameState currentState;


    // Timing/Animation variables
    const float BPM = 60.0f;  // Even slower tempo
    const float BEAT_DURATION = 60.0f / BPM;
    float currentTime;
    float sequenceStartTime;
    float jumpHeight = 5.0f;
    float characterOffset = 0.8f;
    float characterBaseHeight = 0.4f;
    bool lastInputState = false;
    TimingResult lastTimingResult = TimingResult::None;
    float badTimingAnimationTime = 0.0f;
    const float BAD_ANIMATION_DURATION = 0.5f;

    // Initial positions
    vec3 seesawPivotPoint;
    float seesawLength = 6.0f;
    float seesawTiltAngle = 15.0f;
    float currentTiltAngle = 0.0f;
    float tiltSpeed = 360.0f;
    float beatPulseScale = 1.0f;  // For feedback cube animation

    // Add these variables
    bool hasInputThisBeat = false;  // Track if player has input this beat
    float lastFeedbackTime = 0.0f;  // When we last showed feedback
    const float FEEDBACK_DURATION = 1.0f;  // How long to show feedback

    // Timing windows (in percentage of beat duration)
    const float PERFECT_WINDOW = 0.01f;  // ±15% of beat for perfect
    const float GOOD_WINDOW = 0.05f;     // ±25% of beat for good
    const float BAD_WINDOW = 0.35f;      // ±35% of beat for bad

    // Helper functions
    void updateSeesawTilt(float dt);
    void updateCharacterPositions(float dt);
    vec3 calculateJumpPosition(float t, const vec3& startPos, const vec3& peakPos);
    void initializePositions();
    vec3 calculateSeesawEndPoint(float side, float tiltAngle);
    void handlePlayerInput();
    TimingResult checkTiming(float currentPhase);
    void updateFeedbackCube(float dt);
    void updateBadTimingAnimation(float dt);
    vec4 getTimingResultColor(TimingResult result);

    // Audio system reference
    AudioSystem& audio;
};