#include "ModelTestScene.h"
#include "AnimationObjectRenderer.h"
#include "imgui.h"

ModelTestScene::ModelTestScene() {}

ModelTestScene::~ModelTestScene() {}

void ModelTestScene::init() {
    model = AnimationObject(AnimationObjectType::model);
    model.meshName = "../assets/models/test.obj";
    model.localPosition = vec3(0.0f);
    model.localScale = vec3(1.0f);
    model.color = vec4(1.0f);
    model.updateMatrix(true);
}

void ModelTestScene::update(double now, float dt) {
    model.updateMatrix(true);
}

void ModelTestScene::render(const mat4& projection, const mat4& view, bool isShadow) {
    auto& jr = AnimationObjectRenderer::get();

    jr.beginBatchRender(model, false, vec4(1.f), isShadow);
    if (!isShadow) {
        if (model.cullFace) {
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
    jr.renderBatchWithOwnColor(model, isShadow);
    jr.endBatchRender(isShadow);
}

void ModelTestScene::renderUI() {
    if (ImGui::CollapsingHeader("Model Properties")) {
        model.renderUI();
    }
}

ptr_vector<AnimationObject> ModelTestScene::getObjects() {
    return { &model };
}