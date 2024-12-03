#include "BuiltToScaleScene.h"
#include "AnimationObjectRenderer.h"
#include "Input.h"
#include "imgui.h"

BuiltToScaleScene::BuiltToScaleScene() {
    springs.resize(NUM_SPRINGS);
    springAnims.resize(NUM_SPRINGS);
    // Initialize spring animations
    for (auto& anim : springAnims) {
        anim.isAnimating = false;
        anim.animationTime = 0.0f;
        anim.baseHeight = 0.0f;
    }
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
    inputIndicator.localScale = vec3(0.3f);
    inputIndicator.localPosition = springs[2].localPosition + vec3(0.0f, 1.5f, 0.0f);
    inputIndicator.color = vec4(0.5f, 0.5f, 0.5f, 1.0f);
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
    static bool wasLPressed = false;

    // Need input when approaching spring 2 from EITHER direction
    bool needsInput = (poleAnim.currentSpringIndex == 1 && poleAnim.movingRight) ||
        (poleAnim.currentSpringIndex == 3 && !poleAnim.movingRight);

    if (needsInput) {
        float normalizedTime = poleAnim.animationTime / BOUNCE_DURATION;

        // Set the input window active as we approach the spring
        if (normalizedTime >= (0.5f - INPUT_WINDOW)) {
            poleAnim.inputRequired = true;
        }

        // Handle input during the window
        if (poleAnim.inputRequired) {
            if (isLPressed && !wasLPressed) {
                poleAnim.inputSuccess = true;
                poleAnim.inputRequired = false;
                inputIndicator.color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
            }
        }
    }

    // Check for failure if we're at spring 2 and didn't succeed
    if (poleAnim.currentSpringIndex == 2 && !poleAnim.inputSuccess) {
        failPole();
    }

    wasLPressed = isLPressed;
}

void BuiltToScaleScene::updateInputIndicator(double now, float dt) {
    if (poleAnim.inputRequired) {
        inputIndicator.color = vec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow during input window
        inputIndicator.localScale = vec3(0.3f + 0.1f * sin(now * 10.0f));  // Pulse using actual time
    }
    else if (!poleAnim.inputSuccess) {
        inputIndicator.color = vec4(0.5f, 0.5f, 0.5f, 1.0f);  // Gray when inactive
        inputIndicator.localScale = vec3(0.3f);
    }
}

void BuiltToScaleScene::failPole() {
    poleAnim.state = PoleState::Failed;
    poleAnim.failTime = 0.0f;
    poleAnim.startPos = pole.localPosition;
    poleAnim.endPos = pole.localPosition + vec3(2.0f, -3.0f, 0.0f);
    inputIndicator.color = vec4(1.0f, 0.0f, 0.0f, 1.0f);  // Red for failure
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
        if (poleAnim.animationTime >= BOUNCE_DURATION) {
            startNewBounce();
        }
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
    }
}

void BuiltToScaleScene::respawnPole() {
    poleAnim.state = PoleState::Respawning;
    poleAnim.animationTime = 0.0f;

    // Randomly choose starting side
    poleAnim.movingRight = (rand() % 2) == 0;

    // Set starting spring index based on direction
    poleAnim.currentSpringIndex = poleAnim.movingRight ? 0 : NUM_SPRINGS - 1;

    poleAnim.inputRequired = false;
    poleAnim.inputSuccess = false;
    pole.localRotation = vec3(90.0f, 0.0f, 90.0f);

    float baseHeight = 0.5f;
    // Position based on chosen side
    if (poleAnim.movingRight) {
        pole.localPosition = vec3(
            springs[0].localPosition.x - 3.0f,  // Left of first spring
            baseHeight,
            0.0f
        );
        poleAnim.startPos = springs[0].localPosition;
        poleAnim.endPos = springs[1].localPosition;
    }
    else {
        pole.localPosition = vec3(
            springs[NUM_SPRINGS - 1].localPosition.x + 3.0f,  // Right of last spring
            baseHeight,
            0.0f
        );
        poleAnim.startPos = springs[NUM_SPRINGS - 1].localPosition;
        poleAnim.endPos = springs[NUM_SPRINGS - 2].localPosition;
    }
}

void BuiltToScaleScene::startNewBounce() {
    // Trigger bounce animation for the spring we're leaving

    poleAnim.animationTime = 0.0f;

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

    // Reset input flags for BOTH approaches to the red spring
    if ((poleAnim.currentSpringIndex == 1 && poleAnim.movingRight) ||
        (poleAnim.currentSpringIndex == 3 && !poleAnim.movingRight)) {
        poleAnim.inputSuccess = false;
        poleAnim.inputRequired = false;
    }
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
        ImGui::Text("Current Spring Index: %d", poleAnim.currentSpringIndex);
        ImGui::Text("Moving Right: %s", poleAnim.movingRight ? "Yes" : "No");
        ImGui::Text("Animation Time: %.2f", poleAnim.animationTime);
        ImGui::Text("State: %s",
            poleAnim.state == PoleState::Normal ? "Normal" :
            poleAnim.state == PoleState::Failed ? "Failed" : "Respawning");
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