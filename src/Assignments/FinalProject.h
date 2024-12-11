#pragma once

#include "Assignment.h"
#include "Scenes/SceneFactory.h"

class FinalProject : public Assignment {
public:

	FinalProject() : Assignment("Final Project", true) { }
	virtual ~FinalProject() { }

	virtual void init();
	virtual void update();

	virtual void render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow = false);
	virtual void renderUI();

	virtual ptr_vector<AnimationObject> getObjects();
private:
    std::unique_ptr<Scene> currentScene;
    SceneType currentSceneType;
    double lastFrameChange;


    // Remix mode data
    struct RemixData {
        bool enabled;
        double timeInCurrentScene;
        double minSceneTime;  // Minimum time before allowing scene switch
        double maxSceneTime;  // Maximum time before forcing scene switch
        std::vector<SceneType> availableScenes;

        RemixData() : enabled(false),
            timeInCurrentScene(0.0),
            minSceneTime(25.0),
            maxSceneTime(50.0)
        {
            availableScenes = {
                SceneType::TambourineScene,
                SceneType::BuiltToScaleScene,
                SceneType::SeeSawScene,
                SceneType::HoleInOneScene
            };
        }

        SceneType getRandomScene(SceneType currentType) {
            if (availableScenes.empty()) return currentType;

            SceneType newType;
            do {
                int index = rand() % availableScenes.size();
                newType = availableScenes[index];
            } while (newType == currentType && availableScenes.size() > 1);

            return newType;
        }
    } remixData;

};