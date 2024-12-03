// BuiltToScaleScene.cpp

#include "BuiltToScaleScene.h"
#include "AnimationObjectRenderer.h"
#include "Input.h"
#include "imgui.h"
#include <algorithm>

BuiltToScaleScene::BuiltToScaleScene()
    : currentState(GameState::Playing),
    currentTime(0.0),
    beatInterval(0.5f),
    timingTolerance(0.2f),
    lastInputTime(0.0f),
    redBlockIndex(2), // The user block is the third block (index 2)
    beatCounter(0),
    redBlockPullProgress(0.0f)
{
}

BuiltToScaleScene::~BuiltToScaleScene() {
    // Cleanup if necessary
}

void BuiltToScaleScene::init() {
    // Initialize blocks
    blocks.clear();
    blockAnimations.clear();
    for (int i = 0; i < NUM_BLOCKS; ++i) {
        AnimationObject block(AnimationObjectType::box);
        block.localPosition = vec3(-BLOCK_SPACING * (NUM_BLOCKS - 1) / 2 + i * BLOCK_SPACING, 0.0f, -3.0f);
        block.localScale = vec3(0.8f);
        block.color = (i == redBlockIndex) ? vec4(1.0f, 0.2f, 0.2f, 1.0f) : vec4(0.7f);
        block.updateMatrix(true);
        blocks.push_back(block);

        blockAnimations.push_back({ false, 0.0f });
    }

    // Initialize rod
    rod.object = AnimationObject(AnimationObjectType::box);
    rod.object.localScale = vec3(1.0f, 0.2f, 0.2f);
    rod.object.color = vec4(0.8f, 0.8f, 0.0f, 1.0f);
    rod.isActive = false;
    rod.isFalling = false;
    rod.isThrown = false;
    rod.waitingForInput = false;
    rod.waitingForThrow = false;
    rod.progress = 0.0f;
    rod.currentBlockIndex = 0;
    rod.direction = 1;
    resetRod(true); // Start from the left side (block index 0)

    // Initialize squares
    squarePairs.clear();
    spawnSquarePair();

    // Initialize timing
    currentTime = 0.0;
    lastInputTime = 0.0f;
    beatCounter = 0;
    redBlockPullProgress = 0.0f;
}

void BuiltToScaleScene::update(double now, float dt) {
    currentTime += dt;

    handleInput();
    updateRod(now, dt);
    updateBlocks(now, dt);
    updateSquares(now, dt);

    // Remove offscreen or hit squares
    removeOffscreenSquares();

    // Spawn new rod if necessary
    if (!rod.isActive && !rod.isThrown) {
        resetRod(rod.direction == 1 ? true : false);
    }

    // Spawn new square pair if necessary
    if (squarePairs.empty()) {
        spawnSquarePair();
        beatCounter = 0;
    }
}

void BuiltToScaleScene::handleInput() {
    auto& input = Input::get();
    bool lPressed = input.current.keyStates[GLFW_KEY_L] == GLFW_PRESS &&
        input.last.keyStates[GLFW_KEY_L] == GLFW_RELEASE;
    bool kPressed = input.current.keyStates[GLFW_KEY_K] == GLFW_PRESS &&
        input.last.keyStates[GLFW_KEY_K] == GLFW_RELEASE;

    if (rod.isActive && !rod.isFalling && !rod.isThrown) {
        if (rod.currentBlockIndex == redBlockIndex && rod.waitingForInput) {
            if (lPressed) {
                // Correct timing
                float timeSinceExpected = currentTime - lastInputTime;
                if (abs(timeSinceExpected) <= timingTolerance) {
                    // Bounce the rod
                    rod.waitingForInput = false;
                    rod.progress = 0.0f;
                    lastInputTime = currentTime;
                    // Play block bounce animation
                    playBlockBounceAnimation(redBlockIndex);
                }
                else {
                    // Missed timing
                    rod.isFalling = true;
                    playRodFallAnimation();
                }
            }
            else {
                // Waiting for input
                if (currentTime - lastInputTime > timingTolerance * 2) {
                    // Missed timing
                    rod.isFalling = true;
                    playRodFallAnimation();
                }
            }
        }

        if (rod.waitingForThrow && redBlockPullProgress >= 1.0f) {
            if (kPressed) {
                // Correct timing
                float timeSinceExpected = currentTime - lastInputTime;
                if (abs(timeSinceExpected) <= timingTolerance) {
                    // Throw the rod
                    startRodThrow();
                    // Reset red block position
                    redBlockPullProgress = 0.0f;
                    blocks[redBlockIndex].localPosition.z = -3.0f;
                    blocks[redBlockIndex].updateMatrix(true);
                }
                else {
                    // Missed timing
                    rod.isFalling = true;
                    playRodFallAnimation();
                    // Reset red block position
                    redBlockPullProgress = 0.0f;
                    blocks[redBlockIndex].localPosition.z = -3.0f;
                    blocks[redBlockIndex].updateMatrix(true);
                }
            }
            else {
                // Waiting for input
                if (currentTime - lastInputTime > timingTolerance * 2) {
                    // Missed timing
                    rod.isFalling = true;
                    playRodFallAnimation();
                    // Reset red block position
                    redBlockPullProgress = 0.0f;
                    blocks[redBlockIndex].localPosition.z = -3.0f;
                    blocks[redBlockIndex].updateMatrix(true);
                }
            }
        }
    }
}

void BuiltToScaleScene::updateRod(double now, float dt) {
    if (rod.isActive) {
        if (rod.isFalling) {
            // Rod is falling down
            rod.object.localPosition.y -= ROD_FALL_SPEED * dt;
            rod.object.updateMatrix(true);
            if (rod.object.localPosition.y < -5.0f) {
                // Rod has fallen off screen
                rod.isActive = false;
            }
        }
        else if (rod.isThrown) {
            // Rod is moving towards the squares
            rod.progress += dt * (ROD_THROW_SPEED / length(rod.targetPosition - rod.startPosition));
            if (rod.progress >= 1.0f) {
                rod.progress = 1.0f;
            }
            rod.object.localPosition = mix(rod.startPosition, rod.targetPosition, rod.progress);
            rod.object.updateMatrix(true);

            if (rod.progress >= 1.0f) {
                // Reached the squares
                rod.isThrown = false;
                rod.isActive = false;
                checkCollisionWithSquares();
            }
        }
        else {
            // Rod is moving between blocks
            rod.progress += dt * (ROD_SPEED / length(blocks[rod.currentBlockIndex].localPosition - blocks[rod.currentBlockIndex - rod.direction].localPosition));
            if (rod.progress >= 1.0f) {
                // Reached the next block
                rod.progress = 0.0f;
                rod.currentBlockIndex += rod.direction;
                beatCounter++; // Increment beat counter
                lastInputTime = currentTime;

                // Check if we need to change direction
                if (rod.currentBlockIndex >= NUM_BLOCKS || rod.currentBlockIndex < 0) {
                    rod.direction *= -1;
                    rod.currentBlockIndex += rod.direction * 2;
                }

                // Play block bounce animation
                playBlockBounceAnimation(rod.currentBlockIndex);

                // If on the red block, set waitingForInput to true to wait for 'L' key
                if (rod.currentBlockIndex == redBlockIndex) {
                    rod.waitingForInput = true;
                }
            }

            // Update rod position
            int prevBlockIndex = rod.currentBlockIndex - rod.direction;
            if (prevBlockIndex >= 0 && prevBlockIndex < NUM_BLOCKS) {
                vec3 start = blocks[prevBlockIndex].localPosition + vec3(0.0f, 0.5f, 0.0f);
                vec3 end = blocks[rod.currentBlockIndex].localPosition + vec3(0.0f, 0.5f, 0.0f);
                rod.object.localPosition = mix(start, end, rod.progress);
                // Apply arc movement
                float height = sin(rod.progress * M_PI) * BLOCK_BOUNCE_HEIGHT;
                rod.object.localPosition.y += height;
                rod.object.updateMatrix(true);
            }
        }
    }
}

void BuiltToScaleScene::updateBlocks(double now, float dt) {
    for (int i = 0; i < NUM_BLOCKS; ++i) {
        if (blockAnimations[i].isBouncing) {
            blockAnimations[i].progress += dt * 4.0f;
            if (blockAnimations[i].progress >= 1.0f) {
                blockAnimations[i].isBouncing = false;
                blockAnimations[i].progress = 0.0f;
                blocks[i].localPosition.y = 0.0f;
            }
            else {
                float bounce = sin(blockAnimations[i].progress * M_PI) * BLOCK_BOUNCE_HEIGHT;
                blocks[i].localPosition.y = bounce;
            }
            blocks[i].updateMatrix(true);
        }
    }

    // Handle red block pullback when squares are about to overlap
    if (rod.currentBlockIndex == redBlockIndex && !rod.isActive && !rod.isThrown) {
        // Pull back the red block automatically
        redBlockPullProgress += dt * 2.0f;
        if (redBlockPullProgress >= 1.0f) {
            redBlockPullProgress = 1.0f;
            rod.waitingForThrow = true;
        }
        float pullback = sin(redBlockPullProgress * M_PI) * RED_BLOCK_PULLBACK_DISTANCE;
        blocks[redBlockIndex].localPosition.z = -3.0f - pullback;
        blocks[redBlockIndex].updateMatrix(true);
    }
}

void BuiltToScaleScene::updateSquares(double now, float dt) {
    for (auto& pair : squarePairs) {
        if (!pair.isHit) {
            // Move squares towards each other based on beatCounter
            int totalBeatsToOverlap = 4; // Adjust as needed
            float progress = static_cast<float>(beatCounter) / totalBeatsToOverlap;
            if (progress > 1.0f) progress = 1.0f;

            pair.leftSquare.localPosition.x = -SQUARE_MOVE_DISTANCE + (SQUARE_MOVE_DISTANCE * progress);
            pair.rightSquare.localPosition.x = SQUARE_MOVE_DISTANCE - (SQUARE_MOVE_DISTANCE * progress);

            // Check if squares are overlapping
            if (progress >= 1.0f && !pair.isOverlapping) {
                pair.isOverlapping = true;
                // Pull back the red block automatically
                redBlockPullProgress = 0.0f;
            }
        }
        else if (pair.isHit) {
            // Play hit animation
            pair.animationProgress += dt * 2.0f;
            if (pair.animationProgress >= 1.0f) {
                pair.isHit = false;
                // Mark for deletion
                pair.leftSquare.localPosition.y = -100.0f;
                pair.rightSquare.localPosition.y = -100.0f;
            }
            else {
                // Move squares backward and downward
                float t = pair.animationProgress;
                pair.leftSquare.localPosition.x -= t * 2.0f;
                pair.leftSquare.localPosition.y -= t * 5.0f;
                pair.rightSquare.localPosition.x += t * 2.0f;
                pair.rightSquare.localPosition.y -= t * 5.0f;
            }
        }

        pair.leftSquare.updateMatrix(true);
        pair.rightSquare.updateMatrix(true);
    }
}

void BuiltToScaleScene::render(const mat4& projection, const mat4& view, bool isShadow) {
    auto& renderer = AnimationObjectRenderer::get();

    // Render blocks
    for (auto& block : blocks) {
        renderer.beginBatchRender(block.shapeType, false, vec4(1.0f), isShadow);
        renderer.renderBatchWithOwnColor(block, isShadow);
        renderer.endBatchRender(isShadow);
    }

    // Render rod
    if (rod.isActive || rod.isThrown) {
        renderer.beginBatchRender(rod.object.shapeType, false, vec4(1.0f), isShadow);
        renderer.renderBatchWithOwnColor(rod.object, isShadow);
        renderer.endBatchRender(isShadow);
    }

    // Render squares
    for (auto& pair : squarePairs) {
        renderer.beginBatchRender(pair.leftSquare.shapeType, false, vec4(1.0f), isShadow);
        renderer.renderBatchWithOwnColor(pair.leftSquare, isShadow);
        renderer.endBatchRender(isShadow);

        renderer.beginBatchRender(pair.rightSquare.shapeType, false, vec4(1.0f), isShadow);
        renderer.renderBatchWithOwnColor(pair.rightSquare, isShadow);
        renderer.endBatchRender(isShadow);
    }
}

void BuiltToScaleScene::renderUI() {
    if (ImGui::CollapsingHeader("Built To Scale")) {
        ImGui::Text("Press 'L' to bounce the rod on the red block.");
        ImGui::Text("Press 'K' to push the red block and throw the rod.");
        ImGui::Text("Current Beat: %d", beatCounter);
    }
}

ptr_vector<AnimationObject> BuiltToScaleScene::getObjects() {
    ptr_vector<AnimationObject> objects;
    for (auto& block : blocks) {
        objects.push_back(&block);
    }
    if (rod.isActive || rod.isThrown) {
        objects.push_back(&rod.object);
    }
    for (auto& pair : squarePairs) {
        objects.push_back(&pair.leftSquare);
        objects.push_back(&pair.rightSquare);
    }
    return objects;
}

void BuiltToScaleScene::resetRod(bool startFromRight) {
    rod.isActive = true;
    rod.isFalling = false;
    rod.isThrown = false;
    rod.waitingForInput = false;
    rod.waitingForThrow = false;
    rod.progress = 0.0f;
    rod.direction = startFromRight ? 1 : -1;
    rod.currentBlockIndex = startFromRight ? 0 : NUM_BLOCKS - 1;
    rod.object.localPosition = blocks[rod.currentBlockIndex].localPosition + vec3(0.0f, 0.5f, 0.0f);
    rod.object.updateMatrix(true);
    lastInputTime = currentTime;
}

void BuiltToScaleScene::spawnSquarePair() {
    SquarePair pair;
    pair.leftSquare = AnimationObject(AnimationObjectType::box);
    pair.rightSquare = AnimationObject(AnimationObjectType::box);
    pair.leftSquare.localPosition = vec3(-SQUARE_MOVE_DISTANCE, 0.0f, 0.0f);
    pair.rightSquare.localPosition = vec3(SQUARE_MOVE_DISTANCE, 0.0f, 0.0f);
    pair.leftSquare.localScale = vec3(1.0f);
    pair.rightSquare.localScale = vec3(1.0f);
    pair.leftSquare.color = vec4(0.5f, 0.5f, 1.0f, 1.0f);
    pair.rightSquare.color = vec4(0.5f, 0.5f, 1.0f, 1.0f);
    pair.isOverlapping = false;
    pair.isHit = false;
    pair.animationProgress = 0.0f;
    pair.leftSquare.updateMatrix(true);
    pair.rightSquare.updateMatrix(true);
    squarePairs.push_back(pair);
}

void BuiltToScaleScene::checkCollisionWithSquares() {
    for (auto& pair : squarePairs) {
        if (pair.isOverlapping && !pair.isHit) {
            // Check if rod is at the squares' position
            if (abs(rod.object.localPosition.x - SQUARE_OVERLAP_POSITION) <= 1.0f) {
                // Collision detected
                pair.isHit = true;
                rod.isActive = false;
                rod.isThrown = false;
                playSquareHitAnimation(pair);
                break;
            }
        }
    }
}

void BuiltToScaleScene::removeOffscreenSquares() {
    squarePairs.erase(std::remove_if(squarePairs.begin(), squarePairs.end(),
        [](const SquarePair& pair) {
            return (pair.leftSquare.localPosition.y < -50.0f && pair.rightSquare.localPosition.y < -50.0f);
        }), squarePairs.end());
}

void BuiltToScaleScene::playBlockBounceAnimation(int blockIndex) {
    blockAnimations[blockIndex].isBouncing = true;
    blockAnimations[blockIndex].progress = 0.0f;
}

void BuiltToScaleScene::playRodFallAnimation() {
    // Implement any additional effects if needed
}

void BuiltToScaleScene::playSquareHitAnimation(SquarePair& pair) {
    pair.animationProgress = 0.0f;
}

void BuiltToScaleScene::startRodThrow() {
    rod.isThrown = true;
    rod.isActive = true;
    rod.progress = 0.0f;
    rod.startPosition = rod.object.localPosition;
    rod.targetPosition = vec3(SQUARE_OVERLAP_POSITION, rod.object.localPosition.y, 0.0f);
    rod.waitingForThrow = false;
}
