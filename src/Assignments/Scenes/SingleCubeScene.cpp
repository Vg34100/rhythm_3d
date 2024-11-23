#include "SingleCubeScene.h"
#include "AnimationObjectRenderer.h"
#include "imgui.h"

SingleCubeScene::SingleCubeScene() {}

SingleCubeScene::~SingleCubeScene() {}

void SingleCubeScene::init() {
    centerCube = AnimationObject(AnimationObjectType::box);
    centerCube.localPosition = vec3(0.0f);
    centerCube.localScale = vec3(1.0f);
    centerCube.color = vec4(1.0f, 0.5f, 0.0f, 1.0f);
}

void SingleCubeScene::update(double now, float dt) {
    centerCube.updateMatrix(true);
}

void SingleCubeScene::render(const mat4& projection, const mat4& view, bool isShadow) {
    auto& jr = AnimationObjectRenderer::get();

    jr.beginBatchRender(centerCube.shapeType, false, vec4(1.f), isShadow);
    if (!isShadow) {
        // Bind lighting and set culling
        if (centerCube.cullFace) {
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
    jr.renderBatchWithOwnColor(centerCube, isShadow);
    jr.endBatchRender(isShadow);
}

void SingleCubeScene::renderUI() {
    if (ImGui::CollapsingHeader("Cube Properties")) {
        centerCube.renderUI();
    }
}

ptr_vector<AnimationObject> SingleCubeScene::getObjects() {
    return { &centerCube };
}
