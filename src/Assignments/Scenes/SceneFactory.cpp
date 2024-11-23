#include "SceneFactory.h"
#include "SingleCubeScene.h"

std::unique_ptr<Scene> createScene(SceneType type) {
    switch (type) {
        case SceneType::SingleCube:
            return std::make_unique<SingleCubeScene>();
        default:
            return nullptr;
    }
}