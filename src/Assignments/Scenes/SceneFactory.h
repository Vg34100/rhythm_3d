#pragma once
#include <memory>
#include "Scene.h"

enum class SceneType {
    SingleCube
};

std::unique_ptr<Scene> createScene(SceneType type);