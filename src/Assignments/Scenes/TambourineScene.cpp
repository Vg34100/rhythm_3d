#include "TambourineScene.h"
#include "AnimationObjectRenderer.h"
#include "imgui.h"
#include "Input.h"
#include <iostream>
#include <Textures.h>
#include "Lighting.h"
#include "Application.h"
#include "Renderer.h"

// Implementation
TambourineScene::TambourineScene()
    : currentState(GameState::MonkeyTurn)
    , currentNoteIndex(0)
    , sequenceStartTime(0)
    , currentTime(0)
    , introStartTime(0)  // Initialize new variable
    , lastShakeState(false)
    , lastClapState(false)
    , showFeedbackCube(true)
    , audio(AudioSystem::get()) {
    monkeyAnim = { false, false, 0.0f };
    playerAnim = { false, false, 0.0f };
}

void TambourineScene::init() {
    //// Initialize all visual objects immediately
    //monkey = AnimationObject(AnimationObjectType::box);

    auto& lighting = GPU::Lighting::get();

    // Main spotlight from above (light 0)
    lighting.lights[0].position = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    lighting.lights[0].direction = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    lighting.lights[0].diffuse = vec4(10.5f, 10.5f, 10.5f, 1.0f);

    //// Front fill light (light 1)
    //lighting.lights[1].position = vec4(0.0f, 2.0f, 5.0f, 0.0f);
    //lighting.lights[1].direction = vec4(0.0f, -0.5f, -1.0f, 0.0f);
    //lighting.lights[1].diffuse = vec4(2.4f, 2.4f, 2.5f, 1.0f);

    //// Side rim light (light 2)
    //lighting.lights[2].position = vec4(-4.0f, 3.0f, 0.0f, 0.0f);
    //lighting.lights[2].direction = vec4(1.0f, -0.2f, 0.0f, 0.0f);
    //lighting.lights[2].diffuse = vec4(3.3f, 3.3f, 3.4f, 1.0f);

    lighting.update();

    //// Set initial camera position
    //auto& renderer = Application::get().renderer;
    //renderer->camera.Position = vec3(-100.0f, 0.0f, 0.0f);  // Higher and further back
    ////renderer->camera.Lookat = vec3(3.0f, 0.0f, -0.5f);
    ////renderer->camera.FOV = 45.0f;  // Tighter FOV for better view
    //renderer->camera.update();  // Important - need to call update after changing camera properties




    //monkey.localPosition = vec3(-2.0f, 0.0f, 0.0f);
    //monkey.localScale = vec3(1.0f);
    //monkey.color = vec4(0.6f, 0.4f, 0.2f, 1.0f);
    //monkey.updateMatrix(true);  // Important: Update matrices immediately

    // ==========================================
    // Add new model monkey
    monkeyModel = AnimationObject(AnimationObjectType::model);
    monkeyModel.meshName = "../assets/models/tambourine/monkeyCharacter2.obj";

    // Create and register the texture
    auto monkeyTexture = std::make_shared<Texture>("../assets/models/tambourine/monkeyCharacter2.png");
    TextureRegistry::addTexture(monkeyTexture);

    // Assign the texture to the model
    monkeyModel.texture = reinterpret_cast<void*>(monkeyTexture->id);

    // Set up model properties
    monkeyModel.localPosition = vec3(-2.0f, 0.0f, 0.0f);  // Same position as box monkey
    monkeyModel.localScale = vec3(1.0f);
    monkeyModel.color = vec4(1.0f);  // White color to show texture properly
    monkeyModel.updateMatrix(true);
    // ==========================================

    // ==========================================
    // Add new model player
    playerModel = AnimationObject(AnimationObjectType::model);
    playerModel.meshName = "../assets/models/tambourine/userCharacterModel.obj";

    // Create and register the texture
    auto playerTexture = std::make_shared<Texture>("../assets/models/tambourine/userTexture.png");
    TextureRegistry::addTexture(playerTexture);

    // Assign the texture to the model
    playerModel.texture = reinterpret_cast<void*>(playerTexture->id);

    // Set up model properties
    playerModel.localPosition = vec3(2.0f, 0.0f, 0.0f);  // Same position as box monkey
    playerModel.localScale = vec3(1.0f);
    playerModel.color = vec4(1.0f);  // White color to show texture properly
    playerModel.updateMatrix(true);
    // ==========================================


    //monkeyTambourine = AnimationObject(AnimationObjectType::box);
    //monkeyTambourine.localScale = vec3(0.8f, 0.1f, 0.8f);
    //monkeyTambourine.color = vec4(0.8f, 0.7f, 0.6f, 1.0f);
    //monkeyTambourine.updateMatrix(true);

    // Add new model tambourine
    monkeyTambourineModel = AnimationObject(AnimationObjectType::model);
    monkeyTambourineModel.meshName = "../assets/models/tambourine/tambourineModel.obj";

    auto tambourineTexture = std::make_shared<Texture>("../assets/models/tambourine/tambourineTexture.png"); // or .jpg

    // Add it to the texture registry
    TextureRegistry::addTexture(tambourineTexture);

    // Assign the texture ID to your model
    monkeyTambourineModel.texture = reinterpret_cast<void*>(tambourineTexture->id);
    monkeyTambourineModel.localScale = vec3(1.0f);
    monkeyTambourineModel.updateMatrix(true);


    //player = AnimationObject(AnimationObjectType::box);
    //player.localPosition = vec3(2.0f, 0.0f, 0.0f);
    //player.localScale = vec3(1.0f);
    //player.color = vec4(1.0f);
    //player.updateMatrix(true);

    //playerTambourine = AnimationObject(AnimationObjectType::box);
    //playerTambourine.localScale = vec3(0.8f, 0.1f, 0.8f);
    //playerTambourine.color = vec4(0.8f, 0.7f, 0.6f, 1.0f);
    //playerTambourine.updateMatrix(true);


    // Add new model tambourine
    playerTambourineModel = AnimationObject(AnimationObjectType::model);
    playerTambourineModel.meshName = "../assets/models/tambourine/userTambourineModel.obj";
    playerTambourineModel.localScale = vec3(1.0f);
    playerTambourineModel.color = vec4(1.0f);
    playerTambourineModel.updateMatrix(true);


    // ===========================================================
    playerIndicatorModel = AnimationObject(AnimationObjectType::model);
    playerIndicatorModel.meshName = "../assets/models/tambourine/playerIndicator.obj";
    auto playerIndicatorTexture = std::make_shared<Texture>("../assets/models/tambourine/playerIndicatorTexture.png");
    TextureRegistry::addTexture(playerIndicatorTexture);
    playerIndicatorModel.texture = reinterpret_cast<void*>(playerIndicatorTexture->id);
    playerIndicatorModel.updateMatrix(true);
    playerIndicatorModel.localPosition = vec3(playerModel.localPosition.x + 0.5f,
        playerModel.localPosition.y + 2.5f,
        playerModel.localPosition.z);
    // ===========================================================
        // ===========================================================
    backgroundModel = AnimationObject(AnimationObjectType::model);
    backgroundModel.meshName = "../assets/models/tambourine/tambourineBackgroundModel.obj";
    auto backgroundModelTexture = std::make_shared<Texture>("../assets/models/tambourine/tambourineBackgroundTexture.png");
    TextureRegistry::addTexture(backgroundModelTexture);
    backgroundModel.texture = reinterpret_cast<void*>(backgroundModelTexture->id);
    backgroundModel.updateMatrix(true);
    // ===========================================================



    //playerIndicator = AnimationObject(AnimationObjectType::box);
    //playerIndicator.localScale = vec3(0.3f);
    //playerIndicator.color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
    //playerIndicator.updateMatrix(true);

    feedbackCube = AnimationObject(AnimationObjectType::box);
    feedbackCube.localPosition = vec3(0.0f, 2.0f, 0.0f);
    feedbackCube.localScale = vec3(0.5f);
    feedbackCube.color = vec4(0.5f);
    feedbackCube.updateMatrix(true);

    timingIndicator = AnimationObject(AnimationObjectType::box);
    timingIndicator.localPosition = vec3(0.0f, -1.0f, 0.0f);
    timingIndicator.localScale = vec3(4.0f, 0.1f, 0.1f);
    timingIndicator.color = vec4(0.3f, 0.3f, 0.3f, 1.0f);
    timingIndicator.updateMatrix(true);

    // Initialize game state
    beatPulseScale = 1.0f;
    lastBeatTime = 0.0f;
    currentBeatCount = 0;
    comboCount = 0;
    lastInputTime = 0.0f;

    initializeSequences();
    audio.loadScene("tambourine");

    // Set the intro start time
    introStartTime = currentTime;
    currentState = GameState::Intro;
}

void TambourineScene::initializeSequences() {
    // Initialize some example rhythm sequences (4/4 time)
    availableSequences = {
        // Simple sequence: 4 quarter notes (all shakes)
        {{{RhythmNote::Type::Shake, 0.0f},
          {RhythmNote::Type::Shake, 1.0f},
          {RhythmNote::Type::Shake, 2.0f},
          {RhythmNote::Type::Shake, 3.0f},}},

          // Mix of shakes and claps
          {{{RhythmNote::Type::Shake, 0.0f},
            {RhythmNote::Type::Clap, 1.0f},
            {RhythmNote::Type::Shake, 2.0f},
            {RhythmNote::Type::Clap, 3.0f}}},

            // More complex pattern with eighth notes
            {{{RhythmNote::Type::Shake, 0.0f},
              {RhythmNote::Type::Shake, 0.5f},
              {RhythmNote::Type::Clap, 1.0f},
              {RhythmNote::Type::Shake, 2.0f},
              {RhythmNote::Type::Shake, 2.5f},
              {RhythmNote::Type::Clap, 3.0f}}}
    };
}

void TambourineScene::update(double now, float dt) {
    currentTime += dt;

    // Handle intro state exit
    if (currentState == GameState::Intro) {
        // Check if intro duration has elapsed
        if (currentTime - introStartTime >= INTRO_DURATION) {
            currentState = GameState::MonkeyTurn;
            sequenceStartTime = currentTime;
            startNewSequence();
        }
        return;
    }

    // Update animations and beat visuals
    updateAnimations(dt);
    updateBeatVisuals(dt);

    // Update game state
    switch (currentState) {
    case GameState::MonkeyTurn:
        updateMonkeyTurn(dt);
        break;

    case GameState::PlayerTurn:
        updatePlayerTurn(dt);
        break;

    case GameState::Evaluation:
        evaluatePlayerSequence(dt);
        break;
    }

    // Update matrices
    //monkey.updateMatrix(true);
    //monkeyTambourine.updateMatrix(true);
    //player.updateMatrix(true);
    //playerTambourine.updateMatrix(true);
    //playerIndicator.updateMatrix(true);
    feedbackCube.updateMatrix(true);
    timingIndicator.updateMatrix(true);  // Don't forget the new timing indicator!
}

void TambourineScene::updateBeatVisuals(float dt) {
    float currentBeat = (currentTime - sequenceStartTime) / BEAT_DURATION;
    float beatPhase = fmod(currentBeat, 1.0f);

    // Update beat pulse
    beatPulseScale = 1.0f + 0.2f * (1.0f - beatPhase);
    feedbackCube.localScale = vec3(0.5f * beatPulseScale);

    // Update timing indicator
    float progress = currentBeat / 4.0f; // 4 beats per measure
    timingIndicator.color = vec4(0.3f + progress * 0.7f, 0.3f, 0.3f, 1.0f);
}


void TambourineScene::updateMonkeyTurn(float dt) {
    float sequenceTime = currentTime - sequenceStartTime;
    float currentBeat = sequenceTime / BEAT_DURATION;

    if (currentNoteIndex < currentSequence.notes.size()) {
        const auto& note = currentSequence.notes[currentNoteIndex];

        if (currentBeat >= note.timing) {
            // Alternate between normal and alt sounds for shakes
            bool useAlt = (rand() % 2) == 0;

            // Play appropriate sound and animation
            switch (note.type) {
            case RhythmNote::Type::Shake:
                audio.playSound(useAlt ? "tamb_monkey_a_2" : "tamb_monkey_a");
                monkeyAnim.isShaking = true;
                break;
            case RhythmNote::Type::Clap:
                audio.playSound("tamb_monkey_ab");
                monkeyAnim.isClapping = true;
                break;
            }
            currentNoteIndex++;
        }
    }
    else if (sequenceTime >= (4.0f * BEAT_DURATION)) {
        // Play the transition sound
        audio.playSound("tamb_monkey_squeal");

        // Update sequence timing to account for transition
        sequenceStartTime = currentTime + TRANSITION_DURATION;
        currentState = GameState::PlayerTurn;
        currentNoteIndex = 0;
        playerSequence.clear();
    }
}

void TambourineScene::updatePlayerTurn(float dt) {
    auto& input = Input::get();
    bool isShakePressed = input.current.keyStates[GLFW_KEY_L] == GLFW_PRESS;
    bool isClapPressed = input.current.keyStates[GLFW_KEY_K] == GLFW_PRESS;

    float sequenceTime = currentTime - sequenceStartTime;
    float currentBeat = sequenceTime / BEAT_DURATION;

    // Handle inputs
    if ((isShakePressed && !lastShakeState) || (isClapPressed && !lastClapState)) {
        float inputBeat = currentBeat;
        lastInputTime = currentTime;

        // Find closest expected beat
        float closestBeat = -1;
        float minDiff = TIMING_TOLERANCE;
        for (const auto& note : currentSequence.notes) {
            float diff = std::abs(inputBeat - note.timing);
            if (diff < minDiff) {
                minDiff = diff;
                closestBeat = note.timing;
            }
        }

        // Store timing difference for feedback
        playerTimings.push_back(inputBeat - closestBeat);

        RhythmNote::Type inputType = isShakePressed ? RhythmNote::Type::Shake : RhythmNote::Type::Clap;
        playerSequence.push_back({ inputType, inputBeat });

        if (isShakePressed) {
            useAlternateSound = !useAlternateSound;
            audio.playSound(useAlternateSound ? "tamb_player_a_2" : "tamb_player_a");
            playerAnim.isShaking = true;
        }
        else {
            audio.playSound("tamb_player_ab");
            playerAnim.isClapping = true;
        }
    }

    lastShakeState = isShakePressed;
    lastClapState = isClapPressed;

    if (currentTime - sequenceStartTime >= (4.0f * BEAT_DURATION)) {
        currentState = GameState::Evaluation;
    }
}

void TambourineScene::evaluatePlayerSequence(float dt) {
    static float evaluationTime = 0;  // Keep track of how long we've been evaluating

    // Only evaluate on first entry into this state
    if (evaluationTime == 0) {
        bool success = true;
        // Simple evaluation - check if we have the same number of notes
        if (playerSequence.size() == currentSequence.notes.size()) {
            // Check timing of each note (with some tolerance)
            for (size_t i = 0; i < playerSequence.size(); i++) {
                if (std::abs(playerSequence[i].timing - currentSequence.notes[i].timing) > TIMING_TOLERANCE) {
                    success = false;
                    break;
                }
            }
        }
        else {
            success = false;
        }

        if (success) {
            audio.playSound("tamb_monkey_happy");
            feedbackCube.color = vec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
        }
        else {
            audio.playSound("tamb_monkey_squeal");
            feedbackCube.color = vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
        }
    }

    evaluationTime += dt;  // Add time passed

    // Only move to next sequence after one beat
    if (evaluationTime >= BEAT_DURATION) {
        evaluationTime = 0;  // Reset for next evaluation
        startNewSequence();
    }
}

void TambourineScene::startNewSequence() {
    // Randomly select a new sequence
    currentSequence = availableSequences[rand() % availableSequences.size()];
    currentNoteIndex = 0;
    sequenceStartTime = currentTime;
    currentState = GameState::MonkeyTurn;
}

void TambourineScene::updateAnimations(float dt) {
    const float ANIM_DURATION = 0.2f;

    // Update monkey animations
    if (monkeyAnim.isShaking || monkeyAnim.isClapping) {
        monkeyAnim.animationTime += dt;
        if (monkeyAnim.animationTime >= ANIM_DURATION) {
            monkeyAnim.isShaking = false;
            monkeyAnim.isClapping = false;
            monkeyAnim.animationTime = 0.0f;
        }

        float t = monkeyAnim.animationTime / ANIM_DURATION;
        if (monkeyAnim.isShaking) {
            vec3 newPos = vec3(-0.8f + 0.2f * sin(t * 12.0f), 0.0f, 0.0f);
            //monkeyTambourine.localPosition = newPos;
            monkeyTambourineModel.localPosition = newPos;
            monkeyTambourineModel.localRotation = vec3(0.0f, 0.0f, 15.0f * sin(t * 12.0f));

            // Add model monkey animation
            monkeyModel.localPosition = vec3(-2.0f + 0.2f * sin(t * 12.0f), 0.0f, 0.0f);

        }
        else if (monkeyAnim.isClapping) {
            vec3 newPos = vec3(0.0f, 0.5f * (1.0f - t), 0.8f);
            //monkeyTambourine.localPosition = newPos;
            monkeyTambourineModel.localPosition = newPos;
        }
    }
    else {
        vec3 defaultPos = vec3(-0.8f, 0.0f, 0.0f);
        //monkeyTambourine.localPosition = defaultPos;
        monkeyTambourineModel.localPosition = defaultPos;
        monkeyTambourineModel.localRotation = vec3(0.0f);

        // Reset model monkey position
        monkeyModel.localPosition = vec3(-2.0f, 0.0f, 0.0f);
    }

    // Update player animations
    if (playerAnim.isShaking || playerAnim.isClapping) {
        playerAnim.animationTime += dt;
        if (playerAnim.animationTime >= ANIM_DURATION) {
            playerAnim.isShaking = false;
            playerAnim.isClapping = false;
            playerAnim.animationTime = 0.0f;
        }

        float t = playerAnim.animationTime / ANIM_DURATION;
        if (playerAnim.isShaking) {
            vec3 newPos = vec3(0.8f + 0.2f * sin(t * 12.0f), 0.0f, 0.0f);
            //playerTambourine.localPosition = newPos;
            playerTambourineModel.localPosition = newPos;
            playerTambourineModel.localRotation = vec3(0.0f, 0.0f, 15.0f * sin(t * 12.0f));
        }
        else if (playerAnim.isClapping) {
            vec3 newPos = vec3(0.0f, 0.5f * (1.0f - t), 0.8f);
            //playerTambourine.localPosition = newPos;
            playerTambourineModel.localPosition = newPos;
        }
    }
    else {
        vec3 defaultPos = vec3(0.8f, 0.0f, 0.0f);
        //playerTambourine.localPosition = defaultPos;
        playerTambourineModel.localPosition = defaultPos;
        playerTambourineModel.localRotation = vec3(0.0f);
    }

    // Update player indicator position
    //playerIndicator.localPosition = vec3(playerModel.localPosition.x,
    //    playerModel.localPosition.y + 3.5f,
    //    playerModel.localPosition.z);

    // Don't forget to update matrices
    monkeyTambourineModel.updateMatrix(true);
    playerTambourineModel.updateMatrix(true);
    monkeyModel.updateMatrix(true);
    playerModel.updateMatrix(true);
    playerIndicatorModel.updateMatrix(true);
    backgroundModel.updateMatrix(true);
}

void TambourineScene::render(const mat4& projection, const mat4& view, bool isShadow) {
    renderMonkey(projection, view, isShadow);
    renderPlayer(projection, view, isShadow);

    // Render feedback cube if enabled
    if (showFeedbackCube) {
        auto& jr = AnimationObjectRenderer::get();
        jr.beginBatchRender(feedbackCube.shapeType, false, vec4(1.f), isShadow);
        jr.renderBatchWithOwnColor(feedbackCube, isShadow);
        jr.endBatchRender(isShadow);
    }
}

void TambourineScene::renderUI() {
    if (ImGui::CollapsingHeader("Tambourine Game Status")) {
        // Game state
        const char* stateStr = "Unknown";
        switch (currentState) {
            case GameState::Intro: stateStr = "Get Ready!"; break;
            case GameState::MonkeyTurn: stateStr = "Watch the Monkey!"; break;
            case GameState::PlayerTurn: stateStr = "Your Turn!"; break;
            case GameState::Evaluation: stateStr = "Checking..."; break;
        }
        ImGui::Text("Current State: %s", stateStr);

        // Timing info
        float sequenceTime = currentTime - sequenceStartTime;
        float currentBeat = sequenceTime / BEAT_DURATION;
        ImGui::Text("Beat: %.2f / 4.00", currentBeat);
        ImGui::Text("Combo: %d", comboCount);

        // Recent timing feedback
        if (!playerTimings.empty()) {
            ImGui::Text("Last Hit Timing: %s", 
                getTimingFeedback(playerTimings.back()).c_str());
        }

        // Controls reminder
        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::Text("L - Shake");
        ImGui::Text("K - Clap");
        
        // Visual settings
        ImGui::Separator();
        ImGui::Checkbox("Show Beat Indicator", &showFeedbackCube);
    }
}

ptr_vector<AnimationObject> TambourineScene::getObjects() {
    return { &monkeyTambourineModel,
             &playerTambourineModel,
             &playerIndicatorModel,
        &backgroundModel,
             &feedbackCube, &timingIndicator,
             &monkeyModel, &playerModel };
}

TambourineScene::~TambourineScene() {
    // Cleanup if needed
}

void TambourineScene::renderMonkey(const mat4& projection, const mat4& view, bool isShadow) {
    auto& jr = AnimationObjectRenderer::get();

    //// Render monkey
    //jr.beginBatchRender(monkey.shapeType, false, vec4(1.f), isShadow);
    //if (!isShadow) {
    //    if (monkey.cullFace) {
    //        glEnable(GL_CULL_FACE);
    //        glCullFace(GL_BACK);
    //    }
    //    else {
    //        glDisable(GL_CULL_FACE);
    //    }
    //}
    //if (isShadow) {
    //    glEnable(GL_CULL_FACE);
    //    glCullFace(GL_FRONT);
    //}
    //jr.renderBatchWithOwnColor(monkey, isShadow);
    //jr.endBatchRender(isShadow);

    // Render monkey's tambourine
    //jr.beginBatchRender(monkeyTambourine.shapeType, false, vec4(1.f), isShadow);
    //if (!isShadow) {
    //    if (monkeyTambourine.cullFace) {
    //        glEnable(GL_CULL_FACE);
    //        glCullFace(GL_BACK);
    //    }
    //    else {
    //        glDisable(GL_CULL_FACE);
    //    }
    //}
    //if (isShadow) {
    //    glEnable(GL_CULL_FACE);
    //    glCullFace(GL_FRONT);
    //}
    //jr.renderBatchWithOwnColor(monkeyTambourine, isShadow);
    //jr.endBatchRender(isShadow);


    // Add rendering for model monkey
    jr.beginBatchRender(monkeyModel, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(monkeyModel, isShadow);
    jr.endBatchRender(isShadow);


    // Render model monkey's tambourine
    jr.beginBatchRender(monkeyTambourineModel, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(monkeyTambourineModel, isShadow);
    jr.endBatchRender(isShadow);
}

void TambourineScene::renderPlayer(const mat4& projection, const mat4& view, bool isShadow) {
    auto& jr = AnimationObjectRenderer::get();

    //// Render player
    //jr.beginBatchRender(player.shapeType, false, vec4(1.f), isShadow);
    //if (!isShadow) {
    //    if (player.cullFace) {
    //        glEnable(GL_CULL_FACE);
    //        glCullFace(GL_BACK);
    //    }
    //    else {
    //        glDisable(GL_CULL_FACE);
    //    }
    //}
    //if (isShadow) {
    //    glEnable(GL_CULL_FACE);
    //    glCullFace(GL_FRONT);
    //}
    //jr.renderBatchWithOwnColor(player, isShadow);
    //jr.endBatchRender(isShadow);

    //// Render player's tambourine
    //jr.beginBatchRender(playerTambourine.shapeType, false, vec4(1.f), isShadow);
    //if (!isShadow) {
    //    if (playerTambourine.cullFace) {
    //        glEnable(GL_CULL_FACE);
    //        glCullFace(GL_BACK);
    //    }
    //    else {
    //        glDisable(GL_CULL_FACE);
    //    }
    //}
    //if (isShadow) {
    //    glEnable(GL_CULL_FACE);
    //    glCullFace(GL_FRONT);
    //}
    //jr.renderBatchWithOwnColor(playerTambourine, isShadow);
    //jr.endBatchRender(isShadow);

    // Render player indicator
    //jr.beginBatchRender(playerIndicator.shapeType, false, vec4(1.f), isShadow);
    //if (!isShadow) {
    //    if (playerIndicator.cullFace) {
    //        glEnable(GL_CULL_FACE);
    //        glCullFace(GL_BACK);
    //    }
    //    else {
    //        glDisable(GL_CULL_FACE);
    //    }
    //}
    //if (isShadow) {
    //    glEnable(GL_CULL_FACE);
    //    glCullFace(GL_FRONT);
    //}
    //jr.renderBatchWithOwnColor(playerIndicator, isShadow);
    //jr.endBatchRender(isShadow);

    jr.beginBatchRender(playerModel, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(playerModel, isShadow);
    jr.endBatchRender(isShadow);


    // Render model player's tambourine
    jr.beginBatchRender(playerTambourineModel, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(playerTambourineModel, isShadow);
    jr.endBatchRender(isShadow);

    jr.beginBatchRender(playerIndicatorModel, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(playerIndicatorModel, isShadow);
    jr.endBatchRender(isShadow);

    jr.beginBatchRender(backgroundModel, false, vec4(1.f), isShadow);
    jr.renderBatchWithOwnColor(backgroundModel, isShadow);
    jr.endBatchRender(isShadow);

}

std::string TambourineScene::getTimingFeedback(float difference) {
    difference = std::abs(difference);
    if (difference < 0.1f) return "PERFECT!";
    if (difference < 0.2f) return "Great!";
    if (difference < TIMING_TOLERANCE) return "Good";
    return "Miss";
}