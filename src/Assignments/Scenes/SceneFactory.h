#pragma once
#include <memory>
#include "Scene.h"

enum class SceneType {
    // TODO: Add new Scenes to SceneType
    SingleCube,
    SingleSphere,
    TambourineScene
};

std::unique_ptr<Scene> createScene(SceneType type);