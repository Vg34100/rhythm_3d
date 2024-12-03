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
    // Core gameplay objects
    std::vector<AnimationObject> springBlocks;  // The 4 blocks that bounce the rod
    AnimationObject rod;                        // The horizontal rod that bounces
    AnimationObject ground;                     // Ground platform
    AnimationObject debugTimingBar;            // Visual timing indicator
    AnimationObject feedbackCube;              // Visual feedback for hits

    // Rod movement and state
    struct RodState {
        int currentBlock;           // Current block index (1-4)
        int direction;             // 1 for right, -1 for left
        float arcProgress;         // Progress through current arc (0-1)
        bool isActive;             // Whether rod is in play
        bool isThrowing;           // Whether rod is being thrown at squares
        vec3 throwStartPos;        // Start position for throw
        vec3 throwTargetPos;      // Target position for throw
        float throwProgress;       // Progress of throw animation
    } rodState;

    // Square pair management
    struct SquarePair {
        AnimationObject left;
        AnimationObject right;
        float progress;           // Movement progress (0-1)
        bool readyForCollision;   // Whether squares are about to overlap
        bool isColliding;         // Whether squares are currently colliding
        bool isDying;            // Whether squares are in death animation
        float deathProgress;     // Progress of death animation
    };
    std::vector<SquarePair> squarePairs;

    // Game state
    enum class GameState {
        Ready,
        Playing,
        Failed
    } currentState;

    // Timing and gameplay variables
    float currentTime;
    float lastBlockBounce;
    float blockBounceDuration;
    int score;
    bool redBlockPulledBack;
    float redBlockPullProgress;
    float timeSinceLastSpawn;
    const float SPAWN_INTERVAL = 4.0f;
    const float BLOCK_BOUNCE_HEIGHT = 0.2f;
    const float ROD_ARC_HEIGHT = 1.0f;

    // Audio system reference
    AudioSystem& audio;

    // Helper functions
    void updateRodMovement(float dt);
    void updateSquares(float dt);
    void updateBlockAnimations(float dt);
    void handleInput();
    void spawnNewSquarePair();
    void checkCollisions();
    void resetRod(bool startFromRight);
    vec3 calculateRodArcPosition(vec3 start, vec3 end, float progress);
    void cleanupDeadSquares();
};