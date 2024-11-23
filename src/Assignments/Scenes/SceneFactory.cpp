#include "SceneFactory.h"
#include "SingleCubeScene.h"
#include "SingleSphereScene.h"
#include "PianoScene.h"

std::unique_ptr<Scene> createScene(SceneType type) {
    switch (type) {
        // TODO: Have to add SceneSelector Case for each new Scene
        case SceneType::SingleCube:
            return std::make_unique<SingleCubeScene>();
        case SceneType::SingleSphere:
            return std::make_unique<SingleSphereScene>();
        case SceneType::Piano:
            return std::make_unique<PianoScene>();
        default:
            return nullptr;
    }
}