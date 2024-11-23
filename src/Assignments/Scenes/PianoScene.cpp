// PianoScene.cpp
#include "PianoScene.h"
#include "AnimationObjectRenderer.h"
#include "Input.h"
#include "imgui.h"

PianoScene::PianoScene() {
    keys.resize(NUM_KEYS);
}

PianoScene::~PianoScene() {
}

void PianoScene::init() {
    // Initialize each key
    for (int i = 0; i < NUM_KEYS; i++) {
        keys[i] = AnimationObject(AnimationObjectType::box);
        keys[i].localPosition = vec3(i * KEY_SPACING - (NUM_KEYS * KEY_SPACING / 2.f), 0.0f, 0.0f);
        keys[i].localScale = vec3(1.0f, 0.2f, 2.0f);
        keys[i].color = vec4(0.9f, 0.9f, 0.9f, 1.0f);
    }
}

void PianoScene::update(double now, float dt) {
    auto& input = Input::get();
    auto& audio = AudioSystem::get();

    // Handle key input
    for (int i = 0; i < NUM_KEYS; i++) {
        bool isPressed = input.current.keyStates[KEY_BINDINGS[i]] == GLFW_PRESS;

        if (isPressed) {
            keys[i].localScale.y = PRESSED_SCALE;
            audio.playNote(i);
        }
        else {
            keys[i].localScale.y = BASE_SCALE;
            audio.stopNote(i);
        }


     
        keys[i].updateMatrix(true);
    }
    audio.update();

}

void PianoScene::render(const mat4& projection, const mat4& view, bool isShadow) {
    auto& jr = AnimationObjectRenderer::get();

    for (auto& key : keys) {
        jr.beginBatchRender(key.shapeType, false, vec4(1.f), isShadow);
        if (!isShadow) {
            if (key.cullFace) {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
            }
            else {
                glDisable(GL_CULL_FACE);
            }
        }
        if (isShadow) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
        }
        jr.renderBatchWithOwnColor(key, isShadow);
        jr.endBatchRender(isShadow);
    }
}

void PianoScene::renderUI() {
    if (ImGui::CollapsingHeader("Piano Properties")) {
        ImGui::Text("Press A, S, D, F to play notes");
        ImGui::Separator();

        for (int i = 0; i < NUM_KEYS; i++) {
            std::string label = "Key " + std::string(1, 'A' + i);
            if (ImGui::TreeNode(label.c_str())) {
                keys[i].renderUI();
                ImGui::TreePop();
            }
        }
    }
}

ptr_vector<AnimationObject> PianoScene::getObjects() {
    ptr_vector<AnimationObject> objects;
    for (auto& key : keys) {
        objects.push_back(&key);
    }
    return objects;
}