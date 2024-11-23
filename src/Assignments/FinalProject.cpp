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
	assert(yourName != nullptr);

	ground = AnimationObject(AnimationObjectType::quad, vec3(0.f, -0.01f, 0.f), vec3(0.f, 0.f, 90.f), vec3(250), nullptr, vec4(vec3(0.5f), 1.f));
	ground.cullFace = true;
	
	initialized = true;
}

void FinalProject::update() {
	if (!initialized) init();

}

void FinalProject::render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow) {

	auto& jr = AnimationObjectRenderer::get();
	// Render ground
	jr.beginBatchRender(ground.shapeType, false, vec4(1.f), isShadow);

	if (!isShadow) {
		GPU::Lighting::get().bind(jr.currentMesh->shader);
		if (ground.cullFace)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
		}
		else {
			glDisable(GL_CULL_FACE);
		}
	}

	if (isShadow) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}

	ground.updateMatrix(true);
	jr.renderBatchWithOwnColor(ground, isShadow);
	jr.endBatchRender(isShadow);
}

// Renders the UI controls for the lab.
void FinalProject::renderUI() {

	if (ImGui::CollapsingHeader("Ground")) {
		ground.renderUI();
	}
}

ptr_vector<AnimationObject> FinalProject::getObjects() {
	// Add pointers to objects in this function if you want to be able to select/move them using the mouse
	// and keyboard shortcuts. 
	ptr_vector<AnimationObject> object_ptrs = { &ground };
	return object_ptrs;
}