#include "SceneFactory.h"
#include "SingleCubeScene.h"
#include "SingleSphereScene.h"
#include "ModelTestScene.h"

std::unique_ptr<Scene> createScene(SceneType type) {
    switch (type) {
        // TODO: Have to add SceneSelector Case for each new Scene
        case SceneType::SingleCube:
            return std::make_unique<SingleCubeScene>();
        case SceneType::SingleSphere:
            return std::make_unique<SingleSphereScene>();
        case SceneType::ModelTest:    // Add this case
            return std::make_unique<ModelTestScene>();

        default:
            return nullptr;
    }
}