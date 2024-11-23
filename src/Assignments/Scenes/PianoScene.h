#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Scene.h"
#include "AnimationObject.h"
#include <vector>
#include "AudioSystem.h"

class PianoScene : public Scene {
public:
    PianoScene();
    ~PianoScene() override;

    void init() override;
    void update(double now, float dt) override;
    void render(const mat4& projection, const mat4& view, bool isShadow) override;
    void renderUI() override;
    ptr_vector<AnimationObject> getObjects() override;

private:
    std::vector<AnimationObject> keys;

    const float BASE_SCALE = 1.0f;
    const float PRESSED_SCALE = 1.2f;
    const float KEY_SPACING = 1.5f;

    static const int NUM_KEYS = 4;
    const int KEY_BINDINGS[NUM_KEYS] = { GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_F };
};