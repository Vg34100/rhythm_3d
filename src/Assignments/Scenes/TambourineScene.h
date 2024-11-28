#pragma once
#include "Scene.h"
#include "AnimationObject.h"
#include "AudioSystem.h"

struct RhythmNote {
    enum class Type { Shake, Clap };
    Type type;
    float timing; // In beats (0.0 - 4.0 for a 4/4 measure)
};

struct RhythmSequence {
    std::vector<RhythmNote> notes;
};

class TambourineScene : public Scene {
public:
    TambourineScene();
    ~TambourineScene() override;
    void init() override;
    void update(double now, float dt) override;
    void render(const mat4& projection, const mat4& view, bool isShadow) override;
    void renderUI() override;
    ptr_vector<AnimationObject> getObjects() override;

private:
    // Visual objects
    AnimationObject monkey;
    AnimationObject monkeyTambourine;
    AnimationObject player;
    AnimationObject playerTambourine;
    AnimationObject playerIndicator;
    AnimationObject feedbackCube;
    AnimationObject timingIndicator; // New: shows current beat position

    // Game state
    enum class GameState {
        Intro,       // New: show instructions
        MonkeyTurn,
        PlayerTurn,
        Evaluation
    };
    GameState currentState;

    // Rhythm sequences
    std::vector<RhythmSequence> availableSequences;
    RhythmSequence currentSequence;
    size_t currentNoteIndex;

    // Timing
    const float BPM = 60.0f;  // Slower default tempo
    const float BEAT_DURATION = 60.0f / BPM;
    const float TIMING_TOLERANCE = 0.3f; // More forgiving timing window
    float sequenceStartTime;
    float currentTime;

    // Visual feedback
    float beatPulseScale;
    float lastBeatTime;
    int currentBeatCount;
    std::vector<float> playerTimings; // Store actual timings for feedback

    // Player input tracking
    std::vector<RhythmNote> playerSequence;
    bool lastShakeState;
    bool lastClapState;
    bool showFeedbackCube;
    int comboCount;        // Track successful hits in a row
    float lastInputTime;   // For showing timing feedback

    // Animation states
    struct AnimationState {
        bool isShaking;
        bool isClapping;
        float animationTime;
    };
    AnimationState monkeyAnim;
    AnimationState playerAnim;

    // Audio system reference
    AudioSystem& audio;

    // Helper functions
    void initializeSequences();
    void startNewSequence();
    void updateMonkeyTurn(float dt);
    void updatePlayerTurn(float dt);
    void evaluatePlayerSequence();
    void updateAnimations(float dt);
    void updateBeatVisuals(float dt);
    void renderMonkey(const mat4& projection, const mat4& view, bool isShadow);
    void renderPlayer(const mat4& projection, const mat4& view, bool isShadow);
    std::string getTimingFeedback(float difference);
    bool useAlternateSound = false;
};