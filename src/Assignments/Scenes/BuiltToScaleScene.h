#pragma once

#include "Scene.h"
#include "AnimationObject.h"

class BuiltToScaleScene : public Scene {
public:
    BuiltToScaleScene();
    ~BuiltToScaleScene() override;

    void init() override;
    void update(double now, float dt) override;
    void render(const mat4& projection, const mat4& view, bool isShadow) override;
    void renderUI() override;
    ptr_vector<AnimationObject> getObjects() override;

private:
    // Visual objects
    std::vector<AnimationObject> springs; // Index 2 is the player's red spring
    AnimationObject pole;
    AnimationObject inputIndicator;  // Visual feedback for input timing

    // Animation states
    enum class PoleState {
        Normal,     // Regular bouncing between springs
        Failed,     // Missed input, falling off
        Respawning  // Moving back to start position
    };

    struct {
        int currentSpringIndex;     // Current spring the pole is at/heading to
        bool movingRight;           // Direction of movement
        float animationTime;        // Current time in bounce animation
        vec3 startPos;             // Starting position of current bounce
        vec3 endPos;               // Target position of current bounce
        PoleState state;           // Current state of pole animation
        float failTime;            // Time since failure started
        bool inputRequired;        // Flag for when input is needed
        bool inputSuccess;         // Whether input was successful
    } poleAnim;

    // In the header, add to private section:
    struct SpringAnimation {
        bool isAnimating;
        float animationTime;
        float baseHeight;
        static constexpr float ANIM_DURATION = 0.2f;  // Quick bounce
        static constexpr float MAX_BOUNCE = 0.2f;     // How high it bounces
    };

    // Add to class private members:
    std::vector<SpringAnimation> springAnims;

    // Constants
    const float BOUNCE_DURATION = 0.5f;   // Time for one bounce
    const float SPRING_SPACING = 2.0f;    // Space between springs
    const float BOUNCE_HEIGHT = 1.0f;     // Height of bounce arc
    const int NUM_SPRINGS = 4;            // Total number of springs
    const float FAIL_DURATION = 1.0f;     // How long failure animation lasts
    const float INPUT_WINDOW = 0.50f;     // Time window for valid input
    const float POLE_Z_OFFSET = 0.0f;     // Offset to put pole through springs

    // Helper functions
    void initSprings();
    void initPole();
    void initInputIndicator();
    void updatePoleAnimation(float dt);
    vec3 calculatePolePosition(float t);
    void startNewBounce();
    void handleInput();
    void failPole();
    void respawnPole();
    vec3 calculateFailPosition(float t);
    void updateInputIndicator(double now, float dt);
    void triggerSpringBounce(int springIndex);
};