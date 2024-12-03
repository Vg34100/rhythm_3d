#include "BuiltToScaleScene.h"
#include "AnimationObjectRenderer.h"
#include "Input.h"
#include "imgui.h"
#include <iostream>

BuiltToScaleScene::BuiltToScaleScene() {
    springs.resize(NUM_SPRINGS);
    springAnims.resize(NUM_SPRINGS);
    for (auto& anim : springAnims) {
        anim.isAnimating = false;
        anim.animationTime = 0.0f;
        anim.baseHeight = 0.0f;
    }

    // Initialize pattern info
    patternInfo = {
        Pattern::ThreeStep,  // Start with simplest pattern
        3,                   // Steps remaining
        3,                   // Total steps
        false,              // Spring not retracted
        0.0f,               // Retract anim time
        0.0f,               // Launch anim time
        false               // Not awaiting launch input
    };
}

BuiltToScaleScene::~BuiltToScaleScene() {}

void BuiltToScaleScene::init() {
    initSprings();
    initPole();
    initInputIndicator();
}

void BuiltToScaleScene::initSprings() {
    for (int i = 0; i < NUM_SPRINGS; i++) {
        springs[i] = AnimationObject(AnimationObjectType::box);
        springs[i].localPosition = vec3(i * SPRING_SPACING - (NUM_SPRINGS - 1) * SPRING_SPACING * 0.5f, 0.0f, 0.0f);
        springs[i].localScale = vec3(0.7f);  // Cubic springs
        springs[i].color = (i == 2) ? vec4(1.0f, 0.0f, 0.0f, 1.0f) : vec4(1.0f);
        springs[i].updateMatrix(true);
    }
}

void BuiltToScaleScene::initPole() {
    pole = AnimationObject(AnimationObjectType::box);
    pole.localScale = vec3(0.1f, 1.0f, 0.1f);
    pole.localRotation = vec3(90.0f, 0.0f, 90.0f);
    pole.color = vec4(1.0f, 1.0f, 1.0f, 1.0f);

    float baseHeight = 0.5f;
    pole.localPosition = vec3(springs[0].localPosition.x - 3.0f, baseHeight, 0.0f);

    poleAnim.currentSpringIndex = 0;  // Explicitly start at first spring
    poleAnim.movingRight = true;
    poleAnim.animationTime = 0.0f;
    poleAnim.startPos = springs[0].localPosition;  // Start at first spring
    poleAnim.endPos = springs[1].localPosition;    // Bounce to second spring
    poleAnim.state = PoleState::Normal;
    poleAnim.inputRequired = false;
    poleAnim.inputSuccess = false;
}

void BuiltToScaleScene::initInputIndicator() {
    inputIndicator = AnimationObject(AnimationObjectType::box);
    inputIndicator.localScale = vec3(0.5f);  // Made it bigger
    inputIndicator.localPosition = springs[2].localPosition + vec3(0.0f, 2.0f, 0.0f);  // Moved it up
    inputIndicator.color = vec4(1.0f, 1.0f, 1.0f, 1.0f);  // White by default
    inputIndicator.updateMatrix(true);  // Make sure to update the matrix
}

void BuiltToScaleScene::update(double now, float dt) {
    handleInput();
    updatePoleAnimation(dt);
    updateInputIndicator(now, dt);

    // Update spring animations
    for (size_t i = 0; i < springs.size(); i++) {
        if (springAnims[i].isAnimating) {
            springAnims[i].animationTime += dt;

            if (springAnims[i].animationTime >= SpringAnimation::ANIM_DURATION) {
                springAnims[i].isAnimating = false;
                springs[i].localPosition.y = springAnims[i].baseHeight;
            }
            else {
                float t = springAnims[i].animationTime / SpringAnimation::ANIM_DURATION;
                float bounce = sin(t * M_PI) * SpringAnimation::MAX_BOUNCE;
                springs[i].localPosition.y = springAnims[i].baseHeight + bounce;
            }
        }
        springs[i].updateMatrix(true);
    }

    pole.updateMatrix(true);
    inputIndicator.updateMatrix(true);
}

void BuiltToScaleScene::handleInput() {
    if (poleAnim.state != PoleState::Normal) return;

    auto& input = Input::get();
    bool isLPressed = input.current.keyStates[GLFW_KEY_L] == GLFW_PRESS;
    bool isKPressed = input.current.keyStates[GLFW_KEY_K] == GLFW_PRESS;
    static bool wasLPressed = false;
    static bool wasKPressed = false;

    // If spring is retracted, only allow K press
    if (patternInfo.springRetracted) {
        if (isLPressed) {
            failPole();
            return;
        }

        if (isKPressed && !wasKPressed) {
            float normalizedTime = poleAnim.animationTime / BOUNCE_DURATION;
            // Make timing window very obvious for testing
            //if (normalizedTime >= 0.2f && normalizedTime <= 0.8f) {
            if (normalizedTime >= (0.5f - INPUT_WINDOW)) {
                std::cout << "Launch initiated!" << std::endl;  // Debug print
                poleAnim.state = PoleState::Launching;
                patternInfo.launchAnimTime = 0.0f;
                patternInfo.awaitingLaunchInput = false;
                springs[2].localPosition.z = 0.0f;
            }
            else {
                failPole();
            }
        }
    }
    else {
        // Regular bounce logic for non-launch steps
        bool needsInput = (poleAnim.currentSpringIndex == 1 && poleAnim.movingRight) ||
            (poleAnim.currentSpringIndex == 3 && !poleAnim.movingRight);

        if (needsInput) {
            float normalizedTime = poleAnim.animationTime / BOUNCE_DURATION;
            if (normalizedTime >= (0.5f - INPUT_WINDOW)) {
                poleAnim.inputRequired = true;
            }

            if (poleAnim.inputRequired) {
                if (isLPressed && !wasLPressed) {
                    poleAnim.inputSuccess = true;
                    poleAnim.inputRequired = false;
                    inputIndicator.color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
                }
            }
        }

        if (poleAnim.currentSpringIndex == 2 && !poleAnim.inputSuccess) {
            failPole();
        }
    }

    wasLPressed = isLPressed;
    wasKPressed = isKPressed;
}

void BuiltToScaleScene::updateInputIndicator(double now, float dt) {
    if (patternInfo.springRetracted && poleAnim.state == PoleState::Normal) {
        // Make it more obvious when we can launch
        inputIndicator.color = vec4(1.0f, 0.0f, 1.0f, 1.0f);  // Purple for launch window
        inputIndicator.localScale = vec3(0.4f + 0.2f * sin(now * 10.0f));  // Bigger pulse
    }
    else if (poleAnim.inputRequired) {
        inputIndicator.color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
        inputIndicator.localScale = vec3(0.3f + 0.1f * sin(now * 10.0f));
    }
    else if (!poleAnim.inputSuccess) {
        inputIndicator.color = vec4(0.5f, 0.5f, 0.5f, 1.0f);
        inputIndicator.localScale = vec3(0.3f);
    }
}

void BuiltToScaleScene::failPole() {
    poleAnim.state = PoleState::Failed;
    poleAnim.failTime = 0.0f;
    poleAnim.startPos = pole.localPosition;
    poleAnim.endPos = pole.localPosition + vec3(2.0f, -3.0f, 0.0f);
    inputIndicator.color = vec4(1.0f, 0.0f, 0.0f, 1.0f);

    // Reset spring position on fail
    springs[2].localPosition.z = 0.0f;
    patternInfo.springRetracted = false;
}

vec3 BuiltToScaleScene::calculateFailPosition(float t) {
    vec3 gravity = vec3(0.0f, -9.81f, 0.0f);
    vec3 initialVelocity = vec3(3.0f, 2.0f, 0.0f);
    return poleAnim.startPos + initialVelocity * t + 0.5f * gravity * t * t;
}

void BuiltToScaleScene::updatePoleAnimation(float dt) {
    switch (poleAnim.state) {
    case PoleState::Normal:
        poleAnim.animationTime += dt;

        // Don't automatically move to next bounce if waiting for launch
        if (poleAnim.animationTime >= BOUNCE_DURATION && !patternInfo.springRetracted) {
            startNewBounce();
        }
        // Add this check: If we missed the launch window completely, fail
        if (patternInfo.springRetracted) {
            float normalizedTime = poleAnim.animationTime / BOUNCE_DURATION;
            if (normalizedTime > 2.0f) { // Past the launch window
                failPole();
                return;
            }
        }

        updateSpringRetraction(dt);
        pole.localPosition = calculatePolePosition(poleAnim.animationTime / BOUNCE_DURATION);
        break;

    case PoleState::Failed:
        poleAnim.failTime += dt;
        if (poleAnim.failTime >= FAIL_DURATION) {
            respawnPole();
        }
        else {
            pole.localPosition = calculateFailPosition(poleAnim.failTime);
            pole.localRotation += vec3(10.0f, 0.0f, 720.0f * dt);
        }
        break;

    case PoleState::Respawning:
        poleAnim.animationTime += dt;
        if (poleAnim.animationTime >= BOUNCE_DURATION) {
            poleAnim.state = PoleState::Normal;
            poleAnim.animationTime = 0.0f;
            poleAnim.startPos = springs[poleAnim.currentSpringIndex].localPosition;
            poleAnim.endPos = springs[poleAnim.movingRight ?
                poleAnim.currentSpringIndex + 1 :
                poleAnim.currentSpringIndex - 1].localPosition;
        }
        else {
            // Arc coming down onto first/last spring based on direction
            float t = poleAnim.animationTime / BOUNCE_DURATION;
            float baseHeight = 0.5f;

            vec3 startPos, endPos;
            if (poleAnim.movingRight) {
                startPos = vec3(springs[0].localPosition.x - 3.0f, baseHeight + 2.0f, 0.0f);
                endPos = springs[0].localPosition + vec3(0.0f, baseHeight, 0.0f);
            }
            else {
                startPos = vec3(springs[NUM_SPRINGS - 1].localPosition.x + 3.0f, baseHeight + 2.0f, 0.0f);
                endPos = springs[NUM_SPRINGS - 1].localPosition + vec3(0.0f, baseHeight, 0.0f);
            }

            vec3 controlPoint = (startPos + endPos) * 0.5f;
            controlPoint.y += 1.0f;

            pole.localPosition = (1.0f - t) * (1.0f - t) * startPos +
                2.0f * (1.0f - t) * t * controlPoint +
                t * t * endPos;
        }
        break;


    case PoleState::Launching:
        std::cout << "Updating launch animation" << std::endl;  // Debug print
        updateLaunchAnimation(dt);
        break;
    }
}

void BuiltToScaleScene::respawnPole() {
    poleAnim.state = PoleState::Respawning;
    poleAnim.animationTime = 0.0f;
    selectRandomPattern(); // Choose new pattern on respawn

    poleAnim.inputRequired = false;
    poleAnim.inputSuccess = false;
    pole.localRotation = vec3(90.0f, 0.0f, 90.0f);

    float baseHeight = 0.5f;
    // Set position based on pattern's starting point
    if (poleAnim.movingRight) {
        pole.localPosition = vec3(springs[0].localPosition.x - 3.0f, baseHeight, 0.0f);
        poleAnim.startPos = springs[0].localPosition;
        poleAnim.endPos = springs[1].localPosition;
    }
    else {
        pole.localPosition = vec3(springs[3].localPosition.x + 3.0f, baseHeight, 0.0f);
        poleAnim.startPos = springs[3].localPosition;
        poleAnim.endPos = springs[2].localPosition;
    }
}

void BuiltToScaleScene::startNewBounce() {
    // Don't auto-bounce if we're on the final step waiting for launch
    if (patternInfo.stepsRemaining == 1 && patternInfo.springRetracted) {
        return;  // Wait for player input instead of bouncing
    }

    // Don't bounce if we've used all steps
    if (patternInfo.stepsRemaining <= 0) {
        failPole();
        return;
    }

    poleAnim.animationTime = 0.0f;
    patternInfo.stepsRemaining--;

    if (poleAnim.movingRight) {
        poleAnim.currentSpringIndex++;
        if (poleAnim.currentSpringIndex >= NUM_SPRINGS - 1) {
            poleAnim.movingRight = false;
        }
    }
    else {
        poleAnim.currentSpringIndex--;
        if (poleAnim.currentSpringIndex <= 0) {
            poleAnim.movingRight = true;
        }
    }

    triggerSpringBounce(poleAnim.currentSpringIndex);

    poleAnim.startPos = springs[poleAnim.currentSpringIndex].localPosition;
    poleAnim.endPos = springs[poleAnim.movingRight ?
        poleAnim.currentSpringIndex + 1 :
        poleAnim.currentSpringIndex - 1].localPosition;
}


vec3 BuiltToScaleScene::calculatePolePosition(float t) {
    float oneMinusT = 1.0f - t;

    // Get base positions with correct height for start and end
    float baseHeight = 0.5f;  // Height of springs (their scale is 0.5f)
    vec3 heightAdjustedStart = poleAnim.startPos + vec3(0.0f, baseHeight, 0.0f);
    vec3 heightAdjustedEnd = poleAnim.endPos + vec3(0.0f, baseHeight, 0.0f);

    // Calculate control point for the arc, adding bounce height on top of base height
    vec3 controlPoint = (heightAdjustedStart + heightAdjustedEnd) * 0.5f;
    controlPoint.y += BOUNCE_HEIGHT;

    // Calculate final position using quadratic bezier
    return oneMinusT * oneMinusT * heightAdjustedStart +
        2.0f * oneMinusT * t * controlPoint +
        t * t * heightAdjustedEnd;
}

void BuiltToScaleScene::render(const mat4& projection, const mat4& view, bool isShadow) {
    auto& jr = AnimationObjectRenderer::get();

    // Render springs
    for (const auto& spring : springs) {
        jr.beginBatchRender(spring.shapeType, false, vec4(1.f), isShadow);
        jr.renderBatchWithOwnColor(spring, isShadow);
        jr.endBatchRender(isShadow);
    }

    // Render pole
    jr.beginBatchRender(pole.shapeType, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(pole, isShadow);
    jr.endBatchRender(isShadow);
}

void BuiltToScaleScene::renderUI() {
    if (ImGui::CollapsingHeader("Built to Scale Debug")) {
        const char* patternNames[] = { "Three Step", "Five Step", "Eight Step" };
        ImGui::Text("Pattern: %s", patternNames[static_cast<int>(patternInfo.type)]);
        ImGui::Text("Steps Remaining: %d/%d", patternInfo.stepsRemaining, patternInfo.totalSteps);
        ImGui::Text("Spring Retracted: %s", patternInfo.springRetracted ? "Yes" : "No");
        ImGui::Text("Current Spring Index: %d", poleAnim.currentSpringIndex);
        ImGui::Text("Moving Right: %s", poleAnim.movingRight ? "Yes" : "No");
        ImGui::Text("Animation Time: %.2f", poleAnim.animationTime);
        ImGui::Text("State: %s",
            poleAnim.state == PoleState::Normal ? "Normal" :
            poleAnim.state == PoleState::Failed ? "Failed" :
            poleAnim.state == PoleState::Launching ? "Launching" : "Respawning");
        if (poleAnim.state == PoleState::Failed) {
            ImGui::Text("Fail Time: %.2f", poleAnim.failTime);
        }
    }
}

ptr_vector<AnimationObject> BuiltToScaleScene::getObjects() {
    ptr_vector<AnimationObject> objects;
    for (auto& spring : springs) {
        objects.push_back(&spring);
    }
    objects.push_back(&pole);
    objects.push_back(&inputIndicator);
    return objects;
}

void BuiltToScaleScene::triggerSpringBounce(int springIndex) {
    // Only animate the red spring (index 2) if input was successful
    if (springIndex == 2 && !poleAnim.inputSuccess) return;

    springAnims[springIndex].isAnimating = true;
    springAnims[springIndex].animationTime = 0.0f;
    springAnims[springIndex].baseHeight = springs[springIndex].localPosition.y;
}

void BuiltToScaleScene::selectRandomPattern() {
    Pattern patterns[] = { Pattern::ThreeStep, Pattern::FiveStep, Pattern::EightStep };
    patternInfo.type = patterns[rand() % 3];

    switch (patternInfo.type) {
    case Pattern::ThreeStep:
        patternInfo.totalSteps = 3;
        poleAnim.currentSpringIndex = 0;
        poleAnim.movingRight = true;
        break;
    case Pattern::FiveStep:
        patternInfo.totalSteps = 5;
        poleAnim.currentSpringIndex = 0;
        poleAnim.movingRight = true;
        break;
    case Pattern::EightStep:
        patternInfo.totalSteps = 8;
        poleAnim.currentSpringIndex = 3;
        poleAnim.movingRight = false;
        break;
    }

    patternInfo.stepsRemaining = patternInfo.totalSteps;
    patternInfo.springRetracted = false;
    patternInfo.retractAnimTime = 0.0f;
    patternInfo.launchAnimTime = 0.0f;
    patternInfo.awaitingLaunchInput = false;

    // Reset spring position
    springs[2].localPosition.z = 0.0f;
}

bool BuiltToScaleScene::isSecondToLastStep() const {
    return patternInfo.stepsRemaining == 2;
}

bool BuiltToScaleScene::isFinalStep() const {
    return patternInfo.stepsRemaining == 1;
}

void BuiltToScaleScene::updateSpringRetraction(float dt) {
    if (isSecondToLastStep() && !patternInfo.springRetracted) {
        patternInfo.retractAnimTime += dt;
        if (patternInfo.retractAnimTime >= RETRACT_DURATION) {
            patternInfo.springRetracted = true;
            patternInfo.retractAnimTime = RETRACT_DURATION;
            patternInfo.awaitingLaunchInput = true;
        }

        // Animate spring retraction in Z direction (backwards)
        float t = patternInfo.retractAnimTime / RETRACT_DURATION;
        springs[2].localPosition.z = -(t * 1.0f); // Move back by 1 unit
    }
}

void BuiltToScaleScene::handleLaunchInput() {
    if (!patternInfo.awaitingLaunchInput) return;

    auto& input = Input::get();
    bool isKPressed = input.current.keyStates[GLFW_KEY_K] == GLFW_PRESS;
    static bool wasKPressed = false;

    if (isKPressed && !wasKPressed) {
        float normalizedTime = poleAnim.animationTime / BOUNCE_DURATION;
        if (normalizedTime >= 0.3f && normalizedTime <= 0.7f) {
            poleAnim.state = PoleState::Launching;
            patternInfo.launchAnimTime = 0.0f;
            patternInfo.awaitingLaunchInput = false;

            // Reset spring position
            springs[2].localPosition.z = 0.0f; // Reset Z position instead of X
        }
        else {
            failPole();
        }
    }

    wasKPressed = isKPressed;
}

void BuiltToScaleScene::updateLaunchAnimation(float dt) {
    if (poleAnim.state != PoleState::Launching) return;

    std::cout << "Launch animation time: " << patternInfo.launchAnimTime << std::endl;  // Debug print

    patternInfo.launchAnimTime += dt;
    if (patternInfo.launchAnimTime >= LAUNCH_DURATION) {
        respawnPole();
        return;
    }

    // Very obvious launch animation for testing
    pole.localPosition += vec3(0.0f, 0.0f, LAUNCH_SPEED * 5.0f * dt);  // Much faster
    pole.localRotation += vec3(0.0f, 0.0f, 2880.0f * dt);  // More spinning
}