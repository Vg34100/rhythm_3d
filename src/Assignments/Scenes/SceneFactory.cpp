#include "SceneFactory.h"
#include "SingleCubeScene.h"
#include "SingleSphereScene.h"
#include "TambourineScene.h"
#include "BuiltToScaleScene.h"
#include "SeesawScene.h"
#include "HoleInOneScene.h"

std::unique_ptr<Scene> createScene(SceneType type) {
    switch (type) {
        // TODO: Have to add SceneSelector Case for each new Scene
        case SceneType::SingleCube:
            return std::make_unique<SingleCubeScene>();
        case SceneType::SingleSphere:
            return std::make_unique<SingleSphereScene>();
        case SceneType::TambourineScene:
            return std::make_unique<TambourineScene>();
        case SceneType::BuiltToScaleScene:
            return std::make_unique<BuiltToScaleScene>();
        case SceneType::SeeSawScene:
            return std::make_unique<SeesawScene>();
        case SceneType::HoleInOneScene:
            return std::make_unique<HoleInOneScene>();
        default:
            return nullptr;
    }
}