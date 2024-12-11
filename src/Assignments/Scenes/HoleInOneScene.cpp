#include "HoleInOneScene.h"
#include "AnimationObjectRenderer.h"
#include "imgui.h"
#include "Input.h"
#include <random>
#include <Textures.h>

HoleInOneScene::HoleInOneScene()
    : currentState(GameState::Inactive)
    , currentBeat(0)
    , stateStartTime(0)
    , currentTime(0)
    , golfClubAngle(45.0f)
    , swingInProgress(false)
    , swingProgress(0.0f)
    , hasInputThisTurn(false)
    , audio(AudioSystem::get()) {
}

HoleInOneScene::~HoleInOneScene() {}

void HoleInOneScene::init() {
    // Initialize player
    player = AnimationObject(AnimationObjectType::box);
    player.localScale = vec3(1.0f);
    player.color = vec4(1.0f);  // White

    // Initialize golf club (angled relative to player)
    golfClub = AnimationObject(AnimationObjectType::box);
    golfClub.localScale = vec3(GOLF_CLUB_LENGTH, GOLF_CLUB_THICKNESS, GOLF_CLUB_THICKNESS);
    golfClub.color = vec4(0.9f);  // Light gray

    // ==========================
    backgroundModel = AnimationObject(AnimationObjectType::model);
    backgroundModel.meshName = "../assets/models/holeinone/HIO_Background.obj";

    // Create and register texture
    auto backgroundTexture = std::make_shared<Texture>(
        "../assets/models/holeinone/HIO_BackgroundTexture.png");
    TextureRegistry::addTexture(backgroundTexture);
    backgroundModel.texture = reinterpret_cast<void*>(backgroundTexture->id);

    // Set up model properties
    backgroundModel.localScale = vec3(1.0f);
    backgroundModel.color = vec4(1.0f); // White to show texture properly
    backgroundModel.localPosition = vec3(0.0f); // Center position
    backgroundModel.updateMatrix(true);
    // ==================================
        // Initialize mandrill model
    monkeyModel = AnimationObject(AnimationObjectType::model);
    monkeyModel.meshName = "../assets/models/holeinone/HIO_Monkey.obj";

    // Create and register texture
    auto monkeyTexture = std::make_shared<Texture>(
        "../assets/models/holeinone/HIO_MonkeyTexture.png");


    TextureRegistry::addTexture(monkeyTexture);
    monkeyModel.texture = reinterpret_cast<void*>(monkeyTexture->id);

    // Set up model properties
    monkeyModel.localScale = vec3(1.0f);
    monkeyModel.color = vec4(1.0f); // White to show texture properly


    // ==================================

    // ==================================
        // Initialize mandrill model
    mandrillModel = AnimationObject(AnimationObjectType::model);
    mandrillModel.meshName = "../assets/models/holeinone/HIO_Mandrill.obj";

    // Create and register texture
    auto mandrillTexture = std::make_shared<Texture>(
        "../assets/models/holeinone/HIO_MandrillTexture.jpg");


    TextureRegistry::addTexture(mandrillTexture);
    mandrillModel.texture = reinterpret_cast<void*>(mandrillTexture->id);

    // Set up model properties
    mandrillModel.localScale = vec3(1.0f);
    mandrillModel.color = vec4(1.0f); // White to show texture properly


    // ==================================

    // ===============================================

    // Initialize island model
    islandModel = AnimationObject(AnimationObjectType::model);
    islandModel.meshName = "../assets/models/holeinone/HIO_Island.obj";
    
    // Create and register texture
    auto islandTexture = std::make_shared<Texture>(
        "../assets/models/holeinone/HIO_IslandTexture.png");
    TextureRegistry::addTexture(islandTexture);
    islandModel.texture = reinterpret_cast<void*>(islandTexture->id);
    
    // Set up model properties
    islandModel.localScale = vec3(1.0f);
// ===============================================

    // Initialize golf ball
    golfBall = AnimationObject(AnimationObjectType::box);
    golfBall.localScale = vec3(0.2f);  // Small ball
    golfBall.color = vec4(1.0f);  // White
    golfBall.visible = false;  // Start invisible

    // Initialize player indicator
    playerIndicator = AnimationObject(AnimationObjectType::box);
    playerIndicator.localScale = vec3(0.4f);
    playerIndicator.color = vec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow

    // Initialize feedback cube
    feedbackCube = AnimationObject(AnimationObjectType::box);
    feedbackCube.localScale = vec3(0.3f);
    feedbackCube.color = vec4(0.5f);  // Gray

    initializePositions();
    currentPattern = selectNextPattern();
    audio.loadScene("holeinone");
}

void HoleInOneScene::initializePositions() {
    // Set up positions (along X axis)
    player.localPosition = vec3(0.0f, 0.0f, 0.0f);
    // ========================
    monkeyModel.localPosition = vec3(4.0f, 0.0f, 0.0f);
    monkeyModel.updateMatrix(true);
    // =====================
    mandrillModel.localPosition = vec3(6.0f, 0.0f, 0.0f);
    mandrillModel.updateMatrix(true);
    // ====================

    // =====================
    islandModel.localPosition = vec3(0.0f, 0.0f, -50.0f);
    islandModel.updateMatrix(true);
    //  ====================

    // Position indicators relative to player
    playerIndicator.localPosition = player.localPosition + vec3(0.0f, 1.5f, 0.0f);
    feedbackCube.localPosition = player.localPosition + vec3(-1.0f, 1.5f, 0.0f);

    // Initialize golf club
    updateGolfClub(0.0f);
}

vec3 HoleInOneScene::getThrowPosition(bool isMandrill) const {
    // Position in front of the thrower
    const AnimationObject& thrower = isMandrill ? mandrillModel : monkeyModel;
    return thrower.localPosition + vec3(-BALL_OFFSET, 0.0f, 0.0f); // Offset towards player
}


vec3 HoleInOneScene::getPlayerHitPosition() const {
    // Position where the ball should be hit (to right of player)
    return player.localPosition + vec3(PLAYER_BALL_OFFSET, 0.0f, 0.0f);
}

void HoleInOneScene::update(double now, float dt) {
    currentTime += dt;
    float beatProgress = (currentTime - stateStartTime) / BEAT_DURATION;

    // Update golf ball position if visible
    if (golfBall.visible) {
        updateBallPosition(beatProgress);
    }

    // Update golf club animation
    updateGolfClub(dt);

    // State machine for rhythm game
    switch (currentState) {
    case GameState::Inactive:
        if (beatProgress >= currentPattern.waitBeats) {
            startNewPattern();
        }
        break;

    case GameState::MonkeyPrep:
        // Play sound immediately when prep starts
        if (!soundPlayed) {
            audio.playSound("holeinone_monkeythrow");
            soundPlayed = true;
        }

        // During prep, ball appears in front of monkey
        golfBall.localPosition = getThrowPosition(false);
        golfBall.visible = true;

        if (beatProgress >= 1.0f) {
            currentState = GameState::MonkeyThrow;
            stateStartTime = currentTime;
            ballStartPos = golfBall.localPosition;
            ballEndPos = getPlayerHitPosition();
        }
        break;

    case GameState::MonkeyThrow:
        soundPlayed = false;
    case GameState::BallFlight:
        handlePlayerInput();
        if (beatProgress >= 2.0f) {
            golfBall.visible = false;
            swingInProgress = false;

            currentPattern = selectNextPattern();
            if (currentPattern.waitBeats == 0) {
                startNewPattern();
            }
            else {
                currentState = GameState::Inactive;
                stateStartTime = currentTime;
            }
            hasInputThisTurn = false;  // Reset input flag
        }
        break;

    case GameState::MandrillPrep1:

        if (!soundPlayed) {  // Only play at the very start
            audio.playSound("holeinone_mandrillthrow");
            soundPlayed = true;
        }


        golfBall.localPosition = getThrowPosition(true);
        golfBall.visible = true;

        if (beatProgress >= 1.0f) {
            currentState = GameState::MandrillPrep2;
            stateStartTime = currentTime;
        }
        break;

    case GameState::MandrillPrep2:
        if (beatProgress >= 1.0f) {
            currentState = GameState::MandrillPrep3;
            stateStartTime = currentTime;
        }
        break;

    case GameState::MandrillPrep3:
        if (beatProgress >= 1.0f) {
            currentState = GameState::MandrillThrow;
            stateStartTime = currentTime;
            ballStartPos = golfBall.localPosition;
            ballEndPos = islandModel.localPosition;
        }
        break;

    case GameState::MandrillThrow:
        soundPlayed = false;

        handlePlayerInput();
        if (beatProgress >= 1.0f) {
            golfBall.visible = false;
            swingInProgress = false;

            currentPattern = selectNextPattern();
            if (currentPattern.waitBeats == 0) {
                startNewPattern();
            }
            else {
                currentState = GameState::Inactive;
                stateStartTime = currentTime;
            }
            hasInputThisTurn = false;  // Reset input flag
        }
        break;
    }

    // Update matrices for all objects
    backgroundModel.updateMatrix(true);  // Add this line first
    player.updateMatrix(true);
    golfClub.updateMatrix(true);
    // =================
    monkeyModel.updateMatrix(true);
    // =============
    // =================
    mandrillModel.updateMatrix(true);
    // ===========
    islandModel.updateMatrix(true);
    golfBall.updateMatrix(true);
    playerIndicator.updateMatrix(true);
    feedbackCube.updateMatrix(true);
}

void HoleInOneScene::handlePlayerInput() {
    if (hasInputThisTurn) return;

    auto& input = Input::get();
    bool isLPressed = input.current.keyStates[GLFW_KEY_L] == GLFW_PRESS;
    static bool wasLPressed = false;

    if (isLPressed && !wasLPressed) {
        float stateTime = currentTime - stateStartTime;

        // Allow input in both throw states and early BallFlight state
        if (currentState == GameState::MonkeyThrow ||
            currentState == GameState::MandrillThrow ||
            (currentState == GameState::BallFlight && stateTime <= GOOD_WINDOW * 1.5f)) {

            hasInputThisTurn = true;
            TimingResult result = checkTiming(stateTime);
            validHit = (result == TimingResult::Perfect || result == TimingResult::Good);
            updateFeedbackCube(result);
        }
    }

    wasLPressed = isLPressed;
}

void HoleInOneScene::updateFeedbackCube(TimingResult result) {
    switch (result) {
    case TimingResult::Perfect:
        feedbackCube.color = vec4(1.0f, 0.84f, 0.0f, 1.0f);  // Gold
        feedbackCube.localScale = vec3(0.5f);  // Expanded size
        audio.playSound("holeinone_ballflying");
        break;
    case TimingResult::Good:
        feedbackCube.color = vec4(0.0f, 1.0f, 0.0f, 1.0f);   // Green
        feedbackCube.localScale = vec3(0.4f);  // Slightly expanded
        audio.playSound("holeinone_ballflying");
        break;
    case TimingResult::Bad:
    case TimingResult::Miss:
        feedbackCube.color = vec4(1.0f, 0.0f, 0.0f, 1.0f);   // Red
        feedbackCube.localScale = vec3(0.3f);  // Normal size
        break;
    }
}

HoleInOneScene::TimingResult HoleInOneScene::checkTiming(float stateTime) {
    float ballArrivalTime = 0.0f;

    if (currentState == GameState::MonkeyThrow) {
        ballArrivalTime = BEAT_DURATION;  // Ball arrives after one beat
    }
    else if (currentState == GameState::MandrillThrow) {
        ballArrivalTime = BEAT_DURATION * 0.32f;  // Adjusted for faster mandrill throw
    }
    else if (currentState == GameState::BallFlight) {
        // If we just entered BallFlight state, adjust the timing window
        ballArrivalTime = 0.0f;  // Since we just entered this state
    }
    else {
        return TimingResult::Miss;  // Invalid state for hitting
    }

    float timingDiff = abs(stateTime - ballArrivalTime);

    // More forgiving windows for BallFlight state (slightly late hits)
    if (currentState == GameState::BallFlight) {
        if (timingDiff <= GOOD_WINDOW * 1.5f) return TimingResult::Good;  // A bit late but still good
        return TimingResult::Miss;
    }

    // Normal timing windows for other states
    if (timingDiff <= PERFECT_WINDOW) return TimingResult::Perfect;
    if (timingDiff <= GOOD_WINDOW) return TimingResult::Good;
    if (timingDiff <= BAD_WINDOW) return TimingResult::Bad;
    return TimingResult::Miss;
}


void HoleInOneScene::startNewPattern() {
    if (currentPattern.isMandrill) {
        currentState = GameState::MandrillPrep1;
    }
    else {
        currentState = GameState::MonkeyPrep;
    }
    stateStartTime = currentTime;
}

vec3 HoleInOneScene::calculateArcPosition(const vec3& start, const vec3& end, float height, float t) {
    vec3 mid = (start + end) * 0.5f;
    mid.y += height;

    float oneMinusT = 1.0f - t;
    return oneMinusT * oneMinusT * start +
        2.0f * oneMinusT * t * mid +
        t * t * end;
}

void HoleInOneScene::updateBallPosition(float t) {
    switch (currentState) {
    case GameState::MonkeyThrow:
        golfBall.localPosition = calculateArcPosition(
            ballStartPos, ballEndPos, MONKEY_THROW_HEIGHT, t);

        if (t >= 0.95f) {  // When ball is near player
            currentState = GameState::BallFlight;
            stateStartTime = currentTime;
            ballStartPos = getPlayerHitPosition();

            swingInProgress = true;
        }
        break;

    case GameState::MandrillThrow: {
        float adjustedT = t * 3.0f;
        if (adjustedT <= 1.0f) {
            golfBall.localPosition = mix(ballStartPos, getPlayerHitPosition(), adjustedT);

            if (adjustedT >= 0.95f) {
                currentState = GameState::BallFlight;
                stateStartTime = currentTime;
                ballStartPos = getPlayerHitPosition();

                swingInProgress = true;
            }
        }
        break;
    }

    case GameState::BallFlight: {
        // Only determine trajectory if we haven't already
        static bool trajectorySet = false;
        if (!trajectorySet) {
            if (hasInputThisTurn && validHit) {
                ballEndPos = islandModel.localPosition;
            }
            else if (currentTime - stateStartTime > GOOD_WINDOW * 1.5f) {
                // If we're past the late hit window and no valid hit recorded
                if (!hasInputThisTurn) {
                    // No input - straight down
                    ballEndPos = vec3(getPlayerHitPosition().x, -2.0f, 0.0f);
                    feedbackCube.color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
                }
                else if (!validHit) {
                    ballEndPos = vec3(
                        (islandModel.localPosition.x + getPlayerHitPosition().x) * 0.5f,
                        -1.0f,
                        0.0f
                    );
                }
                trajectorySet = true;
            }
            // Otherwise, keep waiting for potential late input
        }

        // Calculate position along trajectory
        float arcHeight = (ballEndPos == islandModel.localPosition) ? BALL_FLIGHT_HEIGHT : BALL_FLIGHT_HEIGHT * 0.5f;

        golfBall.localPosition = calculateArcPosition(
            ballStartPos, ballEndPos, arcHeight, t * 0.5f);

        // Reset trajectory flag when state changes
        if (t >= 1.0f) {
            trajectorySet = false;
        }
        break;
    }
    }
}

void HoleInOneScene::updateGolfClub(float dt) {
    // Club handle at player's hands (adjust if needed)
    vec3 playerHandPos = player.localPosition;
    golfClub.localPosition = playerHandPos;

    // Pivot around handle
    golfClub.localPivot = vec3(-GOLF_CLUB_LENGTH * 0.5f, 0.0f, 0.0f);

    float phase = (currentTime - stateStartTime) / BEAT_DURATION;

    float angleY = 120.0f;

    switch (currentState) {
    case GameState::MonkeyThrow:
        if (phase < 0.0f) phase = 0.0f;
        if (phase > 1.0f) phase = 1.0f;
        angleY = 120.0f + 60.0f * phase;
        break;

    case GameState::BallFlight:
        if (phase < 0.0f) phase = 0.0f;
        if (phase > 2.0f) phase = 2.0f;
        angleY = 180.0f + 40.0f * (phase / 2.0f);
        break;

    case GameState::MandrillThrow:
        if (phase < 0.0f) phase = 0.0f;
        if (phase > 1.0f) phase = 1.0f;
        angleY = 120.0f + 30.0f * phase;
        break;

    default:
        angleY = glm::mix(angleY, 120.0f, dt * 5.0f);
        break;
    }

    golfClub.localRotation = vec3(0.0f, angleY, 0.0f);
    golfClub.updateMatrix(true);
}


HoleInOneScene::ThrowPattern HoleInOneScene::selectNextPattern() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    ThrowPattern pattern;

    // 20% chance for mandrill
    pattern.isMandrill = (dis(gen) < 0.2f);

    // For monkey patterns, randomly choose wait beats
    if (!pattern.isMandrill) {
        float r = dis(gen);
        if (r < 0.4f) pattern.waitBeats = 0;      // 40% chance for immediate next pattern
        else if (r < 0.7f) pattern.waitBeats = 1; // 30% chance for 1 beat wait
        else pattern.waitBeats = 2;               // 30% chance for 2 beat wait
    }
    else {
        pattern.waitBeats = 1; // Mandrill always waits 1 beat
    }

    return pattern;
}

void HoleInOneScene::render(const mat4& projection, const mat4& view, bool isShadow) {
    auto& jr = AnimationObjectRenderer::get();

    // ==================================
    // Render model objects first
    // Render background first
    jr.beginBatchRender(backgroundModel, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(backgroundModel, isShadow);
    jr.endBatchRender(isShadow);

    jr.beginBatchRender(monkeyModel, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(monkeyModel, isShadow);
    jr.endBatchRender(isShadow);
        
    jr.beginBatchRender(mandrillModel, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(mandrillModel, isShadow);
    jr.endBatchRender(isShadow);

    jr.beginBatchRender(islandModel, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(islandModel, isShadow);
    jr.endBatchRender(isShadow);
    //==================================

    // Render all box-type objects
    jr.beginBatchRender(AnimationObjectType::box, false, vec4(1.f), isShadow);

    // Set correct culling based on shadow state
    if (!isShadow) {
        glDisable(GL_CULL_FACE);
    }
    else {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    }

    // Render main characters
    jr.renderBatchWithOwnColor(player, isShadow);
    jr.renderBatchWithOwnColor(golfClub, isShadow);

    // Only render ball if visible
    if (golfBall.visible) {
        jr.renderBatchWithOwnColor(golfBall, isShadow);
    }

    // Render UI elements
    jr.renderBatchWithOwnColor(playerIndicator, isShadow);
    jr.renderBatchWithOwnColor(feedbackCube, isShadow);

    jr.endBatchRender(isShadow);
}

void HoleInOneScene::renderUI() {
    if (ImGui::CollapsingHeader("Hole in One Debug")) {
        // Game state info
        const char* stateNames[] = {
            "Inactive", "MonkeyPrep", "MonkeyThrow", "BallFlight",
            "MandrillPrep1", "MandrillPrep2", "MandrillPrep3", "MandrillThrow"
        };
        ImGui::Text("Current State: %s", stateNames[static_cast<int>(currentState)]);

        // Beat timing info
        float beatProgress = (currentTime - stateStartTime) / BEAT_DURATION;
        ImGui::Text("Beat Progress: %.2f", beatProgress);
        ImGui::Text("Current Pattern: %s", currentPattern.isMandrill ? "Mandrill" : "Monkey");
        ImGui::Text("Wait Beats: %d", currentPattern.waitBeats);

        // Animation debug
        ImGui::Text("Golf Club Angle: %.1f", golfClubAngle);
        if (golfBall.visible) {
            ImGui::Text("Ball Position: (%.1f, %.1f, %.1f)",
                golfBall.localPosition.x,
                golfBall.localPosition.y,
                golfBall.localPosition.z);
        }

        // Add visual tuning sliders for debugging
        ImGui::SliderFloat("Golf Club Length", (float*)&GOLF_CLUB_LENGTH, 0.5f, 2.0f);
        ImGui::SliderFloat("Ball Offset", (float*)&BALL_OFFSET, 0.2f, 1.0f);
        ImGui::SliderFloat("Player Ball Offset", (float*)&PLAYER_BALL_OFFSET, 0.4f, 1.5f);
    }
}

ptr_vector<AnimationObject> HoleInOneScene::getObjects() {
    ptr_vector<AnimationObject> objects;
    objects.push_back(&backgroundModel); // Add background first

    objects.push_back(&player);
    objects.push_back(&golfClub);
    // ===============================
    objects.push_back(&monkeyModel); 
    objects.push_back(&mandrillModel); 
    objects.push_back(&islandModel); 
    // ===============================
    if (golfBall.visible) {
        objects.push_back(&golfBall);
    }
    objects.push_back(&playerIndicator);
    objects.push_back(&feedbackCube);
    return objects;
}