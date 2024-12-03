// BuiltToScaleScene.h

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
    // Game state
    enum class GameState {
        Playing,
        Failed
    } currentState;

    // Rod (pole) state
    struct Rod {
        AnimationObject object;
        bool isActive;
        bool isFalling;
        bool isThrown;
        vec3 startPosition;
        vec3 targetPosition;
        float progress;
        int currentBlockIndex;
        int direction; // 1 for right, -1 for left
        bool waitingForInput;
        bool waitingForThrow;
    } rod;

    // Blocks (the ones the rod bounces off)
    std::vector<AnimationObject> blocks;
    int redBlockIndex; // Index of the red block (user block)

    // Block animations
    struct BlockAnimation {
        bool isBouncing;
        float progress;
    };
    std::vector<BlockAnimation> blockAnimations;
    float redBlockPullProgress;

    // Squares that move towards each other
    struct SquarePair {
        AnimationObject leftSquare;
        AnimationObject rightSquare;
        bool isOverlapping;
        bool isHit;
        float animationProgress;
    };
    std::vector<SquarePair> squarePairs;

    // Timing and input
    double currentTime;
    float beatInterval;
    float timingTolerance;
    float lastInputTime;
    int beatCounter;

    // Animations
    void updateRod(double now, float dt);
    void updateBlocks(double now, float dt);
    void updateSquares(double now, float dt);
    void handleInput();

    // Helper functions
    void resetRod(bool startFromRight);
    void spawnSquarePair();
    void checkCollisionWithSquares();
    void removeOffscreenSquares();
    void playBlockBounceAnimation(int blockIndex);
    void playRodFallAnimation();
    void playSquareHitAnimation(SquarePair& pair);
    void startRodThrow();

    // Constants
    const float BLOCK_SPACING = 2.0f;
    const int NUM_BLOCKS = 4;
    const float ROD_SPEED = 2.0f;
    const float ROD_THROW_SPEED = 5.0f;
    const float ROD_FALL_SPEED = 3.0f;
    const float BLOCK_BOUNCE_HEIGHT = 0.5f;
    const float RED_BLOCK_PULLBACK_DISTANCE = 1.0f;
    const float SQUARE_MOVE_DISTANCE = 10.0f;
    const float SQUARE_OVERLAP_POSITION = 0.0f;
};
