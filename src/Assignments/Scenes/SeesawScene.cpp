#include "SeesawScene.h"
#include "AnimationObjectRenderer.h"
#include "imgui.h"
#include "Input.h"
#include <iostream>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/fast_exponential.hpp>

SeesawScene::SeesawScene()
    : currentState(GameState::StartSequence)
    , currentTime(0)
    , sequenceStartTime(0)
    , audio(AudioSystem::get()) {
}

SeesawScene::~SeesawScene() {}

void SeesawScene::init() {
    // Initialize seesaw
    seesaw = AnimationObject(AnimationObjectType::box);
    seesaw.localScale = vec3(seesawLength, 0.2f, 1.0f);
    seesaw.color = vec4(0.8f, 0.6f, 0.4f, 1.0f);

    // Initialize characters
    playerCharacter = AnimationObject(AnimationObjectType::box);
    playerCharacter.localScale = vec3(0.8f);
    playerCharacter.color = vec4(0.2f, 0.6f, 1.0f, 1.0f);

    npcCharacter = AnimationObject(AnimationObjectType::box);
    npcCharacter.localScale = vec3(0.8f);
    npcCharacter.color = vec4(1.0f, 0.4f, 0.4f, 1.0f);

    // Initialize feedback cube
    feedbackCube = AnimationObject(AnimationObjectType::box);
    feedbackCube.localScale = vec3(0.5f);
    feedbackCube.color = vec4(0.5f);  // Gray
    feedbackCube.localPosition = seesawPivotPoint + vec3(0.0f, 1.0f, 0.0f);

    // Initialize player indicator
    playerIndicator = AnimationObject(AnimationObjectType::box);
    playerIndicator.localScale = vec3(0.4f);
    playerIndicator.color = vec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow

    initializePositions();

    // Update all matrices
    seesaw.updateMatrix(true);
    playerCharacter.updateMatrix(true);
    npcCharacter.updateMatrix(true);
    feedbackCube.updateMatrix(true);
    playerIndicator.updateMatrix(true);
}

void SeesawScene::handlePlayerInput() {
    auto& input = Input::get();
    bool isLPressed = input.current.keyStates[GLFW_KEY_L] == GLFW_PRESS;

    if (isLPressed && !lastInputState && !hasInputThisBeat) {
        float phase = (currentTime - sequenceStartTime) / BEAT_DURATION;

        // Allow input during end of PlayerJumping and start of NPCJumping
        if (currentState == GameState::PlayerJumping ||
            (currentState == GameState::NPCJumping && phase < 0.2f)) {

            lastTimingResult = checkTiming(phase);
            lastFeedbackTime = currentTime;
            hasInputThisBeat = true;

            // Update feedback cube color
            feedbackCube.color = getTimingResultColor(lastTimingResult);

            // Visual feedback based on timing
            if (lastTimingResult == TimingResult::Bad || lastTimingResult == TimingResult::Miss) {
                badTimingAnimationTime = BAD_ANIMATION_DURATION;
            }
            else if (lastTimingResult == TimingResult::Perfect) {
                // Add some "success" animation
                playerCharacter.color = vec4(1.0f, 0.84f, 0.0f, 1.0f);  // Temporarily gold
            }
            else if (lastTimingResult == TimingResult::Good) {
                playerCharacter.color = vec4(0.0f, 1.0f, 0.0f, 1.0f);  // Temporarily green
            }
        }
    }

    lastInputState = isLPressed;
}

SeesawScene::TimingResult SeesawScene::checkTiming(float phase) {
    // Perfect timing should be at landing (phase 0.7 is when the character starts falling, 
    // and they land at phase 1.0)
    float timingDiff;

    if (currentState == GameState::PlayerJumping) {
        // For player jumping, check timing relative to landing (phase 1.0)
        timingDiff = abs(1.0f - phase);
    }
    else if (currentState == GameState::NPCJumping) {
        // If we're in early part of NPC jumping, consider it relative to previous landing
        if (phase < 0.2f) {
            timingDiff = abs(0.0f - phase);  // How far we are from the start of this phase
        }
        else {
            return TimingResult::Miss;  // Too late if we're well into NPC jumping
        }
    }
    else {
        return TimingResult::Miss;
    }

    if (timingDiff <= PERFECT_WINDOW) return TimingResult::Perfect;
    if (timingDiff <= GOOD_WINDOW) return TimingResult::Good;
    if (timingDiff <= BAD_WINDOW) return TimingResult::Bad;
    return TimingResult::Miss;
}

vec4 SeesawScene::getTimingResultColor(TimingResult result) {
    switch (result) {
    case TimingResult::Perfect: return vec4(1.0f, 0.84f, 0.0f, 1.0f);  // Gold
    case TimingResult::Good: return vec4(0.0f, 1.0f, 0.0f, 1.0f);      // Green
    case TimingResult::Bad: return vec4(1.0f, 0.0f, 0.0f, 1.0f);       // Red
    default: return vec4(0.5f, 0.5f, 0.5f, 1.0f);                      // Gray
    }
}

void SeesawScene::updateBadTimingAnimation(float dt) {
    if (badTimingAnimationTime > 0) {
        badTimingAnimationTime -= dt;

        if (lastTimingResult == TimingResult::Bad || lastTimingResult == TimingResult::Miss) {
            // More obvious shake animation
            float shake = sin(badTimingAnimationTime * 30.0f) * 0.3f;
            playerCharacter.localPosition.x += shake;
            playerCharacter.localRotation.z = shake * 45.0f;  // Add rotation shake
            playerCharacter.color = vec4(1.0f, 0.0f, 0.0f, 1.0f);  // Red during shake
        }
    }
    else if (lastTimingResult != TimingResult::Perfect && lastTimingResult != TimingResult::Good) {
        // Reset character appearance
        playerCharacter.color = vec4(0.2f, 0.6f, 1.0f, 1.0f);
        playerCharacter.localRotation.z = 0.0f;
    }
}

vec3 SeesawScene::calculateSeesawEndPoint(float side, float tiltAngle) {
    float adjustedLength = (seesawLength * 0.5f) - characterOffset;
    return seesawPivotPoint +
        vec3(cos(glm::radians(tiltAngle)) * adjustedLength * side,
            sin(glm::radians(tiltAngle)) * adjustedLength * side + characterBaseHeight,
            0.0f);
}

void SeesawScene::initializePositions() {
    seesawPivotPoint = vec3(0.0f);
    currentTiltAngle = -seesawTiltAngle;

    seesaw.localPosition = seesawPivotPoint;
    seesaw.localRotation = vec3(0.0f, 0.0f, currentTiltAngle);

    playerCharacter.localPosition = calculateSeesawEndPoint(1.0f, currentTiltAngle);
    npcCharacter.localPosition = vec3(-seesawLength * 0.5f - 2.0f, 2.0f, 0.0f);

    // Position player indicator below right side
    playerIndicator.localPosition = calculateSeesawEndPoint(1.0f, currentTiltAngle) +
        vec3(0.0f, -1.0f, 0.0f);
}

void SeesawScene::update(double now, float dt) {
    currentTime += dt;

    handlePlayerInput();
    updateBadTimingAnimation(dt);
    updateFeedbackCube(dt);

    switch (currentState) {
    case GameState::StartSequence: {
        float phase = (currentTime - sequenceStartTime) / BEAT_DURATION;
        if (phase >= 1.0f) {
            currentState = GameState::NPCJumping;
            sequenceStartTime = currentTime;
        }
        else {
            // Parabolic jump to starting position
            vec3 startPos = npcCharacter.localPosition;
            vec3 endPos = calculateSeesawEndPoint(-1.0f, currentTiltAngle);
            vec3 midPoint = (startPos + endPos) * 0.5f + vec3(0.0f, 2.0f, 0.0f);

            float t = phase;
            if (t < 0.5f) {
                t = t * 2.0f;
                npcCharacter.localPosition = glm::mix(startPos, midPoint, t);
            }
            else {
                t = (t - 0.5f) * 2.0f;
                npcCharacter.localPosition = glm::mix(midPoint, endPos, t);
            }
        }
        break;
    }

    case GameState::PlayerJumping:
    case GameState::NPCJumping: {
        float phase = (currentTime - sequenceStartTime) / BEAT_DURATION;
        if (phase >= 1.0f) {
            currentState = (currentState == GameState::PlayerJumping) ?
                GameState::NPCJumping : GameState::PlayerJumping;
            sequenceStartTime = currentTime;

            // Only reset timing stuff at start of player's turn
            if (currentState == GameState::PlayerJumping) {
                hasInputThisBeat = false;
                playerCharacter.color = vec4(0.2f, 0.6f, 1.0f, 1.0f);  // Reset color
            }
        }

        updateSeesawTilt(dt);
        updateCharacterPositions(dt);
        break;
    }
    }

    // Update matrices
    seesaw.updateMatrix(true);
    playerCharacter.updateMatrix(true);
    npcCharacter.updateMatrix(true);
    feedbackCube.updateMatrix(true);
    playerIndicator.updateMatrix(true);
}

void SeesawScene::updateSeesawTilt(float dt) {
    float targetAngle = (currentState == GameState::PlayerJumping) ?
        seesawTiltAngle : -seesawTiltAngle;

    float angleDiff = targetAngle - currentTiltAngle;
    float maxChange = tiltSpeed * dt;

    if (abs(angleDiff) > maxChange) {
        currentTiltAngle += maxChange * (angleDiff > 0 ? 1.0f : -1.0f);
    }
    else {
        currentTiltAngle = targetAngle;
    }

    seesaw.localRotation.z = currentTiltAngle;
}

void SeesawScene::updateCharacterPositions(float dt) {
    float phase = (currentTime - sequenceStartTime) / BEAT_DURATION;

    AnimationObject* jumpingChar = (currentState == GameState::PlayerJumping) ?
        &playerCharacter : &npcCharacter;
    float jumpSide = (jumpingChar == &playerCharacter) ? 1.0f : -1.0f;

    vec3 startPos = calculateSeesawEndPoint(jumpSide, currentTiltAngle);
    vec3 peakPos = startPos + vec3(0.0f, jumpHeight, 0.0f);
    jumpingChar->localPosition = calculateJumpPosition(phase, startPos, peakPos);

    AnimationObject* groundedChar = (currentState == GameState::PlayerJumping) ?
        &npcCharacter : &playerCharacter;
    float groundedSide = (groundedChar == &playerCharacter) ? 1.0f : -1.0f;
    groundedChar->localPosition = calculateSeesawEndPoint(groundedSide, currentTiltAngle);
}

vec3 SeesawScene::calculateJumpPosition(float t, const vec3& startPos, const vec3& peakPos) {
    // Modified jump curve to spend more time at peak
    float modifiedT;
    if (t < 0.3f) {
        // Going up (30% of time)
        modifiedT = t / 0.3f * 0.5f;
    }
    else if (t < 0.7f) {
        // At peak (40% of time)
        modifiedT = 0.5f;
    }
    else {
        // Going down (30% of time)
        modifiedT = 0.5f + (t - 0.7f) / 0.3f * 0.5f;
    }

    return glm::mix(startPos, peakPos, sin(modifiedT * M_PI));
}

void SeesawScene::updateFeedbackCube(float dt) {
    float phase = (currentTime - sequenceStartTime) / BEAT_DURATION;

    // Base pulsing effect
    beatPulseScale = 1.0f + 0.2f * sin(phase * 2.0f * M_PI);
    feedbackCube.localScale = vec3(0.5f * beatPulseScale);

    // Only fade the feedback if enough time has passed
    if (currentTime - lastFeedbackTime > FEEDBACK_DURATION) {
        feedbackCube.color = vec4(0.5f);  // Reset to gray
    }
}

void SeesawScene::render(const mat4& projection, const mat4& view, bool isShadow) {
    auto& jr = AnimationObjectRenderer::get();

    // Render seesaw
    jr.beginBatchRender(seesaw.shapeType, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(seesaw, isShadow);
    jr.endBatchRender(isShadow);

    // Render characters and indicators
    jr.beginBatchRender(playerCharacter.shapeType, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(playerCharacter, isShadow);
    jr.renderBatchWithOwnColor(npcCharacter, isShadow);
    jr.renderBatchWithOwnColor(feedbackCube, isShadow);
    jr.renderBatchWithOwnColor(playerIndicator, isShadow);
    jr.endBatchRender(isShadow);
}

void SeesawScene::renderUI() {
    if (ImGui::CollapsingHeader("Seesaw Settings")) {
        ImGui::SliderFloat("Jump Height", &jumpHeight, 1.0f, 8.0f);
        ImGui::SliderFloat("Seesaw Tilt", &seesawTiltAngle, 5.0f, 30.0f);
        ImGui::SliderFloat("Character Offset", &characterOffset, 0.0f, 2.0f);
        ImGui::SliderFloat("Base Height", &characterBaseHeight, 0.0f, 1.0f);

        const char* stateStr = "Unknown";
        switch (currentState) {
        case GameState::StartSequence: stateStr = "Start Sequence"; break;
        case GameState::PlayerJumping: stateStr = "Player Jumping"; break;
        case GameState::NPCJumping: stateStr = "NPC Jumping"; break;
        }
        ImGui::Text("Current State: %s", stateStr);
        ImGui::Text("Beat Progress: %.2f", (currentTime - sequenceStartTime) / BEAT_DURATION);

        const char* timingStr = "None";
        switch (lastTimingResult) {
        case TimingResult::Perfect: timingStr = "PERFECT!"; break;
        case TimingResult::Good: timingStr = "Good!"; break;
        case TimingResult::Bad: timingStr = "Bad"; break;
        case TimingResult::Miss: timingStr = "Miss"; break;
        case TimingResult::None: timingStr = "None"; break;
        }
        ImGui::Text("Last Timing: %s", timingStr);
    }
}

ptr_vector<AnimationObject> SeesawScene::getObjects() {
    return { &seesaw, &playerCharacter, &npcCharacter, &feedbackCube, &playerIndicator };
}