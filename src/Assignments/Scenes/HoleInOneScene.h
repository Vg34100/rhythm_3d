#pragma once
#include "Scene.h"
#include "AnimationObject.h"
#include "AudioSystem.h"

class HoleInOneScene : public Scene {
public:
    HoleInOneScene();
    ~HoleInOneScene() override;
    void init() override;
    void update(double now, float dt) override;
    void render(const mat4& projection, const mat4& view, bool isShadow) override;
    void renderUI() override;
    ptr_vector<AnimationObject> getObjects() override;

private:
    // Characters
    AnimationObject player;      // Main player (white box)
    AnimationObject golfClub;    // Player's golf club
    AnimationObject monkey;      // Monkey character (brown box)
    AnimationObject mandrill;    // Mandrill character (gray box)
    AnimationObject island;      // Destination island
    AnimationObject golfBall;    // The golf ball being thrown/hit


    AnimationObject mandrillModel;  // HIO_Monkey.obj
    AnimationObject monkeyModel;  // HIO_Mandrill.obj
    AnimationObject islandModel;  // HIO_Island.obj
    AnimationObject backgroundModel;  // HIO_background.obj


    // UI Elements
    AnimationObject playerIndicator;  // Yellow indicator above player
    AnimationObject feedbackCube;     // Feedback cube for timing

    // Game State
    enum class GameState {
        Inactive,       // Waiting between patterns
        MonkeyPrep,     // Monkey preparing throw (1 beat)
        MonkeyThrow,    // Ball in air towards player (1 beat)
        BallFlight,     // Ball flying to island (2 beats)
        MandrillPrep1,  // First prep beat
        MandrillPrep2,  // Second prep beat
        MandrillPrep3,  // Third prep beat
        MandrillThrow   // Throw and instant hit
    };

    // Input timing results
    enum class TimingResult {
        Perfect,
        Good,
        Bad,
        Miss
    };

    struct ThrowPattern {
        bool isMandrill;
        int waitBeats;  // 0-2 beats between patterns
    };

    // Timing and Animation Constants
    const float BPM = 115.0f;
    const float BEAT_DURATION = 60.0f / BPM;
    const float PREP_COLOR_INTENSITY = 0.3f;     // Color change during prep
    const float BALL_OFFSET = 0.6f;              // How far in front of thrower the ball appears
    const float PLAYER_BALL_OFFSET = 0.8f;       // Where the ball should be relative to player for hitting
    const float GOLF_CLUB_LENGTH = 1.2f;         // Length of the golf club
    const float GOLF_CLUB_THICKNESS = 0.15f;     // Thickness of the golf club
    const float MONKEY_THROW_HEIGHT = 3.0f;      // Height of monkey's throw arc
    const float BALL_FLIGHT_HEIGHT = 5.0f;       // Height of ball flight to island

    // Input timing windows (in seconds)
    const float PERFECT_WINDOW = 0.05f;  // �0.05s for perfect
    const float GOOD_WINDOW = 0.15f;      // �0.1s for good
    const float BAD_WINDOW = 0.25f;       // �0.2s for bad, beyond is miss
    bool soundPlayed = false;

    GameState currentState;
    float currentBeat;
    float stateStartTime;
    float currentTime;
    ThrowPattern currentPattern;
    vec3 ballStartPos;
    vec3 ballEndPos;

    // Animation variables
    float golfClubAngle;        // Current angle of the golf club
    bool swingInProgress;       // Whether a swing is currently happening
    float swingProgress;        // Progress of current swing (0 to 1)
    bool hasInputThisTurn;      // Track if player has input during current turn

    bool validHit;
    TimingResult lastHitResult;

    // Audio system reference
    AudioSystem& audio;

    // Helper methods
    void updateBallPosition(float t);
    ThrowPattern selectNextPattern();
    void initializePositions();
    void updateGolfClub(float dt);
    vec3 calculateArcPosition(const vec3& start, const vec3& end, float height, float t);
    void startNewPattern();
    vec3 getThrowPosition(bool isMandrill) const;  // Gets position in front of thrower
    vec3 getPlayerHitPosition() const;             // Gets position where player should hit ball
    void handlePlayerInput();                      // New method for input handling
    TimingResult checkTiming(float stateTime);     // New method for checking input timing
    void updateFeedbackCube(TimingResult result);

};