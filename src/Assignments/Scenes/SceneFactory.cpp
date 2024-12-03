#include "SceneFactory.h"
#include "SingleCubeScene.h"
#include "SingleSphereScene.h"
#include "TambourineScene.h"
#include "BuiltToScaleScene.h"

std::unique_ptr<Scene> createScene(SceneType type) {
    switch (type) {
        // TODO: Have to add SceneSelector Case for each new Scene
        case SceneType::SingleCube:
            return std::make_unique<SingleCubeScene>();
        case SceneType::SingleSphere:
            return std::make_unique<SingleSphereScene>();
        case SceneType::TambourineScene:
            return std::make_unique<TambourineScene>();
        case SceneType::BuiltToScaleScene:  // Add this case
            return std::make_unique<BuiltToScaleScene>();
        default:
            return nullptr;
    }
}