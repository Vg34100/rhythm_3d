#include "BuiltToScaleScene.h"
#include "AnimationObjectRenderer.h"
#include "Input.h"
#include "imgui.h"
#include <iostream>

BuiltToScaleScene::BuiltToScaleScene()
    : audio(AudioSystem::get()) {


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
    guideSquaresMovedDuringLaunch = false;
    squaresLaunchAnim.isAnimating = false;
    squaresLaunchAnim.animationTime = 0.0f;
}

BuiltToScaleScene::~BuiltToScaleScene() {}

void BuiltToScaleScene::init() {
    initSprings();
    initPole();
    initInputIndicator();

    // Initialize guide squares
    leftGuideSquare = AnimationObject(AnimationObjectType::box);
    leftGuideSquare.localScale = vec3(guideSquareWidth);
    leftGuideSquare.color = vec4(0.2f, 0.5f, 1.0f, 0.7f);

    rightGuideSquare = AnimationObject(AnimationObjectType::box);
    rightGuideSquare.localScale = vec3(guideSquareWidth);
    rightGuideSquare.color = vec4(0.2f, 0.5f, 1.0f, 0.7f);

    audio.loadScene("builttoscale");

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

    // Add these lines where other matrices are updated
    if (guidesActive) {
        leftGuideSquare.updateMatrix(true);
        rightGuideSquare.updateMatrix(true);
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
            moveGuideSquares();
            playRandomImpactSound();
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
    // Reset guides for new pattern

        // Reset guide squares positions
    leftGuideSquare.localPosition = squaresLaunchAnim.startLeftPos;
    rightGuideSquare.localPosition = squaresLaunchAnim.startRightPos;
   
    leftGuideSquare.localRotation = vec3(0);
    rightGuideSquare.localRotation = vec3(0);

    
    leftGuideSquare.updateMatrix(true);
    rightGuideSquare.updateMatrix(true);

    // Deactivate guides
    //guidesActive = false;

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
    moveGuideSquares();  // Add this line
    playRandomImpactSound();

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

    if (guidesActive) {
        jr.beginBatchRender(leftGuideSquare.shapeType, false, vec4(1.f), isShadow);
        jr.renderBatchWithOwnColor(leftGuideSquare, isShadow);
        jr.renderBatchWithOwnColor(rightGuideSquare, isShadow);
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
    for (auto& spring : springs) {
        objects.push_back(&spring);
    }
    objects.push_back(&pole);
    objects.push_back(&inputIndicator);
    if (guidesActive) {
        objects.push_back(&leftGuideSquare);
        objects.push_back(&rightGuideSquare);
    }
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


    //// Stop any currently playing sound
    //if (!currentSoundId.empty()) {
    //    audio.stopSound(currentSoundId);
    //}

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
        //currentSoundId = ""; // No sound available for ThreeStep
        break;
    case Pattern::FiveStep:
        patternInfo.totalSteps = 5;
        poleAnim.currentSpringIndex = 0;
        poleAnim.movingRight = true;
        //currentSoundId = "builttoscale_patternB";
        break;
    case Pattern::EightStep:
        patternInfo.totalSteps = 8;
        poleAnim.currentSpringIndex = 3;
        poleAnim.movingRight = false;
        //currentSoundId = "builttoscale_patternA";
        break;
    }

    patternInfo.stepsRemaining = patternInfo.totalSteps;
    patternInfo.springRetracted = false;
    patternInfo.retractAnimTime = 0.0f;
    patternInfo.launchAnimTime = 0.0f;
    patternInfo.awaitingLaunchInput = false;

    // Reset spring position
    springs[2].localPosition.z = 0.0f;
    guideSquaresMovedDuringLaunch = false; // Reset the flag when a new pattern is selected

    //// Play the selected sound
    //if (!currentSoundId.empty()) {
    //    audio.playSound(currentSoundId);
    //}
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
    if (!guideSquaresMovedDuringLaunch) {
        // We already moved the guide squares to overlap; now start the push animation
        moveGuideSquares();
        guideSquaresMovedDuringLaunch = true;
    }
    // Update the pole's launch animation time
    patternInfo.launchAnimTime += dt;

    // Move the pole forward
    pole.localPosition += vec3(0.0f, 0.0f, LAUNCH_SPEED * dt);
    pole.localRotation += vec3(0.0f, 0.0f, 2880.0f * dt);
    pole.updateMatrix(true);

    // Calculate when the pole reaches the squares
    if (!squaresLaunchAnim.isAnimating) {
        float poleStartZ = springs[2].localPosition.z; // Starting Z position of the pole
        float squaresZ = leftGuideSquare.localPosition.z; // Z position of the squares
        float distanceToSquares = squaresZ - poleStartZ;
        float timeToReachSquares = distanceToSquares / LAUNCH_SPEED;

        // Start the squares' animation when the pole reaches them
        if (patternInfo.launchAnimTime >= timeToReachSquares) {
            // Initialize squares animation
            squaresLaunchAnim.isAnimating = true;
            squaresLaunchAnim.animationTime = 0.0f;
            squaresLaunchAnim.totalAnimationDuration = LAUNCH_DURATION - patternInfo.launchAnimTime;

            // Record starting positions
            squaresLaunchAnim.startLeftPos = leftGuideSquare.localPosition;
            squaresLaunchAnim.startRightPos = rightGuideSquare.localPosition;

            // Set ending positions (move backward along Z-axis)
            float backwardDistance = 5.0f; // Adjust as desired
            squaresLaunchAnim.endLeftPos = squaresLaunchAnim.startLeftPos + vec3(0.0f, 0.0f, backwardDistance);
            squaresLaunchAnim.endRightPos = squaresLaunchAnim.startRightPos + vec3(0.0f, 0.0f, backwardDistance);
        }
    }

    // Update squares animation if it has started
    if (squaresLaunchAnim.isAnimating) {
        squaresLaunchAnim.animationTime += dt;

        float t = squaresLaunchAnim.animationTime / squaresLaunchAnim.totalAnimationDuration;
        if (t > 1.0f) t = 1.0f;

        // Interpolate positions
        leftGuideSquare.localPosition = mix(squaresLaunchAnim.startLeftPos, squaresLaunchAnim.endLeftPos, t);
        rightGuideSquare.localPosition = mix(squaresLaunchAnim.startRightPos, squaresLaunchAnim.endRightPos, t);

        leftGuideSquare.updateMatrix(true);
        rightGuideSquare.updateMatrix(true);
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
    vec3 basePos = springs[2].localPosition + vec3(0.0f, 1.0f, guideForwardOffset);

    // Calculate total movement needed for each square to overlap
    float totalMovementPerSquare = guideSquareWidth * patternInfo.totalSteps;

    // Set initial positions based on total movement
    leftGuideSquare.localPosition = basePos + vec3(-totalMovementPerSquare, 0.0f, 0.0f);
    rightGuideSquare.localPosition = basePos + vec3(totalMovementPerSquare, 0.0f, 0.0f);

    guidesActive = true;
}



void BuiltToScaleScene::moveGuideSquares() {
    if (!guidesActive) return;

    // Move by one cube width each step
    float stepSize = guideSquareWidth;

    leftGuideSquare.localPosition.x += stepSize;
    rightGuideSquare.localPosition.x -= stepSize;

    leftGuideSquare.updateMatrix(true);
    rightGuideSquare.updateMatrix(true);
}

void BuiltToScaleScene::playRandomImpactSound() {
    // Randomly select between "impact1" and "impact2"
    std::string impactSoundId = (rand() % 2 == 0) ? "builttoscale_impactA" : "builttoscale_impactB";

    // Play the selected impact sound
    audio.playSound(impactSoundId);
}
