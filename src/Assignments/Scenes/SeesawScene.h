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
    // Position types
    enum class Position {
        FarSide,
        CloseSide
    };

    // Pattern definition
    struct JumpPattern {
        Position startPos;
        Position endPos;
        bool isHighJump;
    };

    // Character state tracking
    struct CharacterState {
        Position currentPos;
        JumpPattern currentPattern;
        bool isJumping;
        float jumpStartTime;
        bool isHighJump;
    };

    // Visual objects (existing)
    AnimationObject seesaw;
    AnimationObject playerCharacter;
    AnimationObject npcCharacter;
    AnimationObject feedbackCube;
    AnimationObject playerIndicator;

    // Timing enums (existing)
    enum class TimingResult {
        None,
        Perfect,
        Good,
        Bad,
        Miss
    };

    // Game states (modified)
    enum class GameState {
        StartSequence,
        PlayerJumping,
        NPCJumping
    };

    // Timing/Animation variables (existing + new)
    const float BASE_BPM = 60.0f;
    const float CLOSE_POSITION_BPM_MULTIPLIER = 2.0f;
    const float BASE_BEAT_DURATION = 60.0f / BASE_BPM;
    const float HIGH_JUMP_PROBABILITY = 0.1f; // 10% chance for high jump
    const float HIGH_JUMP_MULTIPLIER = 2.0f;

    float currentTime;
    float sequenceStartTime;
    float jumpHeight = 5.0f;
    float baseJumpHeight = 5.0f;
    float characterOffset = 0.8f;
    float closePositionOffset = 0.4f; // Offset for close position
    float characterBaseHeight = 0.4f;

    // Pattern tracking
    CharacterState playerState;
    CharacterState npcState;
    GameState currentState;

    // Position constants
    //const float FAR_POSITION_OFFSET = 0.8f;
    //const float CLOSE_POSITION_OFFSET = 0.4f;

    // Existing timing variables
    bool lastInputState = false;
    TimingResult lastTimingResult = TimingResult::None;
    float badTimingAnimationTime = 0.0f;
    const float BAD_ANIMATION_DURATION = 0.5f;

    // Seesaw properties
    vec3 seesawPivotPoint;
    float seesawLength = 6.0f;
    float seesawTiltAngle = 15.0f;
    float currentTiltAngle = 0.0f;
    float tiltSpeed = 360.0f;
    float beatPulseScale = 1.0f;

    // Input tracking
    bool hasInputThisBeat = false;
    float lastFeedbackTime = 0.0f;
    const float FEEDBACK_DURATION = 1.0f;

    // Timing windows
    const float PERFECT_WINDOW = 0.01f;
    const float GOOD_WINDOW = 0.03f;
    const float BAD_WINDOW = 0.35f;

    // Position constants
    const float SEESAW_LENGTH = 12.0f;  // Doubled from original
    const float FAR_POSITION_OFFSET = SEESAW_LENGTH * 0.4f;  // Far from center
    const float CLOSE_POSITION_OFFSET = SEESAW_LENGTH * 0.15f; // Much closer to center
    const float CHARACTER_BASE_HEIGHT = 0.4f;

    // New helper functions for pattern system
    void selectNewPattern(CharacterState& state);
    float getCurrentBPM(Position startPos) const;
    float getJumpDuration(Position startPos) const;
    vec3 getPositionForCharacter(bool isPlayer, Position pos) const;
    bool shouldTriggerHighJump() const;
    void updateCharacterJump(CharacterState& state, AnimationObject& character, float dt);

    // Existing helper functions
    void updateSeesawTilt(float dt);
    void updateCharacterPositions(float dt);
    //vec3 calculateJumpPosition(float t, const vec3& startPos, const vec3& peakPos);
    vec3 calculateJumpPosition(float t, const vec3& startPos, const vec3& endPos, const vec3& peakPos);

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