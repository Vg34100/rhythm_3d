// CMPS 4480 Final Project
// Name: 

#include "FinalProject.h"
#include "Application.h"
#include "AnimationObjectRenderer.h"
#include "GLTFImporter.h"
#include "InputOutput.h"
#include "Lighting.h"
#include "Prompts.h"
#include "UIHelpers.h"
#include "Textures.h"

#include <cassert>

#include <glm/gtc/random.hpp>

#include "implot.h"
#include "Scenes/SceneFactory.h"


// Variables for Final Project
namespace cmps_4480_final_project {
	// TODO: change this to not be a nullptr
	const char* yourName = "Pablo Rodriguez";

	// Variables for time and frame progression
	int numFrames = 120;
	int currentFrame = 0;
	int FPS = 24;					// frames per second
	double SPF = 1.0 / 24;			// seconds per frame (1/FPS)
	double lastFrameChange = 0;		// timestamp of the last frame change
	double fixedSecondsElapsed = 0;

	AnimationObject ground;
}

using namespace cmps_4480_final_project;

namespace cmps_4480_final_project {
	
}

void FinalProject::init() {
    currentSceneType = SceneType::SingleCube;
    currentScene = createScene(currentSceneType);
    currentScene->init();
    lastFrameChange = getTime();
}

//void FinalProject::update() {
//	if (!currentScene) init();
//
//    double now = getTime();
//    float dt = static_cast<float>(now - lastFrameChange);
//    lastFrameChange = now;
//
//    if (currentScene) {
//        currentScene->update(now, dt);
//    }
//}
void FinalProject::update() {
    if (!currentScene) init();

    double now = getTime();
    float dt = static_cast<float>(now - lastFrameChange);
    lastFrameChange = now;

    // Update remix mode logic
    if (remixData.enabled) {
        remixData.timeInCurrentScene += dt;

        // Check if it's time to switch scenes
        bool shouldSwitch = false;

        // Force switch if we've exceeded max time
        if (remixData.timeInCurrentScene >= remixData.maxSceneTime) {
            shouldSwitch = true;
        }
        // Random chance to switch after min time
        else if (remixData.timeInCurrentScene >= remixData.minSceneTime) {
            // 10% chance per second to switch
            shouldSwitch = (rand() % 100) < (10 * dt);
        }

        if (shouldSwitch) {
            SceneType newType = remixData.getRandomScene(currentSceneType);
            if (newType != currentSceneType) {
                currentScene.reset();
                currentScene = createScene(newType);
                currentScene->init();
                currentSceneType = newType;
                remixData.timeInCurrentScene = 0.0;
            }
        }
    }

    if (currentScene) {
        currentScene->update(now, dt);
    }
}

void FinalProject::render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow) {
    if (currentScene) {
        currentScene->render(projection, view, isShadow);
    }
}

// Renders the UI controls for the lab.
void FinalProject::renderUI() {
    if (ImGui::CollapsingHeader("Scene Selection")) {
        // Add Remix Mode controls
        if (ImGui::TreeNode("Remix Mode Settings")) {
            // Capture whether the checkbox value changed
            bool remixEnabledChanged = ImGui::Checkbox("Enable Remix Mode", &remixData.enabled);

            // If Remix Mode was just enabled, switch to a random scene immediately
            if (remixEnabledChanged && remixData.enabled) {
                SceneType newType = remixData.getRandomScene(currentSceneType);
                if (newType != currentSceneType) {
                    currentScene.reset();
                    currentScene = createScene(newType);
                    currentScene->init();
                    currentSceneType = newType;
                    remixData.timeInCurrentScene = 0.0;
                }
            }

            if (remixData.enabled) {
                // Show current scene time
                ImGui::Text("Time in current scene: %.1f s", remixData.timeInCurrentScene);
            }

            ImGui::TreePop();
        }

        // Only show manual scene selection when Remix Mode is disabled
        if (!remixData.enabled) {
            static const char* scenes[] = {
                "Single Cube",
                "Single Sphere",
                "Tambourine",
                "Built To Scale"
            };

            int currentItem = static_cast<int>(currentSceneType);
            if (ImGui::Combo("Current Scene", &currentItem, scenes, IM_ARRAYSIZE(scenes))) {
                SceneType newSceneType = static_cast<SceneType>(currentItem);
                if (newSceneType != currentSceneType) {
                    currentScene.reset();
                    currentScene = createScene(newSceneType);
                    currentScene->init();
                    currentSceneType = newSceneType;
                }
            }
        }
    }

    if (currentScene) {
        currentScene->renderUI();
    }
}


ptr_vector<AnimationObject> FinalProject::getObjects() {
    if (currentScene) {
        return currentScene->getObjects();
    }
    return {};
}