#include "SeesawScene.h"
#include "AnimationObjectRenderer.h"
#include "imgui.h"
#include "Input.h"
#include <iostream>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/fast_exponential.hpp>
#include <random>
#include <Textures.h>

SeesawScene::SeesawScene()
    : currentState(GameState::StartSequence)
    , currentTime(0)
    , sequenceStartTime(0)
    , audio(AudioSystem::get()) {

    // Initialize character states
    playerState = {
        Position::FarSide,  // start position
        {Position::FarSide, Position::FarSide, false},  // initial pattern
        false,  // not jumping
        0.0f,   // jump start time
        false   // not high jump
    };

    npcState = {
        Position::FarSide,
        {Position::FarSide, Position::FarSide, false},
        false,
        0.0f,
        false
    };
}

SeesawScene::~SeesawScene() {}

void SeesawScene::init() {
    // ===================================================
    // Initialize player stand model
    playerStandModel = AnimationObject(AnimationObjectType::model);
    playerStandModel.meshName = "../assets/models/seesaw/SS_InspectorRight.obj";
    
    // Create and register texture
    auto playerTexture = std::make_shared<Texture>(
        "../assets/models/seesaw/SS_InspectorRightTexture.png");
    TextureRegistry::addTexture(playerTexture);
    playerStandModel.texture = reinterpret_cast<void*>(playerTexture->id);
    
    // Set up model properties
    playerStandModel.localScale = vec3(1.0f);
    playerStandModel.color = vec4(1.0f);
    playerStandModel.updateMatrix(true);

    // Initialize player jump model
    playerJumpModel = AnimationObject(AnimationObjectType::model);
    playerJumpModel.meshName = "../assets/models/seesaw/SS_InspectorRightJump.obj";
    
    // Can reuse the same texture
    playerJumpModel.texture = reinterpret_cast<void*>(playerTexture->id);
    
    // Set up model properties
    playerJumpModel.localScale = vec3(1.0f);
    playerJumpModel.color = vec4(1.0f);
    playerJumpModel.updateMatrix(true);

    // ====
        // Initialize player fall model
    playerFallModel = AnimationObject(AnimationObjectType::model);
    playerFallModel.meshName = "../assets/models/seesaw/SS_InspectorRightFall.obj";
    
    // Reuse the same texture
    playerFallModel.texture = reinterpret_cast<void*>(playerTexture->id);
    
    // Set up model properties
    playerFallModel.localScale = vec3(1.0f);
    playerFallModel.color = vec4(1.0f);
    playerFallModel.updateMatrix(true);

    // ===================================================

    // ===================================================
    // Initialize player stand model
    npcStandModel = AnimationObject(AnimationObjectType::model);
    npcStandModel.meshName = "../assets/models/seesaw/SS_InspectorLeft.obj";
    
    // Create and register texture
    auto npcTexture = std::make_shared<Texture>(
        "../assets/models/seesaw/SS_InspectorRightTexture.png"); // TODO: Change this
    TextureRegistry::addTexture(npcTexture);
    npcStandModel.texture = reinterpret_cast<void*>(npcTexture->id);
    
    // Set up model properties
    npcStandModel.localScale = vec3(1.0f);
    npcStandModel.color = vec4(1.0f);
    npcStandModel.updateMatrix(true);

    // Initialize player jump model
    npcJumpModel = AnimationObject(AnimationObjectType::model);
    npcJumpModel.meshName = "../assets/models/seesaw/SS_InspectorLeftJump.obj";
    
    // Can reuse the same texture
    npcJumpModel.texture = reinterpret_cast<void*>(npcTexture->id);
    
    // Set up model properties
    npcJumpModel.localScale = vec3(1.0f);
    npcJumpModel.color = vec4(1.0f);
    npcJumpModel.updateMatrix(true);

    // ====
        // Initialize player fall model
    npcFallModel = AnimationObject(AnimationObjectType::model);
    npcFallModel.meshName = "../assets/models/seesaw/SS_InspectorLeftFall.obj";
    
    // Reuse the same texture
    npcFallModel.texture = reinterpret_cast<void*>(npcTexture->id);
    
    // Set up model properties
    npcFallModel.localScale = vec3(1.0f);
    npcFallModel.color = vec4(1.0f);
    npcFallModel.updateMatrix(true);

    // ===================================================

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

    // Initialize audio
    audio.loadScene("seesaw");

    initializePositions();

    // Update all matrices
    seesaw.updateMatrix(true);
    playerCharacter.updateMatrix(true);
    npcCharacter.updateMatrix(true);
    feedbackCube.updateMatrix(true);
    playerIndicator.updateMatrix(true);
}

vec3 SeesawScene::getPositionForCharacter(bool isPlayer, Position pos) const {
    float side = isPlayer ? 1.0f : -1.0f;
    float offset = (pos == Position::FarSide) ? FAR_POSITION_OFFSET : CLOSE_POSITION_OFFSET;
    float adjustedLength = offset * side;

    // More subtle height difference based on position
    float positionBasedHeight = (pos == Position::FarSide) ?
        characterBaseHeight * 0.2 - 0.8f :    // Lower at far ends
        characterBaseHeight * 0.4f;     // Higher near center

    // Calculate position with tilt
    vec3 basePosition = seesawPivotPoint +
        vec3(cos(glm::radians(currentTiltAngle)) * adjustedLength,
            sin(glm::radians(currentTiltAngle)) * adjustedLength,
            0.0f);

    // Add the height offset after tilt calculation
    return basePosition + vec3(0.0f, positionBasedHeight, 0.0f);
}

float SeesawScene::getCurrentBPM(Position startPos) const {
    float bpm = BASE_BPM;
    if (startPos == Position::CloseSide) {
        bpm *= CLOSE_POSITION_BPM_MULTIPLIER;
    }
    return bpm;
}

float SeesawScene::getJumpDuration(Position startPos) const {
    return 60.0f / getCurrentBPM(startPos);
}

bool SeesawScene::shouldTriggerHighJump() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    return dis(gen) < HIGH_JUMP_PROBABILITY;
}

void SeesawScene::selectNewPattern(CharacterState& state) {
    std::vector<JumpPattern> availablePatterns;

    if (state.currentPos == Position::FarSide) {
        // From far side, can jump to same far side or to close side
        availablePatterns.push_back({ Position::FarSide, Position::FarSide, false });
        availablePatterns.push_back({ Position::FarSide, Position::CloseSide, false });
    }
    else {
        // From close side, can jump to same close side or to far side
        availablePatterns.push_back({ Position::CloseSide, Position::CloseSide, false });
        availablePatterns.push_back({ Position::CloseSide, Position::FarSide, false });
    }

    // Weighted random selection to favor pattern changes
    float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    if (r < 0.7f) { // 70% chance to change position
        for (const auto& pattern : availablePatterns) {
            if (pattern.endPos != state.currentPos) {
                state.currentPattern = pattern;
                break;
            }
        }
    }
    else {
        // 30% chance to stay at current position
        for (const auto& pattern : availablePatterns) {
            if (pattern.endPos == state.currentPos) {
                state.currentPattern = pattern;
                break;
            }
        }
    }

    // Chance for high jump
    state.currentPattern.isHighJump = shouldTriggerHighJump();
}

void SeesawScene::update(double now, float dt) {
    currentTime += dt;

    handlePlayerInput();
    updateBadTimingAnimation(dt);
    updateFeedbackCube(dt);

    switch (currentState) {
    case GameState::StartSequence: {
        float phase = (currentTime - sequenceStartTime) / BASE_BEAT_DURATION;
        if (phase >= 1.0f) {
            currentState = GameState::NPCJumping;
            sequenceStartTime = currentTime;
            // Initialize first NPC pattern
            selectNewPattern(npcState);
            npcState.isJumping = true;
            npcState.jumpStartTime = currentTime;

            // Play initial jump sound
            bool isQuickJump = (npcState.currentPattern.startPos == Position::CloseSide);
            audio.playSound(isQuickJump ? "seesaw_see_quick" : "seesaw_see_normal");

        }
        else {
            // Initial NPC jump to position
            vec3 startPos = npcCharacter.localPosition;
            vec3 endPos = getPositionForCharacter(false, Position::FarSide);
            vec3 midPoint = (startPos + endPos) * 0.5f + vec3(0.0f, 2.0f, 0.0f);

            float t = phase;
            if (t < 0.5f) {
                t = t * 2.0f;
                npcCharacter.localPosition = glm::mix(startPos, midPoint, t);

                npcStandModel.localPosition = glm::mix(startPos, midPoint, t);
                npcJumpModel.localPosition = glm::mix(startPos, midPoint, t);
                npcFallModel.localPosition = glm::mix(startPos, midPoint, t);

            }
            else {
                t = (t - 0.5f) * 2.0f;
                npcCharacter.localPosition = glm::mix(midPoint, endPos, t);

                npcStandModel.localPosition = glm::mix(midPoint, endPos, t);
                npcJumpModel.localPosition = glm::mix(midPoint, endPos, t);
                npcFallModel.localPosition = glm::mix(midPoint, endPos, t);

            }
        }
        break;
    }

    case GameState::PlayerJumping:
    case GameState::NPCJumping: {
        CharacterState& activeState = (currentState == GameState::PlayerJumping) ? playerState : npcState;
        AnimationObject& activeChar = (currentState == GameState::PlayerJumping) ? playerCharacter : npcCharacter;

        float jumpDuration = getJumpDuration(activeState.currentPattern.startPos);
        float phase = (currentTime - sequenceStartTime) / jumpDuration;

        // Play jump sound at the start of the jump
        if (phase < 0.1f && !activeState.isJumping) {
            bool isQuickJump = (activeState.currentPattern.startPos == Position::CloseSide);

            if (currentState == GameState::PlayerJumping) {
                // Player sounds
                audio.playSound(isQuickJump ? "seesaw_saw_quick" : "seesaw_saw_normal");
            }
            else {
                // NPC sounds
                audio.playSound(isQuickJump ? "seesaw_see_quick" : "seesaw_see_normal");
            }
        }

        if (phase >= 1.0f) {
            // Complete jump and switch states
            currentState = (currentState == GameState::PlayerJumping) ?
                GameState::NPCJumping : GameState::PlayerJumping;
            sequenceStartTime = currentTime;

            // Update position and select new pattern
            activeState.currentPos = activeState.currentPattern.endPos;
            activeState.isJumping = false;

            // Only select new pattern for the character about to jump
            CharacterState& nextState = (currentState == GameState::PlayerJumping) ? playerState : npcState;
            selectNewPattern(nextState);
            nextState.isJumping = true;
            nextState.jumpStartTime = currentTime;

            // Play sound for the new jump
            bool isQuickJump = (nextState.currentPattern.startPos == Position::CloseSide);
            if (currentState == GameState::PlayerJumping) {
                audio.playSound(isQuickJump ? "seesaw_saw_quick" : "seesaw_saw_normal");
            }
            else {
                audio.playSound(isQuickJump ? "seesaw_see_quick" : "seesaw_see_normal");
            }

            // Reset player-specific states when switching to player's turn
            if (currentState == GameState::PlayerJumping) {
                hasInputThisBeat = false;
                playerCharacter.color = vec4(0.2f, 0.6f, 1.0f, 1.0f);
            }
        }

        updateSeesawTilt(dt);
        updateCharacterJump(activeState, activeChar, dt);
        break;
    }
    }

    // Update matrices
    seesaw.updateMatrix(true);
    playerCharacter.updateMatrix(true);

    playerStandModel.updateMatrix(true);
    playerJumpModel.updateMatrix(true);
    playerFallModel.updateMatrix(true);
    npcStandModel.updateMatrix(true);
    npcJumpModel.updateMatrix(true);
    npcFallModel.updateMatrix(true);

    npcCharacter.updateMatrix(true);
    feedbackCube.updateMatrix(true);
    playerIndicator.updateMatrix(true);
}

void SeesawScene::updateCharacterJump(CharacterState& state, AnimationObject& character, float dt) {
    if (!state.isJumping) {
        vec3 groundedPos = getPositionForCharacter(
            &character == &playerCharacter,
            state.currentPos
        );

        if (&character == &playerCharacter) {
            character.localPosition = groundedPos;
            playerStandModel.localPosition = groundedPos;
            playerJumpModel.localPosition = groundedPos;
            playerFallModel.localPosition = groundedPos;
            currentPlayerModel = PlayerModelState::Standing;
        } else {
            character.localPosition = groundedPos;
            // Update NPC model positions
            npcStandModel.localPosition = groundedPos;
            npcJumpModel.localPosition = groundedPos;
            npcFallModel.localPosition = groundedPos;
            currentNPCModel = PlayerModelState::Standing;
        }
        return;
    }

    float jumpDuration = getJumpDuration(state.currentPattern.startPos);
    float phase = (currentTime - state.jumpStartTime) / jumpDuration;

    // Calculate start and end positions
    vec3 startPos = getPositionForCharacter(
        &character == &playerCharacter,
        state.currentPattern.startPos
    );

    vec3 endPos = getPositionForCharacter(
        &character == &playerCharacter,
        state.currentPattern.endPos
    );

    // Adjust jump height based on pattern
    float actualJumpHeight = state.currentPattern.isHighJump ?
        baseJumpHeight * HIGH_JUMP_MULTIPLIER : baseJumpHeight;

    // Calculate peak position
    vec3 peakPos = (startPos + endPos) * 0.5f + vec3(0.0f, actualJumpHeight, 0.0f);

    // Calculate new position
    vec3 newPos = calculateJumpPosition(phase, startPos, endPos, peakPos);

    // Update position based on whether it's the player or NPC
    if (&character == &playerCharacter) {
        // Update all model positions
        playerStandModel.localPosition = newPos;
        playerJumpModel.localPosition = newPos;
        playerFallModel.localPosition = newPos;

        // Determine which model to show based on vertical movement
        static vec3 lastPos = newPos;
        float verticalVelocity = newPos.y - lastPos.y;
        
        // Add some threshold to avoid flickering
        const float VELOCITY_THRESHOLD = 0.01f;
        
        if (abs(verticalVelocity) < VELOCITY_THRESHOLD && phase > 0.98f) {
            currentPlayerModel = PlayerModelState::Standing;
        } else if (verticalVelocity > VELOCITY_THRESHOLD) {
            currentPlayerModel = PlayerModelState::Jumping;
        } else if (verticalVelocity < -VELOCITY_THRESHOLD) {
            currentPlayerModel = PlayerModelState::Falling;
        }
        
        lastPos = newPos;
    } else {
        // Update NPC model positions
        npcStandModel.localPosition = newPos;
        npcJumpModel.localPosition = newPos;
        npcFallModel.localPosition = newPos;

        // Determine which model to show based on vertical movement
        static vec3 lastNPCPos = newPos;
        float verticalVelocity = newPos.y - lastNPCPos.y;
        
        const float VELOCITY_THRESHOLD = 0.01f;
        
        if (abs(verticalVelocity) < VELOCITY_THRESHOLD && phase > 0.98f) {
            currentNPCModel = PlayerModelState::Standing;
        } else if (verticalVelocity > VELOCITY_THRESHOLD) {
            currentNPCModel = PlayerModelState::Jumping;
        } else if (verticalVelocity < -VELOCITY_THRESHOLD) {
            currentNPCModel = PlayerModelState::Falling;
        }
        
        lastNPCPos = newPos;
        // =
        character.localPosition = newPos;
    }
}

void SeesawScene::updateSeesawTilt(float dt) {
    float targetAngle;
    Position activePos = (currentState == GameState::PlayerJumping) ?
        playerState.currentPattern.startPos :
        npcState.currentPattern.startPos;

    // Adjust tilt angle based on position
    float tiltMultiplier = (activePos == Position::FarSide) ? 1.0f : 0.6f;

    if (currentState == GameState::PlayerJumping) {
        targetAngle = seesawTiltAngle * tiltMultiplier;
    }
    else {
        targetAngle = -seesawTiltAngle * tiltMultiplier;
    }

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

void SeesawScene::handlePlayerInput() {
    auto& input = Input::get();
    bool isLPressed = input.current.keyStates[GLFW_KEY_L] == GLFW_PRESS;
    float jumpDuration = getJumpDuration(playerState.currentPattern.startPos);
    float phase = (currentTime - sequenceStartTime) / jumpDuration;

    // Only check for missed inputs during player's turn and after descent begins
    if (currentState == GameState::PlayerJumping &&
        !hasInputThisBeat && phase >= 0.85f) { // Starting descent
        // Check for miss only after passing the good timing window
        if (phase > 1.0f + GOOD_WINDOW) {
            lastTimingResult = TimingResult::Miss;
            lastFeedbackTime = currentTime;
            hasInputThisBeat = true;
            badTimingAnimationTime = BAD_ANIMATION_DURATION;
            feedbackCube.color = getTimingResultColor(TimingResult::Miss);
            playerCharacter.color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    if (isLPressed && !lastInputState && !hasInputThisBeat) {
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
                playerCharacter.color = vec4(1.0f, 0.84f, 0.0f, 1.0f);  // Temporarily gold
                if (!playerState.currentPattern.isHighJump && shouldTriggerHighJump()) {
                    playerState.currentPattern.isHighJump = true;
                }
            }
            else if (lastTimingResult == TimingResult::Good) {
                playerCharacter.color = vec4(0.0f, 1.0f, 0.0f, 1.0f);  // Temporarily green
            }
        }
    }

    lastInputState = isLPressed;
}

SeesawScene::TimingResult SeesawScene::checkTiming(float phase) {
    float timingDiff;

    // Landing occurs at phase 1.0
    // Perfect timing should be just before landing
    const float LANDING_START = 0.85f; // When character starts final descent
    const float IDEAL_TIMING = 0.95f;  // Ideal timing point (just before landing)

    if (currentState == GameState::PlayerJumping) {
        // Calculate timing difference relative to ideal timing point
        timingDiff = abs(phase - IDEAL_TIMING);
    }
    else if (currentState == GameState::NPCJumping) {
        // If we're in early part of NPC jumping, consider it relative to previous landing
        if (phase < 0.2f) {
            timingDiff = abs(0.0f - phase);
        }
        else {
            return TimingResult::Miss;
        }
    }
    else {
        return TimingResult::Miss;
    }

    // Adjust windows to favor early inputs
    if (phase < IDEAL_TIMING) {
        // More forgiving for early inputs
        if (timingDiff <= PERFECT_WINDOW * 1.5f) return TimingResult::Perfect;
        if (timingDiff <= GOOD_WINDOW * 1.5f) return TimingResult::Good;
        if (timingDiff <= BAD_WINDOW) return TimingResult::Bad;
    }
    else {
        // Stricter for late inputs
        if (timingDiff <= PERFECT_WINDOW) return TimingResult::Perfect;
        if (timingDiff <= GOOD_WINDOW) return TimingResult::Good;
        if (timingDiff <= BAD_WINDOW * 0.8f) return TimingResult::Bad;
    }

    return TimingResult::Miss;
}

void SeesawScene::updateBadTimingAnimation(float dt) {

}

void SeesawScene::updateFeedbackCube(float dt) {
    float currentJumpDuration = getJumpDuration(
        currentState == GameState::PlayerJumping ?
        playerState.currentPattern.startPos :
        npcState.currentPattern.startPos
    );

    float phase = (currentTime - sequenceStartTime) / currentJumpDuration;

    // Keep feedback cube in fixed position above seesaw center
    feedbackCube.localPosition = seesawPivotPoint + vec3(0.0f, 2.5f, 0.0f);

    // Base pulsing effect
    beatPulseScale = 1.0f + 0.2f * sin(phase * 2.0f * M_PI);
    feedbackCube.localScale = vec3(0.5f * beatPulseScale);

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

    // Render appropriate player model based on state
    AnimationObject* currentModel = &playerStandModel;
    switch (currentPlayerModel) {
        case PlayerModelState::Jumping:
            currentModel = &playerJumpModel;
            break;
        case PlayerModelState::Falling:
            currentModel = &playerFallModel;
            break;
        default:
            currentModel = &playerStandModel;
            break;
    }

    jr.beginBatchRender(*currentModel, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(*currentModel, isShadow);
    jr.endBatchRender(isShadow);

        // Render NPC model
    AnimationObject* currentNPCModelrender = &npcStandModel;
    switch (currentNPCModel) {
        case PlayerModelState::Jumping:
            currentNPCModelrender = &npcJumpModel;
            break;
        case PlayerModelState::Falling:
            currentNPCModelrender = &npcFallModel;
            break;
        default:
            currentNPCModelrender = &npcStandModel;
            break;
    }

    jr.beginBatchRender(*currentNPCModelrender, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(*currentNPCModelrender, isShadow);
    jr.endBatchRender(isShadow);


    // Render characters and indicators
    jr.beginBatchRender(playerCharacter.shapeType, false, vec4(1.f), isShadow);
    // jr.renderBatchWithOwnColor(playerCharacter, isShadow);
    // jr.renderBatchWithOwnColor(npcCharacter, isShadow);
    jr.renderBatchWithOwnColor(feedbackCube, isShadow);
    jr.renderBatchWithOwnColor(playerIndicator, isShadow);
    jr.endBatchRender(isShadow);
}

void SeesawScene::renderUI() {
    if (ImGui::CollapsingHeader("Seesaw Settings")) {
        ImGui::SliderFloat("Base Jump Height", &baseJumpHeight, 1.0f, 8.0f);
        ImGui::SliderFloat("Seesaw Tilt", &seesawTiltAngle, 5.0f, 30.0f);
        ImGui::SliderFloat("Character Offset", &characterOffset, 0.0f, 2.0f);

        ImGui::Separator();

        // Pattern info
        ImGui::Text("Player Pattern: %s to %s",
            playerState.currentPattern.startPos == Position::FarSide ? "Far" : "Close",
            playerState.currentPattern.endPos == Position::FarSide ? "Far" : "Close");
        ImGui::Text("NPC Pattern: %s to %s",
            npcState.currentPattern.startPos == Position::FarSide ? "Far" : "Close",
            npcState.currentPattern.endPos == Position::FarSide ? "Far" : "Close");

        // Current state and timing info
        const char* stateStr = "Unknown";
        switch (currentState) {
        case GameState::StartSequence: stateStr = "Start Sequence"; break;
        case GameState::PlayerJumping: stateStr = "Player Jumping"; break;
        case GameState::NPCJumping: stateStr = "NPC Jumping"; break;
        }
        ImGui::Text("Current State: %s", stateStr);

        float currentJumpDuration = getJumpDuration(
            currentState == GameState::PlayerJumping ?
            playerState.currentPattern.startPos :
            npcState.currentPattern.startPos
        );
        ImGui::Text("Beat Progress: %.2f", (currentTime - sequenceStartTime) / currentJumpDuration);

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

vec3 SeesawScene::calculateJumpPosition(float t, const vec3& startPos, const vec3& endPos, const vec3& peakPos) {
    float verticalT;
    if (t < 0.3f) {
        // Going up (30% of time)
        verticalT = t / 0.3f * 0.5f;
    }
    else if (t < 0.7f) {
        // At peak (40% of time)
        verticalT = 0.5f;
    }
    else {
        // Going down (30% of time)
        float descendT = (t - 0.7f) / 0.3f;
        verticalT = 0.5f + descendT * 0.5f;

        // Smooth landing without extra drop
        if (descendT > 0.8f) {
            float landingT = (descendT - 0.8f) / 0.2f;
            // Use smoothstep for landing
            landingT = landingT * landingT * (3.0f - 2.0f * landingT);
            verticalT = glm::mix(verticalT, 1.0f, landingT);
        }
    }

    // Calculate vertical position using sine curve
    float height = sin(verticalT * M_PI);

    // Smoothly interpolate horizontal position
    vec3 horizontalPos = glm::mix(startPos, endPos, t);

    // Ensure perfect landing at t = 1.0
    if (t >= 0.99f) {
        return endPos;
    }

    return horizontalPos + vec3(0.0f, height * (peakPos.y - startPos.y), 0.0f);
}

void SeesawScene::initializePositions() {
    seesawPivotPoint = vec3(0.0f);
    currentTiltAngle = -seesawTiltAngle;

    // Initialize seesaw with new length
    seesaw.localPosition = seesawPivotPoint;
    seesaw.localRotation = vec3(0.0f, 0.0f, currentTiltAngle);
    seesaw.localScale = vec3(SEESAW_LENGTH, 0.2f, 1.0f);

    // Set initial positions with new offsets
    playerCharacter.localPosition = getPositionForCharacter(true, Position::FarSide);

    playerStandModel.localPosition = getPositionForCharacter(true, Position::FarSide);
    playerJumpModel.localPosition = getPositionForCharacter(true, Position::FarSide);
    playerFallModel.localPosition = getPositionForCharacter(true, Position::FarSide);


    npcCharacter.localPosition = vec3(-SEESAW_LENGTH * 0.5f - 2.0f, 2.0f, 0.0f);

    // Position player indicator
    playerIndicator.localPosition = getPositionForCharacter(true, Position::FarSide) +
        vec3(0.0f, -1.0f, 0.0f);
}

void SeesawScene::updateCharacterPositions(float dt) {
    float phase = (currentTime - sequenceStartTime) / getJumpDuration(
        currentState == GameState::PlayerJumping ?
        playerState.currentPattern.startPos :
        npcState.currentPattern.startPos
    );

    AnimationObject* jumpingChar = (currentState == GameState::PlayerJumping) ?
        &playerCharacter : &npcCharacter;
    CharacterState& activeState = (currentState == GameState::PlayerJumping) ?
        playerState : npcState;
    bool isPlayer = (jumpingChar == &playerCharacter);

    vec3 startPos = getPositionForCharacter(isPlayer, activeState.currentPattern.startPos);
    vec3 endPos = getPositionForCharacter(isPlayer, activeState.currentPattern.endPos);
    float actualJumpHeight = activeState.currentPattern.isHighJump ?
        jumpHeight * HIGH_JUMP_MULTIPLIER : jumpHeight;

    vec3 peakPos = (startPos + endPos) * 0.5f + vec3(0.0f, actualJumpHeight, 0.0f);
    jumpingChar->localPosition = calculateJumpPosition(phase, startPos, endPos, peakPos);

    // Update grounded character position
    AnimationObject* groundedChar = (currentState == GameState::PlayerJumping) ?
        &npcCharacter : &playerCharacter;
    CharacterState& groundedState = (currentState == GameState::PlayerJumping) ?
        npcState : playerState;
    groundedChar->localPosition = getPositionForCharacter(
        groundedChar == &playerCharacter,
        groundedState.currentPos
    );
}

vec3 SeesawScene::calculateSeesawEndPoint(float side, float tiltAngle) {
    float adjustedLength = (seesawLength * 0.5f) - characterOffset;
    return seesawPivotPoint +
        vec3(cos(glm::radians(tiltAngle)) * adjustedLength * side,
            sin(glm::radians(tiltAngle)) * adjustedLength * side + characterBaseHeight,
            0.0f);
}

ptr_vector<AnimationObject> SeesawScene::getObjects() {
    ptr_vector<AnimationObject> objects;
    objects.push_back(&seesaw);
    objects.push_back(&playerCharacter);
    // Add appropriate player model based on state
    switch (currentPlayerModel) {
        case PlayerModelState::Jumping:
            objects.push_back(&playerJumpModel);
            break;
        case PlayerModelState::Falling:
            objects.push_back(&playerFallModel);
            break;
        default:
            objects.push_back(&playerStandModel);
            break;
    }    
    
    switch (currentNPCModel) {
        case PlayerModelState::Jumping:
            objects.push_back(&npcJumpModel);
            break;
        case PlayerModelState::Falling:
            objects.push_back(&npcFallModel);
            break;
        default:
            objects.push_back(&npcStandModel);
            break;
    }

    objects.push_back(&npcCharacter);

    objects.push_back(&feedbackCube);
    objects.push_back(&playerIndicator);
    return objects;
}

vec4 SeesawScene::getTimingResultColor(TimingResult result) {
    switch (result) {
    case TimingResult::Perfect: return vec4(1.0f, 0.84f, 0.0f, 1.0f);  // Gold
    case TimingResult::Good: return vec4(0.0f, 1.0f, 0.0f, 1.0f);      // Green
    case TimingResult::Bad: return vec4(1.0f, 0.0f, 0.0f, 1.0f);       // Red
    default: return vec4(0.5f, 0.5f, 0.5f, 1.0f);                      // Gray
    }
}

