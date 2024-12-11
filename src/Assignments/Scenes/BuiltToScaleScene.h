#pragma once
#include "Scene.h"
#include "AnimationObject.h"
#include "AudioSystem.h"

class BuiltToScaleScene : public Scene {
public:
    BuiltToScaleScene();
    ~BuiltToScaleScene() override;
    void init() override;
    void update(double now, float dt) override;
    void render(const mat4& projection, const mat4& view, bool isShadow) override;
    void renderUI() override;
    ptr_vector<AnimationObject> getObjects() override;

private:
    // Visual objects with models
    AnimationObject poleModel;  // BTS_PlayerPoleModel.obj
    AnimationObject leftGuideModel;  // BTS_PlayerWidgetModel.obj
    AnimationObject rightGuideModel; // BTS_PlayerWidgetModel.obj
    std::vector<AnimationObject> springModels;  // Mix of BTS_PlayerSpringModel and BTS_OtherSpringModel
    AnimationObject backgroundModel;

    // Visual objects
    std::vector<AnimationObject> springs;
    AnimationObject pole;
    AnimationObject inputIndicator;

    // Animation states
    enum class PoleState {
        Normal,
        Failed,
        Respawning,
        Launching
    };

    // Pattern types
    enum class Pattern {
        ThreeStep,   // 0->1->2
        FiveStep,    // 0->1->2->3->2
        EightStep    // 3->2->1->0->1->2->3->4->3
    };

    struct {
        int currentSpringIndex;
        bool movingRight;
        float animationTime;
        vec3 startPos;
        vec3 endPos;
        PoleState state;
        float failTime;
        bool inputRequired;
        bool inputSuccess;
    } poleAnim;

    struct SpringAnimation {
        bool isAnimating;
        float animationTime;
        float baseHeight;
        static constexpr float ANIM_DURATION = 0.2f;
        static constexpr float MAX_BOUNCE = 0.2f;
    };

    // New pattern tracking
    struct {
        Pattern type;
        int stepsRemaining;
        int totalSteps;
        bool springRetracted;      // Is red spring pulled back
        float retractAnimTime;     // For spring retraction animation
        float launchAnimTime;      // For launch animation
        bool awaitingLaunchInput;  // Waiting for K press
    } patternInfo;

    std::vector<SpringAnimation> springAnims;

    // Constants
    float BOUNCE_DURATION = 0.6122f;
    const float SPRING_SPACING = 2.0f;
    const float BOUNCE_HEIGHT = 1.0f;
    const int NUM_SPRINGS = 4;
    const float FAIL_DURATION = 1.0f;
    const float INPUT_WINDOW = 0.50f;
    const float POLE_Z_OFFSET = 0.0f;

    // New constants
    const float RETRACT_DURATION = 0.3f;
    const float LAUNCH_DURATION = 0.5f;
    const float LAUNCH_SPEED = 15.0f;

    // Helper functions
    void initSprings();
    void initPole();
    void initInputIndicator();
    void updatePoleAnimation(float dt);
    vec3 calculatePolePosition(float t);
    void startNewBounce();
    void handleInput();
    void failPole();
    void respawnPole();
    vec3 calculateFailPosition(float t);
    void updateInputIndicator(double now, float dt);
    void triggerSpringBounce(int springIndex);

    // New helper functions
    void selectRandomPattern();
    void updateSpringRetraction(float dt);
    void handleLaunchInput();
    void updateLaunchAnimation(float dt);
    bool isSecondToLastStep() const;
    bool isFinalStep() const;
    bool guideSquaresMovedDuringLaunch;

    AnimationObject leftGuideSquare;
    AnimationObject rightGuideSquare;
    float guideSquareWidth = 1.0f;
    float guideForwardOffset = 5.0f;
    bool guidesActive = false;
    void spawnGuideSquares();
    void moveGuideSquares();

    struct {
        bool isAnimating;
        float animationTime;
        float totalAnimationDuration;
        vec3 startLeftPos;
        vec3 startRightPos;
        vec3 endLeftPos;
        vec3 endRightPos;
    } squaresLaunchAnim;


    std::string currentSoundId;
    AudioSystem& audio;
    void playRandomImpactSound();
    const std::vector<int> TEMPO_BPM = { 76, 80, 98, 110, 120, 140 };
};