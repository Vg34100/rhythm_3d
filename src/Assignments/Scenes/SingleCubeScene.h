#pragma once

#include "Scene.h"
#include "AnimationObject.h"

class SingleCubeScene : public Scene {
public:
    SingleCubeScene();
    ~SingleCubeScene() override;

    void init() override;
    void update(double now, float dt) override;
    void render(const mat4& projection, const mat4& view, bool isShadow) override;
    void renderUI() override;
    ptr_vector<AnimationObject> getObjects() override;

private:
    AnimationObject centerCube;
};