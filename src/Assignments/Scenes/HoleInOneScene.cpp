#include "HoleInOneScene.h"
#include "AnimationObjectRenderer.h"
#include "imgui.h"
#include <random>

HoleInOneScene::HoleInOneScene()
    : currentState(GameState::Inactive)
    , currentBeat(0)
    , stateStartTime(0)
    , currentTime(0)
    , golfClubAngle(45.0f)  // Start angled back
    , swingInProgress(false)
    , swingProgress(0.0f) {
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

    // Initialize monkey
    monkey = AnimationObject(AnimationObjectType::box);
    monkey.localScale = vec3(0.8f);  // Slightly smaller than player
    monkey.color = vec4(0.6f, 0.4f, 0.2f, 1.0f);  // Brown

    // Initialize mandrill
    mandrill = AnimationObject(AnimationObjectType::box);
    mandrill.localScale = vec3(1.2f);  // Larger than player
    mandrill.color = vec4(0.3f);  // Dark gray

    // Initialize island
    island = AnimationObject(AnimationObjectType::box);
    island.localScale = vec3(3.0f, 0.5f, 2.0f);  // Wide platform
    island.color = vec4(0.2f, 0.8f, 0.2f, 1.0f);  // Green

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
}

//void HoleInOneScene::initializePositions() {
//    // Set up positions (all characters facing each other along X axis)
//    player.localPosition = vec3(0.0f, 0.0f, 0.0f);
//    monkey.localPosition = vec3(-4.0f, 0.0f, 0.0f);
//    mandrill.localPosition = vec3(-6.0f, 0.0f, 0.0f);
//    island.localPosition = vec3(6.0f, 0.0f, 0.0f);  // Far to the right of player
//
//    // Position indicators relative to player
//    playerIndicator.localPosition = player.localPosition + vec3(0.0f, 1.5f, 0.0f);
//    feedbackCube.localPosition = player.localPosition + vec3(-1.0f, 1.5f, 0.0f);
//
//    // Initialize golf club position (angled back, with pivot point away from player)
//    updateGolfClub(0.0f);
//}
void HoleInOneScene::initializePositions() {
    // Set up positions (along X axis)
    player.localPosition = vec3(0.0f, 0.0f, 0.0f);
    monkey.localPosition = vec3(4.0f, 0.0f, 0.0f);     // Monkey to right of player
    mandrill.localPosition = vec3(6.0f, 0.0f, 0.0f);   // Mandrill further right
    island.localPosition = vec3(0.0f, 0.0f, -10.0f);   // Island far to player's left

    // Position indicators relative to player
    playerIndicator.localPosition = player.localPosition + vec3(0.0f, 1.5f, 0.0f);
    feedbackCube.localPosition = player.localPosition + vec3(-1.0f, 1.5f, 0.0f);

    // Initialize golf club
    updateGolfClub(0.0f);
}



//vec3 HoleInOneScene::getThrowPosition(bool isMandrill) const {
//    // Position in front of the thrower
//    const AnimationObject& thrower = isMandrill ? mandrill : monkey;
//    return thrower.localPosition + vec3(BALL_OFFSET, 0.0f, 0.0f);
//}
vec3 HoleInOneScene::getThrowPosition(bool isMandrill) const {
    // Position in front of the thrower
    const AnimationObject& thrower = isMandrill ? mandrill : monkey;
    return thrower.localPosition + vec3(-BALL_OFFSET, 0.0f, 0.0f); // Offset towards player
}

//vec3 HoleInOneScene::getPlayerHitPosition() const {
//    // Position where the ball should be hit (in front of player)
//    return player.localPosition + vec3(-PLAYER_BALL_OFFSET, 0.0f, 0.0f);
//}
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
        // During prep, ball appears in front of monkey
        golfBall.localPosition = getThrowPosition(false);
        golfBall.visible = true;

        // Brighten monkey during prep
        monkey.color = vec4(0.6f + PREP_COLOR_INTENSITY,
            0.4f + PREP_COLOR_INTENSITY,
            0.2f + PREP_COLOR_INTENSITY, 1.0f);

        if (beatProgress >= 1.0f) {
            currentState = GameState::MonkeyThrow;
            stateStartTime = currentTime;
            ballStartPos = golfBall.localPosition;
            ballEndPos = getPlayerHitPosition();

            // Reset monkey color
            monkey.color = vec4(0.6f, 0.4f, 0.2f, 1.0f);
        }
        break;

    case GameState::MonkeyThrow:
        if (beatProgress >= 1.0f) {
            // Immediately transition to ball flight, with swing already completed
            currentState = GameState::BallFlight;
            stateStartTime = currentTime;
            ballStartPos = getPlayerHitPosition();
            ballEndPos = island.localPosition;
            swingInProgress = true;
            swingProgress = 0.0f;
        }
        break;

    case GameState::BallFlight:
        if (beatProgress >= 2.0f) {  // Takes 2 beats to reach island
            golfBall.visible = false;
            swingInProgress = false;

            // Reset for next pattern
            currentPattern = selectNextPattern();
            if (currentPattern.waitBeats == 0) {
                startNewPattern();
            }
            else {
                currentState = GameState::Inactive;
                stateStartTime = currentTime;
            }
        }
        break;

        // Mandrill states
    case GameState::MandrillPrep1:
        golfBall.localPosition = getThrowPosition(true);
        golfBall.visible = true;
        mandrill.color = vec4(0.3f + PREP_COLOR_INTENSITY);

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
            ballEndPos = island.localPosition;
            mandrill.color = vec4(0.3f);  // Reset color
        }
        break;

    case GameState::MandrillThrow:
        // For mandrill throw, the hit is instant with the throw
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
        }
        break;
    }

    // Update matrices for all objects
    player.updateMatrix(true);
    golfClub.updateMatrix(true);
    monkey.updateMatrix(true);
    mandrill.updateMatrix(true);
    island.updateMatrix(true);
    golfBall.updateMatrix(true);
    playerIndicator.updateMatrix(true);
    feedbackCube.updateMatrix(true);
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

//void HoleInOneScene::updateBallPosition(float t) {
//    switch (currentState) {
//    case GameState::MonkeyThrow:
//        golfBall.localPosition = calculateArcPosition(
//            ballStartPos, ballEndPos, MONKEY_THROW_HEIGHT, t);
//        break;
//
//    case GameState::BallFlight:
//        golfBall.localPosition = calculateArcPosition(
//            ballStartPos, ballEndPos, BALL_FLIGHT_HEIGHT, t * 0.5f); // t * 0.5f because it takes 2 beats
//        break;
//
//    case GameState::MandrillThrow:
//        // Straight line for mandrill throw
//        golfBall.localPosition = mix(ballStartPos, ballEndPos, t);
//        break;
//    }
//}
void HoleInOneScene::updateBallPosition(float t) {
    switch (currentState) {
    case GameState::MonkeyThrow:
        golfBall.localPosition = calculateArcPosition(
            ballStartPos, ballEndPos, MONKEY_THROW_HEIGHT, t);
        break;

    case GameState::MandrillThrow: {
        // Fast linear throw to player
        float adjustedT = t * 3.0f; // 3x faster throw
        if (adjustedT <= 1.0f) {
            // Straight line to player
            golfBall.localPosition = mix(ballStartPos, getPlayerHitPosition(), adjustedT);
            if (adjustedT >= 0.95f) { // Near end of throw, trigger swing
                swingInProgress = true;

                // Transition to ball flight
                currentState = GameState::BallFlight;
                stateStartTime = currentTime;
                ballStartPos = getPlayerHitPosition();
                ballEndPos = island.localPosition;
            }
        }
        break;
    }

    case GameState::BallFlight:
        // Same arc and timing for both monkey and mandrill hits
        golfBall.localPosition = calculateArcPosition(
            ballStartPos, ballEndPos, BALL_FLIGHT_HEIGHT, t * 0.5f); // t * 0.5f for 2-beat duration
        break;
    }
}

//void HoleInOneScene::updateGolfClub(float dt) {
//    // Calculate base position (pivot point should be away from player)
//    vec3 pivotPoint = player.localPosition + vec3(-PLAYER_BALL_OFFSET - GOLF_CLUB_LENGTH / 2, 0.0f, 0.0f);
//
//    if (currentState == GameState::MonkeyThrow) {
//        // Prepare for swing as ball approaches
//        float throwProgress = (currentTime - stateStartTime) / BEAT_DURATION;
//        if (throwProgress > 0.8f) {  // Start backswing near end of throw
//            golfClubAngle = 45.0f * (1.0f - (throwProgress - 0.8f) / 0.2f);
//        }
//    }
//    else if (currentState == GameState::BallFlight) {
//        // Follow through after hitting
//        float flightProgress = (currentTime - stateStartTime) / BEAT_DURATION;
//        if (flightProgress < 0.2f) {  // Quick follow through
//            golfClubAngle = -90.0f * flightProgress / 0.2f;
//        }
//        else {
//            // Slowly return to rest position
//            golfClubAngle = -90.0f + (45.0f + 90.0f) * (flightProgress - 0.2f) / 1.8f;
//        }
//    }
//    else if (currentState == GameState::MandrillThrow) {
//        // Instant swing for mandrill throw
//        float throwProgress = (currentTime - stateStartTime) / BEAT_DURATION;
//        if (throwProgress < 0.1f) {
//            golfClubAngle = -90.0f * throwProgress / 0.1f;
//        }
//        else {
//            golfClubAngle = -90.0f + (45.0f + 90.0f) * (throwProgress - 0.1f) / 0.9f;
//        }
//    }
//    else {
//        // Return to rest position
//        golfClubAngle = glm::mix(golfClubAngle, 45.0f, dt * 5.0f);
//    }
//
//    // Update golf club position and rotation
//    golfClub.localPosition = pivotPoint;
//    golfClub.localRotation = vec3(0.0f, 0.0f, golfClubAngle);
//}
//void HoleInOneScene::updateGolfClub(float dt) {
//    // The grip (top) should be closer to player, club head further away
//    vec3 gripPos = player.localPosition + vec3(0.2f, 0.5f, 0.0f);  // Grip position near player
//    golfClub.localPosition = gripPos;
//
//    // Rest position has club head away from player at roughly 45 degrees
//    if (currentState == GameState::MonkeyThrow || currentState == GameState::MandrillThrow) {
//        float throwProgress = (currentTime - stateStartTime) / BEAT_DURATION;
//
//        if (throwProgress > 0.8f) {  // Start backswing as ball approaches
//            // Lift club up and back for backswing
//            float backswingT = (throwProgress - 0.8f) / 0.2f;
//            golfClub.localRotation = vec3(0.0f, -90.0f * backswingT, 45.0f);  // Rotate back around Y-axis
//        }
//    }
//    else if (currentState == GameState::BallFlight) {
//        // Forward swing and follow through
//        float swingProgress = (currentTime - stateStartTime) / (BEAT_DURATION * 0.2f);
//        if (swingProgress < 1.0f) {
//            float swingT = std::min(swingProgress, 1.0f);
//            // Swing through from back position to follow through
//            golfClub.localRotation = vec3(0.0f, -90.0f + (180.0f * swingT), 45.0f);
//        }
//        else {
//            // Return to rest
//            golfClub.localRotation = vec3(0.0f, 0.0f, 45.0f);
//        }
//    }
//    else {
//        // Rest position
//        golfClub.localRotation = vec3(0.0f, 0.0f, 45.0f);
//    }
//}
void HoleInOneScene::updateGolfClub(float dt) {
    // Club handle at player's hands (adjust if needed)
    vec3 playerHandPos = player.localPosition;
    golfClub.localPosition = playerHandPos;

    // Pivot around handle
    golfClub.localPivot = vec3(-GOLF_CLUB_LENGTH * 0.5f, 0.0f, 0.0f);

    float phase = (currentTime - stateStartTime) / BEAT_DURATION;

    // Start angle at rest is 120°, as requested
    float angleY = 120.0f;

    switch (currentState) {
    case GameState::MonkeyThrow:
        // MonkeyThrow: from phase=0 to phase=1
        // Increase angle from 120° at start to 180° at impact
        if (phase < 0.0f) phase = 0.0f;
        if (phase > 1.0f) phase = 1.0f;
        // angleY = 120° + (180° - 120°)*phase = 120° + 60°*phase
        // at phase=0: angleY=120°, at phase=1: angleY=180°
        angleY = 120.0f + 60.0f * phase;
        break;

    case GameState::BallFlight:
        // BallFlight: let's say it lasts 2 beats
        // We continue from angleY=180° at start (phase=0) to angleY=220° at phase=2
        if (phase < 0.0f) phase = 0.0f;
        if (phase > 2.0f) phase = 2.0f;
        // angleY = 180° + (220° - 180°)*(phase/2.0) = 180° + 40°*(phase/2)
        // at phase=0: angleY=180°, at phase=2: angleY=220°
        angleY = 180.0f + 40.0f * (phase / 2.0f);
        break;

    case GameState::MandrillThrow:
        // Similar to MonkeyThrow, but let's say we go from 120° at start to 150° at end
        if (phase < 0.0f) phase = 0.0f;
        if (phase > 1.0f) phase = 1.0f;
        // angleY = 120° + (150° - 120°)*phase
        // at phase=0: angleY=120°, at phase=1: angleY=150°
        angleY = 120.0f + 30.0f * phase;
        break;

    default:
        // Not in a throwing state, smoothly return to rest angle = 120°
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
    jr.renderBatchWithOwnColor(monkey, isShadow);
    jr.renderBatchWithOwnColor(mandrill, isShadow);
    jr.renderBatchWithOwnColor(island, isShadow);

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
    objects.push_back(&player);
    objects.push_back(&golfClub);
    objects.push_back(&monkey);
    objects.push_back(&mandrill);
    objects.push_back(&island);
    if (golfBall.visible) {
        objects.push_back(&golfBall);
    }
    objects.push_back(&playerIndicator);
    objects.push_back(&feedbackCube);
    return objects;
}