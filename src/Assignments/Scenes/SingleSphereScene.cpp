#include "SingleSphereScene.h"
#include "AnimationObjectRenderer.h"
#include "imgui.h"

SingleSphereScene::SingleSphereScene() {}

SingleSphereScene::~SingleSphereScene() {}

void SingleSphereScene::init() {
    centerSphere = AnimationObject(AnimationObjectType::sphere);
    centerSphere.localPosition = vec3(0.0f);
    centerSphere.localScale = vec3(1.0f);
    centerSphere.color = vec4(0.5f, 1.0f, 0.0f, 1.0f);
}

void SingleSphereScene::update(double now, float dt) {
    centerSphere.updateMatrix(true);
}

void SingleSphereScene::render(const mat4& projection, const mat4& view, bool isShadow) {
    auto& jr = AnimationObjectRenderer::get();

    jr.beginBatchRender(centerSphere.shapeType, false, vec4(1.f), isShadow);
    if (!isShadow) {
        if (centerSphere.cullFace) {
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
    jr.renderBatchWithOwnColor(centerSphere, isShadow);
    jr.endBatchRender(isShadow);
}

void SingleSphereScene::renderUI() {
    if (ImGui::CollapsingHeader("Sphere Properties")) {
        centerSphere.renderUI();
    }
}

ptr_vector<AnimationObject> SingleSphereScene::getObjects() {
    return { &centerSphere };
}
