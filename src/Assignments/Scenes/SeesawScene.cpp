#include "SeesawScene.h"
#include "AnimationObjectRenderer.h"
#include "imgui.h"
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
    seesaw.localScale = vec3(seesawLength, 0.2f, 1.0f);  // Thin, long platform
    seesaw.color = vec4(0.8f, 0.6f, 0.4f, 1.0f);  // Wooden color

    // Initialize characters
    playerCharacter = AnimationObject(AnimationObjectType::box);
    playerCharacter.localScale = vec3(0.8f);
    playerCharacter.color = vec4(0.2f, 0.6f, 1.0f, 1.0f);  // Blue

    npcCharacter = AnimationObject(AnimationObjectType::box);
    npcCharacter.localScale = vec3(0.8f);
    npcCharacter.color = vec4(1.0f, 0.4f, 0.4f, 1.0f);  // Red

    initializePositions();

    // Update all matrices
    seesaw.updateMatrix(true);
    playerCharacter.updateMatrix(true);
    npcCharacter.updateMatrix(true);
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
    currentTiltAngle = -seesawTiltAngle;  // Start tilted down on right

    // Initialize seesaw
    seesaw.localPosition = seesawPivotPoint;
    seesaw.localRotation = vec3(0.0f, 0.0f, currentTiltAngle);

    // Position player on right side of seesaw
    playerCharacter.localPosition = calculateSeesawEndPoint(1.0f, currentTiltAngle);

    // NPC starts to the left of seesaw
    float startDistance = seesawLength * 0.5f + 2.0f;
    npcCharacter.localPosition = vec3(-startDistance, 2.0f, 0.0f);  // Start higher for jump
}

void SeesawScene::update(double now, float dt) {
    currentTime += dt;

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
}

void SeesawScene::updateSeesawTilt(float dt) {
    float targetAngle = (currentState == GameState::PlayerJumping) ?
        seesawTiltAngle : -seesawTiltAngle;

    // Smoothly interpolate to target angle
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

    // Update jumping character
    AnimationObject* jumpingChar = (currentState == GameState::PlayerJumping) ?
        &playerCharacter : &npcCharacter;
    float jumpSide = (jumpingChar == &playerCharacter) ? 1.0f : -1.0f;

    vec3 startPos = calculateSeesawEndPoint(jumpSide, currentTiltAngle);
    vec3 peakPos = startPos + vec3(0.0f, jumpHeight, 0.0f);
    jumpingChar->localPosition = calculateJumpPosition(phase, startPos, peakPos);

    // Update grounded character
    AnimationObject* groundedChar = (currentState == GameState::PlayerJumping) ?
        &npcCharacter : &playerCharacter;
    float groundedSide = (groundedChar == &playerCharacter) ? 1.0f : -1.0f;
    groundedChar->localPosition = calculateSeesawEndPoint(groundedSide, currentTiltAngle);
}

vec3 SeesawScene::calculateJumpPosition(float t, const vec3& startPos, const vec3& peakPos) {
    // Parabolic jump
    if (t < 0.5f) {
        float u = t * 2.0f;
        return glm::mix(startPos, peakPos, u);
    }
    else {
        float u = (t - 0.5f) * 2.0f;
        return glm::mix(peakPos, startPos, u);
    }
}

void SeesawScene::render(const mat4& projection, const mat4& view, bool isShadow) {
    auto& jr = AnimationObjectRenderer::get();

    // Render seesaw
    jr.beginBatchRender(seesaw.shapeType, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(seesaw, isShadow);
    jr.endBatchRender(isShadow);

    // Render characters
    jr.beginBatchRender(playerCharacter.shapeType, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(playerCharacter, isShadow);
    jr.renderBatchWithOwnColor(npcCharacter, isShadow);
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
    }
}

ptr_vector<AnimationObject> SeesawScene::getObjects() {
    return { &seesaw, &playerCharacter, &npcCharacter };
}