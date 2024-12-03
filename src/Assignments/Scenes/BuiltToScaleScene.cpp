#include "BuiltToScaleScene.h"
#include "AnimationObjectRenderer.h"
#include "imgui.h"
#include "Input.h"
#include <iostream>

BuiltToScaleScene::BuiltToScaleScene()
    : currentState(GameState::Ready)
    , currentTime(0.0f)
    , lastBlockBounce(0.0f)
    , blockBounceDuration(0.3f)
    , score(0)
    , redBlockPulledBack(false)
    , redBlockPullProgress(0.0f)
    , timeSinceLastSpawn(0.0f)
    , audio(AudioSystem::get()) {

    // Initialize rod state
    rodState = {
        3,      // Start at block 3 (red block)
        -1,     // Moving left initially
        0.0f,   // No arc progress
        true,   // Rod is active
        false,  // Not throwing
        vec3(0.0f), // Will be set in init
        vec3(0.0f), // Will be set in init
        0.0f    // No throw progress
    };
}

BuiltToScaleScene::~BuiltToScaleScene() {
    // Cleanup
}

void BuiltToScaleScene::init() {
    // Initialize spring blocks in a row
    springBlocks.resize(4);
    for (int i = 0; i < 4; i++) {
        springBlocks[i] = AnimationObject(AnimationObjectType::box);
        springBlocks[i].localPosition = vec3(-3.0f + i * 2.0f, 0.0f, -3.0f);
        springBlocks[i].localScale = vec3(0.8f);
        // Make block 3 (index 2) red for player interaction
        springBlocks[i].color = (i == 2) ? vec4(1.0f, 0.2f, 0.2f, 1.0f) : vec4(0.7f);
        springBlocks[i].updateMatrix(true);
    }

    // Initialize rod (horizontal orientation)
    rod = AnimationObject(AnimationObjectType::box);
    rod.localScale = vec3(0.8f, 0.15f, 0.15f); // Longer in x-axis for horizontal orientation
    rod.color = vec4(0.8f, 0.0f, 0.0f, 1.0f);
    rod.localPosition = springBlocks[2].localPosition + vec3(0.0f, 0.5f, 0.0f);
    rod.updateMatrix(true);

    // Initialize ground
    ground = AnimationObject(AnimationObjectType::box);
    ground.localPosition = vec3(0.0f, -2.0f, 0.0f);
    ground.localScale = vec3(12.0f, 0.2f, 8.0f);
    ground.color = vec4(0.3f, 0.3f, 0.3f, 1.0f);
    ground.updateMatrix(true);

    // Initialize debug timing bar
    debugTimingBar = AnimationObject(AnimationObjectType::box);
    debugTimingBar.localPosition = vec3(0.0f, 2.0f, 0.0f);
    debugTimingBar.localScale = vec3(4.0f, 0.1f, 0.1f);
    debugTimingBar.color = vec4(0.5f, 0.5f, 0.5f, 1.0f);
    debugTimingBar.updateMatrix(true);

    // Initialize feedback cube
    feedbackCube = AnimationObject(AnimationObjectType::box);
    feedbackCube.localPosition = vec3(0.0f, 3.0f, 0.0f);
    feedbackCube.localScale = vec3(0.5f);
    feedbackCube.color = vec4(0.5f, 0.5f, 0.5f, 1.0f);
    feedbackCube.updateMatrix(true);

    // Load audio
    audio.loadScene("builttoscale");

    // Spawn initial square pair
    spawnNewSquarePair();
}

void BuiltToScaleScene::update(double now, float dt) {
    currentTime += dt;

    switch (currentState) {
    case GameState::Ready:
        if (currentTime >= 2.0f) {
            currentState = GameState::Playing;
        }
        break;

    case GameState::Playing:
        handleInput();
        updateRodMovement(dt);
        updateSquares(dt);
        updateBlockAnimations(dt);
        checkCollisions();
        cleanupDeadSquares();

        // Handle square pair spawning
        timeSinceLastSpawn += dt;
        if (timeSinceLastSpawn >= SPAWN_INTERVAL) {
            spawnNewSquarePair();
            timeSinceLastSpawn = 0.0f;
        }
        break;

    case GameState::Failed:
        // Allow a brief pause before resetting
        if (currentTime - lastBlockBounce > 1.0f) {
            resetRod(rand() % 2 == 0); // Randomly choose start side
            currentState = GameState::Playing;
        }
        break;
    }

    // Update all matrices
    for (auto& block : springBlocks) {
        block.updateMatrix(true);
    }
    rod.updateMatrix(true);
    ground.updateMatrix(true);
    debugTimingBar.updateMatrix(true);
    feedbackCube.updateMatrix(true);

    for (auto& pair : squarePairs) {
        pair.left.updateMatrix(true);
        pair.right.updateMatrix(true);
    }
}

void BuiltToScaleScene::updateRodMovement(float dt) {
    if (!rodState.isActive) return;

    if (rodState.isThrowing) {
        // Update throw animation
        rodState.throwProgress += dt * 2.0f;
        if (rodState.throwProgress >= 1.0f) {
            // Reset rod state if we missed the squares
            if (!squarePairs.empty() && !squarePairs[0].isColliding) {
                rodState.isActive = false;
                currentState = GameState::Failed;
            }
            rodState.isThrowing = false;
            rodState.throwProgress = 0.0f;
        }
        else {
            // Calculate throw position using bezier curve
            float t = rodState.throwProgress;
            vec3 control = (rodState.throwStartPos + rodState.throwTargetPos) * 0.5f + vec3(0.0f, 2.0f, 0.0f);
            vec3 p0 = rodState.throwStartPos;
            vec3 p1 = control;
            vec3 p2 = rodState.throwTargetPos;

            float u = 1.0f - t;
            rod.localPosition = u * u * p0 + 2.0f * u * t * p1 + t * t * p2;

            // Add spinning animation during throw
            rod.localRotation = vec3(0.0f, 0.0f, t * 720.0f);
        }
        return;
    }

    // Normal bouncing movement between blocks
    rodState.arcProgress += dt * 2.0f;
    if (rodState.arcProgress >= 1.0f) {
        // Move to next block
        rodState.currentBlock += rodState.direction;

        // Change direction at ends
        if (rodState.currentBlock <= 0 || rodState.currentBlock >= 3) {
            rodState.direction *= -1;
        }

        rodState.arcProgress = 0.0f;
        lastBlockBounce = currentTime;
    }

    // Calculate start and end positions for current arc
    int currentIdx = rodState.currentBlock;
    int nextIdx = rodState.currentBlock + rodState.direction;
    vec3 start = springBlocks[currentIdx].localPosition + vec3(0.0f, 0.5f, 0.0f);
    vec3 end = springBlocks[nextIdx].localPosition + vec3(0.0f, 0.5f, 0.0f);

    // Calculate current position along arc
    rod.localPosition = calculateRodArcPosition(start, end, rodState.arcProgress);

    // Keep rod horizontal but rotate slightly based on arc position
    float angle = 15.0f * sin(rodState.arcProgress * M_PI);
    rod.localRotation = vec3(0.0f, 0.0f, angle);
}

vec3 BuiltToScaleScene::calculateRodArcPosition(vec3 start, vec3 end, float progress) {
    // Calculate arc height using sine wave
    float height = sin(progress * M_PI) * ROD_ARC_HEIGHT;

    // Linear interpolation between start and end points
    vec3 base = start * (1.0f - progress) + end * progress;

    // Add height to y component
    return base + vec3(0.0f, height, 0.0f);
}

void BuiltToScaleScene::updateSquares(float dt) {
    const float ROLL_SPEED = 0.5f;

    for (auto& pair : squarePairs) {
        if (pair.isDying) {
            // Update death animation
            pair.deathProgress += dt * 2.0f;
            if (pair.deathProgress >= 1.0f) continue;

            // Fall and spin animation
            float t = pair.deathProgress;
            float fallHeight = -5.0f * t * t;
            float spin = t * 360.0f;

            vec3 basePos = (pair.left.localPosition + pair.right.localPosition) * 0.5f;
            vec3 pushBack = vec3(0.0f, 0.0f, -2.0f * t);

            pair.left.localPosition = basePos + pushBack + vec3(0.0f, fallHeight, 0.0f);
            pair.right.localPosition = basePos + pushBack + vec3(0.0f, fallHeight, 0.0f);

            pair.left.localRotation = vec3(spin, 0.0f, 0.0f);
            pair.right.localRotation = vec3(spin, 0.0f, 0.0f);

        }
        else if (!pair.isColliding) {
            // Normal square movement
            pair.progress += dt * ROLL_SPEED;

            float rollDist = 6.0f;
            float leftPos = -rollDist + pair.progress * 2.0f * rollDist;
            float rightPos = rollDist - pair.progress * 2.0f * rollDist;

            pair.left.localPosition = vec3(leftPos, 0.0f, 2.0f);
            pair.right.localPosition = vec3(rightPos, 0.0f, 2.0f);

            // Check if squares are about to overlap
            pair.readyForCollision = abs(leftPos - rightPos) < 1.0f && !pair.isColliding;

            // Auto pull-back red block when squares are about to collide
            if (pair.readyForCollision && !redBlockPulledBack &&
                rodState.currentBlock == 2 && !rodState.isThrowing) {
                redBlockPulledBack = true;
                redBlockPullProgress = 0.0f;
            }
        }
    }

    // Update red block pull-back animation
    if (redBlockPulledBack) {
        redBlockPullProgress += dt * 3.0f;
        float pullback = sin(redBlockPullProgress * M_PI) * 0.5f;
        springBlocks[2].localPosition.z = -3.0f - pullback;

        if (redBlockPullProgress >= 1.0f) {
            redBlockPulledBack = false;
            springBlocks[2].localPosition.z = -3.0f;
        }
    }
}

void BuiltToScaleScene::updateBlockAnimations(float dt) {
    // Bounce animation for blocks when hit by rod
    if (currentTime - lastBlockBounce < blockBounceDuration) {
        float t = (currentTime - lastBlockBounce) / blockBounceDuration;
        float bounce = sin(t * M_PI) * BLOCK_BOUNCE_HEIGHT;

        springBlocks[rodState.currentBlock].localPosition.y = bounce;
    }
    else {
        // Reset block positions
        for (auto& block : springBlocks) {
            block.localPosition.y = 0.0f;
        }
    }
}

void BuiltToScaleScene::handleInput() {
    auto& input = Input::get();
    bool bouncePressed = input.current.keyStates[GLFW_KEY_L] == GLFW_PRESS &&
        input.last.keyStates[GLFW_KEY_L] == GLFW_RELEASE;
    bool throwPressed = input.current.keyStates[GLFW_KEY_K] == GLFW_PRESS &&
        input.last.keyStates[GLFW_KEY_K] == GLFW_RELEASE;

    // Handle bounce input ('L' key)
    if (bouncePressed && rodState.isActive && !rodState.isThrowing) {
        if (rodState.currentBlock == 2) { // Red block
            // Trigger bounce animation
            lastBlockBounce = currentTime;
        }
        else {
            // Failed to hit at the right time
            rodState.isActive = false;
            currentState = GameState::Failed;
            feedbackCube.color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    // Handle throw input ('K' key)
    if (throwPressed && rodState.currentBlock == 2 && redBlockPulledBack) {
        for (auto& pair : squarePairs) {
            if (pair.readyForCollision && !pair.isColliding) {
                rodState.isThrowing = true;
                rodState.throwProgress = 0.0f;
                rodState.throwStartPos = rod.localPosition;
                // Calculate midpoint between squares for target
                rodState.throwTargetPos = (pair.left.localPosition + pair.right.localPosition) * 0.5f;

                // Spring animation for red block
                redBlockPulledBack = false;
                springBlocks[2].localPosition.z = -3.0f;
                break;
            }
        }
    }
}

void BuiltToScaleScene::checkCollisions() {
    if (!rodState.isThrowing) return;

    for (auto& pair : squarePairs) {
        if (pair.readyForCollision && !pair.isColliding && !pair.isDying) {
            // Check if rod is close enough to squares
            vec3 squarePos = (pair.left.localPosition + pair.right.localPosition) * 0.5f;
            float dist = glm::distance(rod.localPosition, squarePos);

            if (dist < 0.5f) {
                pair.isColliding = true;
                pair.isDying = true;
                pair.deathProgress = 0.0f;
                score += 100;
                feedbackCube.color = vec4(0.0f, 1.0f, 0.0f, 1.0f);

                // Reset rod for next sequence
                resetRod(rand() % 2 == 0);
            }
        }
    }
}

void BuiltToScaleScene::cleanupDeadSquares() {
    // Remove squares that have completed their death animation or moved too far
    squarePairs.erase(
        std::remove_if(squarePairs.begin(), squarePairs.end(),
            [](const SquarePair& pair) {
                return (pair.isDying && pair.deathProgress >= 1.0f) ||
                    (!pair.isDying && pair.progress >= 1.5f);
            }
        ),
        squarePairs.end()
    );
}

void BuiltToScaleScene::spawnNewSquarePair() {
    SquarePair newPair;
    newPair.left = AnimationObject(AnimationObjectType::box);
    newPair.right = AnimationObject(AnimationObjectType::box);

    // Initialize properties for both squares
    for (auto* square : { &newPair.left, &newPair.right }) {
        square->localScale = vec3(0.5f);
        square->color = vec4(0.2f, 0.6f, 1.0f, 1.0f);
        square->updateMatrix(true);
    }

    newPair.progress = 0.0f;
    newPair.readyForCollision = false;
    newPair.isColliding = false;
    newPair.isDying = false;
    newPair.deathProgress = 0.0f;

    squarePairs.push_back(newPair);
}

void BuiltToScaleScene::resetRod(bool startFromRight) {
    rodState.currentBlock = startFromRight ? 3 : 0;
    rodState.direction = startFromRight ? -1 : 1;
    rodState.arcProgress = 0.0f;
    rodState.isActive = true;
    rodState.isThrowing = false;
    rodState.throwProgress = 0.0f;

    // Set initial position
    rod.localPosition = springBlocks[rodState.currentBlock].localPosition + vec3(0.0f, 0.5f, 0.0f);
    rod.localRotation = vec3(0.0f);
}

void BuiltToScaleScene::render(const mat4& projection, const mat4& view, bool isShadow) {
    auto& jr = AnimationObjectRenderer::get();

    // Render spring blocks
    for (const auto& block : springBlocks) {
        jr.beginBatchRender(block.shapeType, false, vec4(1.f), isShadow);
        jr.renderBatchWithOwnColor(block, isShadow);
        jr.endBatchRender(isShadow);
    }

    // Render rod
    jr.beginBatchRender(rod.shapeType, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(rod, isShadow);
    jr.endBatchRender(isShadow);

    // Render ground
    jr.beginBatchRender(ground.shapeType, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(ground, isShadow);
    jr.endBatchRender(isShadow);

    // Render visual indicators (not in shadow pass)
    if (!isShadow) {
        jr.beginBatchRender(debugTimingBar.shapeType, false, vec4(1.f), false);
        jr.renderBatchWithOwnColor(debugTimingBar, false);
        jr.endBatchRender(false);

        jr.beginBatchRender(feedbackCube.shapeType, false, vec4(1.f), false);
        jr.renderBatchWithOwnColor(feedbackCube, false);
        jr.endBatchRender(false);
    }

    // Render square pairs
    for (const auto& pair : squarePairs) {
        jr.beginBatchRender(pair.left.shapeType, false, vec4(1.f), isShadow);
        jr.renderBatchWithOwnColor(pair.left, isShadow);
        jr.endBatchRender(isShadow);

        jr.beginBatchRender(pair.right.shapeType, false, vec4(1.f), isShadow);
        jr.renderBatchWithOwnColor(pair.right, isShadow);
        jr.endBatchRender(isShadow);
    }
}

void BuiltToScaleScene::renderUI() {
    if (ImGui::CollapsingHeader("Built to Scale")) {
        ImGui::Text("Score: %d", score);

        const char* stateStr = "Unknown";
        switch (currentState) {
        case GameState::Ready: stateStr = "Get Ready!"; break;
        case GameState::Playing: stateStr = "Playing!"; break;
        case GameState::Failed: stateStr = "Failed - Resetting..."; break;
        }
        ImGui::Text("State: %s", stateStr);

        if (ImGui::TreeNode("Debug Info")) {
            ImGui::Text("Current Block: %d", rodState.currentBlock);
            ImGui::Text("Rod Direction: %d", rodState.direction);
            ImGui::Text("Active Square Pairs: %zu", squarePairs.size());
            ImGui::TreePop();
        }

        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::Text("L - Bounce Rod on Red Block");
        ImGui::Text("K - Throw Rod at Overlapping Squares");
    }
}

ptr_vector<AnimationObject> BuiltToScaleScene::getObjects() {
    ptr_vector<AnimationObject> objects;

    for (auto& block : springBlocks) objects.push_back(&block);
    objects.push_back(&rod);
    objects.push_back(&ground);
    objects.push_back(&debugTimingBar);
    objects.push_back(&feedbackCube);

    for (auto& pair : squarePairs) {
        objects.push_back(&pair.left);
        objects.push_back(&pair.right);
    }

    return objects;
}