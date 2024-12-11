#include "BuiltToScaleScene.h"
#include "AnimationObjectRenderer.h"
#include "Input.h"
#include "imgui.h"
#include <iostream>
#include <Textures.h>

BuiltToScaleScene::BuiltToScaleScene()
    : audio(AudioSystem::get()) {


    //springs.resize(NUM_SPRINGS);
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
    guideSquaresMovedDuringLaunch = false;
    squaresLaunchAnim.isAnimating = false;
    squaresLaunchAnim.animationTime = 0.0f;
}

BuiltToScaleScene::~BuiltToScaleScene() {}

void BuiltToScaleScene::init() {
    initSprings();
    initPole();
    initInputIndicator();

    // =====================================
    // First-time initialization of guide widget models
    leftGuideModel = AnimationObject(AnimationObjectType::model);
    leftGuideModel.meshName = "../assets/models/builttoscale/BTS_PlayerWidgetModel.obj";

    // Create and register texture
    auto widgetTexture = std::make_shared<Texture>(
        "../assets/models/builttoscale/BTS_PlayerWidgetTexture.png");
    TextureRegistry::addTexture(widgetTexture);
    leftGuideModel.texture = reinterpret_cast<void*>(widgetTexture->id);

    // Set up left model properties
    leftGuideModel.localScale = vec3(1.0f);
    leftGuideModel.color = vec4(1.0f); // White to show texture properly

    // Initialize right guide widget (using same texture)
    rightGuideModel = AnimationObject(AnimationObjectType::model);
    rightGuideModel.meshName = "../assets/models/builttoscale/BTS_PlayerWidgetModel.obj";
    rightGuideModel.texture = reinterpret_cast<void*>(widgetTexture->id);
    rightGuideModel.localScale = vec3(1.0f);
    rightGuideModel.color = vec4(1.0f);
    // =====================================
    // Initialize background model
    backgroundModel = AnimationObject(AnimationObjectType::model);
    backgroundModel.meshName = "../assets/models/builttoscale/BTS_BackgroundModel.obj";

    // Create and register texture
    auto backgroundTexture = std::make_shared<Texture>(
        "../assets/models/builttoscale/BTS_BackgroundTexture.png");
    TextureRegistry::addTexture(backgroundTexture);
    backgroundModel.texture = reinterpret_cast<void*>(backgroundTexture->id);

    // Set up model properties
    backgroundModel.localPosition = vec3(0.0f); // Center position
    backgroundModel.localScale = vec3(1.0f);
    backgroundModel.color = vec4(1.0f); // White to show texture properly
    backgroundModel.updateMatrix(true);

    audio.loadScene("builttoscale");
}

void BuiltToScaleScene::initSprings() {
    // Initialize springModels vector to correct size
    springModels.resize(NUM_SPRINGS);

    for (int i = 0; i < NUM_SPRINGS; i++) {
        // Calculate position for both legacy and model springs
        vec3 springPosition = vec3(i * SPRING_SPACING - (NUM_SPRINGS - 1) * SPRING_SPACING * 0.5f, 0.0f, 0.0f);

        // Initialize the model spring
        springModels[i] = AnimationObject(AnimationObjectType::model);

        // Choose between red (player) spring and regular spring models
        if (i == 2) { // Index 2 is the red spring
            springModels[i].meshName = "../assets/models/builttoscale/BTS_PlayerSpringModel.obj";

            // Create and register texture for player spring
            auto playerSpringTexture = std::make_shared<Texture>(
                "../assets/models/builttoscale/BTS_PlayerSpringTexture.png");
            TextureRegistry::addTexture(playerSpringTexture);
            springModels[i].texture = reinterpret_cast<void*>(playerSpringTexture->id);
        }
        else {
            springModels[i].meshName = "../assets/models/builttoscale/BTS_OtherSpringModel.obj";

            // Create and register texture for other springs
            auto otherSpringTexture = std::make_shared<Texture>(
                "../assets/models/builttoscale/BTS_OtherSpringTexture.png");
            TextureRegistry::addTexture(otherSpringTexture);
            springModels[i].texture = reinterpret_cast<void*>(otherSpringTexture->id);
        }

        // Set up model properties
        springModels[i].localPosition = springPosition;
        springModels[i].localScale = vec3(1.0f);
        springModels[i].color = vec4(1.0f); // White to show texture properly
        springModels[i].updateMatrix(true);

    }
}

void BuiltToScaleScene::initPole() {
    float baseHeight = 0.5f;

    // ===========================================================================
    // Initialize the model pole
    poleModel = AnimationObject(AnimationObjectType::model);
    poleModel.meshName = "../assets/models/builttoscale/BTS_PlayerPoleModel.obj";

    // Create and register texture
    auto poleTexture = std::make_shared<Texture>(
        "../assets/models/builttoscale/BTS_PlayerPoleTexture.png");
    TextureRegistry::addTexture(poleTexture);
    poleModel.texture = reinterpret_cast<void*>(poleTexture->id);

    // Set up model properties
    poleModel.localPosition = vec3(springModels[0].localPosition.x - 3.0f, baseHeight, 0.0f);

    poleModel.localScale = vec3(1.0f);
    poleModel.updateMatrix(true);
    // ===========================================================================
    poleAnim.currentSpringIndex = 0;
    poleAnim.movingRight = true;
    poleAnim.animationTime = 0.0f;
    poleAnim.startPos = springModels[0].localPosition;  // Start at first spring
    poleAnim.endPos = springModels[1].localPosition;    // Bounce to second spring
    poleAnim.state = PoleState::Normal;
    poleAnim.inputRequired = false;
    poleAnim.inputSuccess = false;




}

void BuiltToScaleScene::initInputIndicator() {
    inputIndicator = AnimationObject(AnimationObjectType::box);
    inputIndicator.localScale = vec3(0.5f); 
    inputIndicator.localPosition = springModels[2].localPosition + vec3(0.0f, 2.0f, 0.0f);

    inputIndicator.color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    inputIndicator.updateMatrix(true);
}

void BuiltToScaleScene::update(double now, float dt) {
    handleInput();
    updatePoleAnimation(dt);
    updateInputIndicator(now, dt);

    // Update spring animations
    for (size_t i = 0; i < springModels.size(); i++) {
        if (springAnims[i].isAnimating) {
            springAnims[i].animationTime += dt;

            if (springAnims[i].animationTime >= SpringAnimation::ANIM_DURATION) {
                springAnims[i].isAnimating = false;
                springModels[i].localPosition.y = springAnims[i].baseHeight;

            }
            else {
                float t = springAnims[i].animationTime / SpringAnimation::ANIM_DURATION;
                float bounce = sin(t * M_PI) * SpringAnimation::MAX_BOUNCE;
                springModels[i].localPosition.y = springAnims[i].baseHeight + bounce;

            }
        }
        springModels[i].updateMatrix(true);

    }

    poleModel.updateMatrix(true); // POLE
    inputIndicator.updateMatrix(true);

    if (guidesActive) {
        leftGuideModel.updateMatrix(true);
        rightGuideModel.updateMatrix(true);
    }
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
            if (normalizedTime >= (0.5f - INPUT_WINDOW)) {
                std::cout << "Launch initiated!" << std::endl;  // Debug print
                poleAnim.state = PoleState::Launching;
                patternInfo.launchAnimTime = 0.0f;
                patternInfo.awaitingLaunchInput = false;
                springModels[2].localPosition.z = 0.0f;

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
    poleAnim.startPos = poleModel.localPosition;
    poleAnim.endPos = poleModel.localPosition + vec3(2.0f, -3.0f, 0.0f);
    inputIndicator.color = vec4(1.0f, 0.0f, 0.0f, 1.0f);

    // Reset spring position on fail
    springModels[2].localPosition.z = 0.0f;
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

        // If we missed the launch window completely, fail
        if (patternInfo.springRetracted) {
            float normalizedTime = poleAnim.animationTime / BOUNCE_DURATION;

            if (normalizedTime > 2.0f) { // Past the launch window
                failPole();
                return;
            }
        }

        updateSpringRetraction(dt);
        poleModel.localPosition = calculatePolePosition(poleAnim.animationTime / BOUNCE_DURATION);

        break;

    case PoleState::Failed:
        poleAnim.failTime += dt;
        if (poleAnim.failTime >= FAIL_DURATION) {
            respawnPole();
        }
        else {
            poleModel.localPosition = calculateFailPosition(poleAnim.failTime);
            poleModel.localRotation += vec3(10.0f, 0.0f, 720.0f * dt);
        }
        break;

    case PoleState::Respawning:
        poleAnim.animationTime += dt;
        if (poleAnim.animationTime >= BOUNCE_DURATION) {
            poleAnim.state = PoleState::Normal;
            poleAnim.animationTime = 0.0f;
            moveGuideSquares();
            playRandomImpactSound();
            poleAnim.startPos = springModels[poleAnim.currentSpringIndex].localPosition;
            poleAnim.endPos = springModels[poleAnim.movingRight ?
                poleAnim.currentSpringIndex + 1 :
                poleAnim.currentSpringIndex - 1].localPosition;
        }
        else {
            // Arc coming down onto first/last spring based on direction
            float t = poleAnim.animationTime / BOUNCE_DURATION;
            float baseHeight = 0.5f;

            vec3 startPos, endPos;
            if (poleAnim.movingRight) {
                startPos = vec3(springModels[0].localPosition.x - 3.0f, baseHeight + 2.0f, 0.0f);
                endPos = springModels[0].localPosition + vec3(0.0f, baseHeight, 0.0f);
            }
            else {
                startPos = vec3(springModels[NUM_SPRINGS - 1].localPosition.x + 3.0f, baseHeight + 2.0f, 0.0f);
                endPos = springModels[NUM_SPRINGS - 1].localPosition + vec3(0.0f, baseHeight, 0.0f);
            }

            vec3 controlPoint = (startPos + endPos) * 0.5f;
            controlPoint.y += 1.0f;

            poleModel.localPosition = (1.0f - t) * (1.0f - t) * startPos +
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
    poleModel.localRotation = vec3(0.0f, 0.0f, 0.0f);

    float baseHeight = 0.5f;
    // Set position based on pattern's starting point
    if (poleAnim.movingRight) {
        poleModel.localPosition = vec3(springModels[0].localPosition.x - 3.0f, baseHeight, 0.0f);

        poleAnim.startPos = springModels[0].localPosition;
        poleAnim.endPos = springModels[1].localPosition;
    }
    else {
        poleModel.localPosition = vec3(springModels[3].localPosition.x + 3.0f, baseHeight, 0.0f);

        poleAnim.startPos = springModels[3].localPosition;
        poleAnim.endPos = springModels[2].localPosition;
    }

    // ==================
    leftGuideModel.localPosition = squaresLaunchAnim.startLeftPos;
    rightGuideModel.localPosition = squaresLaunchAnim.startRightPos;

    leftGuideModel.localRotation = vec3(0);
    rightGuideModel.localRotation = vec3(0);


    leftGuideModel.updateMatrix(true);
    rightGuideModel.updateMatrix(true);
    // =================

    // Reset squares animation variables
    squaresLaunchAnim.isAnimating = false;
    squaresLaunchAnim.animationTime = 0.0f;
    spawnGuideSquares();
    guideSquaresMovedDuringLaunch = false; // Reset the flag for the new pattern
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
    moveGuideSquares(); 
    playRandomImpactSound();

    poleAnim.startPos = springModels[poleAnim.currentSpringIndex].localPosition;
    poleAnim.endPos = springModels[poleAnim.movingRight ?
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

    // Render background first
    jr.beginBatchRender(backgroundModel, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(backgroundModel, isShadow);
    jr.endBatchRender(isShadow);

    // Render springs using models instead of boxes
    for (const auto& spring : springModels) {
        jr.beginBatchRender(spring, false, vec4(1.f), isShadow);
        jr.renderBatchWithOwnColor(spring, isShadow);
        jr.endBatchRender(isShadow);
    }

    // Render Pole MODEL
    jr.beginBatchRender(poleModel, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(poleModel, isShadow);
    jr.endBatchRender(isShadow);

    if (guidesActive) {
        jr.beginBatchRender(leftGuideModel, false, vec4(1.f), isShadow);
        jr.renderBatchWithOwnColor(leftGuideModel, isShadow);
        jr.endBatchRender(isShadow);

        jr.beginBatchRender(rightGuideModel, false, vec4(1.f), isShadow);
        jr.renderBatchWithOwnColor(rightGuideModel, isShadow);
        jr.endBatchRender(isShadow);
    }
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
    objects.push_back(&backgroundModel);
    for (auto& spring : springModels) {
        objects.push_back(&spring);
    }
    objects.push_back(&poleModel);
    objects.push_back(&inputIndicator);
    if (guidesActive) {
        objects.push_back(&leftGuideModel);
        objects.push_back(&rightGuideModel);
    }
    return objects;
}

void BuiltToScaleScene::triggerSpringBounce(int springIndex) {
    // Only animate the red spring (index 2) if input was successful
    if (springIndex == 2 && !poleAnim.inputSuccess) return;

    springAnims[springIndex].isAnimating = true;
    springAnims[springIndex].animationTime = 0.0f;
    springAnims[springIndex].baseHeight = springModels[springIndex].localPosition.y;


}

void BuiltToScaleScene::selectRandomPattern() {
    Pattern patterns[] = { Pattern::ThreeStep, Pattern::FiveStep, Pattern::EightStep };
    patternInfo.type = patterns[rand() % 3];

    // Randomly choose a tempo
    int randomTempoIndex = rand() % TEMPO_BPM.size();
    int selectedBPM = TEMPO_BPM[randomTempoIndex];
    BOUNCE_DURATION = 60.0f / static_cast<float>(selectedBPM); // Calculate bounce duration

    std::cout << "Selected BPM: " << selectedBPM << ", Bounce Duration: " << BOUNCE_DURATION << " seconds" << std::endl;



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
    springModels[2].localPosition.z = 0.0f;

    guideSquaresMovedDuringLaunch = false; // Reset the flag when a new pattern is selected
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

            audio.playSound("builttoscale_impactThrow");
        }

        // Animate spring retraction in Z direction (backwards)
        float t = patternInfo.retractAnimTime / RETRACT_DURATION;
        springModels[2].localPosition.z = -(t * 1.0f);
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
            springModels[2].localPosition.z = 0.0f; // Reset Z position instead of X

        }
        else {
            failPole();
        }
    }

    wasKPressed = isKPressed;
}

void BuiltToScaleScene::updateLaunchAnimation(float dt) {
    if (poleAnim.state != PoleState::Launching) return;
    if (!guideSquaresMovedDuringLaunch) {
        // We already moved the guide squares to overlap; now start the push animation
        moveGuideSquares();
        guideSquaresMovedDuringLaunch = true;
    }
    // Update the pole's launch animation time
    patternInfo.launchAnimTime += dt;

    poleModel.localPosition += vec3(0.0f, 0.0f, LAUNCH_SPEED * dt);
    poleModel.localRotation += vec3(0.0f, 0.0f, 2880.0f * dt);
    poleModel.updateMatrix(true);

    // Calculate when the pole reaches the squares
    if (!squaresLaunchAnim.isAnimating) {
        float poleStartZ = springModels[2].localPosition.z; // Starting Z position of the pole

        float squaresZ = leftGuideModel.localPosition.z; // Z position of the squares

        float distanceToSquares = squaresZ - poleStartZ;
        float timeToReachSquares = distanceToSquares / LAUNCH_SPEED;

        // Start the squares' animation when the pole reaches them
        if (patternInfo.launchAnimTime >= timeToReachSquares) {
            // Initialize squares animation
            squaresLaunchAnim.isAnimating = true;
            squaresLaunchAnim.animationTime = 0.0f;
            squaresLaunchAnim.totalAnimationDuration = LAUNCH_DURATION - patternInfo.launchAnimTime;

            // Record starting positions
            squaresLaunchAnim.startLeftPos = leftGuideModel.localPosition;
            squaresLaunchAnim.startRightPos = rightGuideModel.localPosition;

            // Set ending positions (move backward along Z-axis)
            float backwardDistance = 5.0f;
            squaresLaunchAnim.endLeftPos = squaresLaunchAnim.startLeftPos + vec3(0.0f, 0.0f, backwardDistance);
            squaresLaunchAnim.endRightPos = squaresLaunchAnim.startRightPos + vec3(0.0f, 0.0f, backwardDistance);
        }
    }

    // Update squares animation if it has started
    if (squaresLaunchAnim.isAnimating) {
        squaresLaunchAnim.animationTime += dt;

        float t = squaresLaunchAnim.animationTime / squaresLaunchAnim.totalAnimationDuration;
        if (t > 1.0f) t = 1.0f;

        // ==================
        leftGuideModel.localPosition = mix(squaresLaunchAnim.startLeftPos, squaresLaunchAnim.endLeftPos, t);
        rightGuideModel.localPosition = mix(squaresLaunchAnim.startRightPos, squaresLaunchAnim.endRightPos, t);

        leftGuideModel.updateMatrix(true);
        rightGuideModel.updateMatrix(true);
    }

    // Check if the launch animation is complete
    if (patternInfo.launchAnimTime >= LAUNCH_DURATION) {
        // Animation complete
        respawnPole();
        return;
    }
}



void BuiltToScaleScene::spawnGuideSquares() {
    // Position at fixed distance in front of red spring
    vec3 basePos = springModels[2].localPosition + vec3(0.0f, 1.0f, guideForwardOffset);


    // Calculate total movement needed for each square to overlap
    float totalMovementPerSquare = guideSquareWidth * patternInfo.totalSteps;
    // ==============
    leftGuideModel.localPosition = basePos + vec3(-totalMovementPerSquare, 0.0f, 0.0f);
    rightGuideModel.localPosition = basePos + vec3(totalMovementPerSquare, 0.0f, 0.0f);


    guidesActive = true;
}



void BuiltToScaleScene::moveGuideSquares() {
    if (!guidesActive) return;

    // Move by one cube width each step
    float stepSize = guideSquareWidth;
    // ===============
    leftGuideModel.localPosition.x += stepSize;
    rightGuideModel.localPosition.x -= stepSize;

    leftGuideModel.updateMatrix(true);
    rightGuideModel.updateMatrix(true);

}

void BuiltToScaleScene::playRandomImpactSound() {
    // Randomly select between "impact1" and "impact2"
    std::string impactSoundId = (rand() % 2 == 0) ? "builttoscale_impactA" : "builttoscale_impactB";

    // Play the selected impact sound
    audio.playSound(impactSoundId);
}
