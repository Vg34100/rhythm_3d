#include "Application.h"

#include "Assignment.h"
//#include "FullscreenFilter.h"
#include "GeometryUtils.h"
#include "GLTFImporter.h"
//#include "LineRenderer.h"
#include "Input.h"
#include "InputOutput.h"
#include "Prompts.h"
#include "Renderer.h"
//#include "Shader.h"
#include "AnimationObjectRenderer.h"
#include "Tools.h"
#include "UIHelpers.h"

#include <GL/glew.h>
#include "GLFW/glfw3.h"

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include <random>

#include "ImCurveEdit.h"
#include "ImSequencer.h"

#include "Assignments/FinalProject.h"




void Application::init(GLFWwindow* w)
{
	startedAt = getTime();
	lastFrameAt = startedAt;

	window = w;

	//renderer = std::make_shared<SoftwareRenderer>();
	renderer = std::make_shared<OpenGLRenderer>();
	//OGLRenderer = std::make_shared<OpenGLRenderer>();

	if (!renderer) {
		log("Unable to initialize renderer!\n");
		exit(1);
	}

	renderer->init();

	tools.push_back(std::make_shared<CameraTool>(&renderer->camera));
	tools.push_back(std::make_shared<SelectTool>());
	tools.push_back(std::make_shared<TransformTool>());

	tools.push_back(std::make_shared<Tool>("CommandTool", Tool::EventActions({
		{
			InputEvent::KeyEvent(GLFW_KEY_V, GLFW_PRESS),
			[&](Tool* tool, const InputEvent& ie)
			{
				renderer->showImGui = !renderer->showImGui;
				return true;
			}
		}
		})));

	/*objects.push_back({ AnimationObjectType::sphere });
	objects.push_back(AnimationObject(AnimationObjectType::box, vec3(1.f, 0.f, 0.f), vec3(0.f), vec3(1.f), nullptr, vec4(1.f, 0.f, 0.f, 1.f)));*/

	// Floor plane
	//objects.push_back(AnimationObject(AnimationObjectType::quad, vec3(0.f, -6.f, 0.f), vec3(0.f, 0.f, 90.f), vec3(1000), nullptr, vec4(vec3(0.5f), 1.f)));

	// Ceiling plane
	//objects.push_back(AnimationObject(AnimationObjectType::quad, vec3(0.f, 6.f, 0.f), vec3(0.f, 0.f, -90.f), vec3(12.f), nullptr, vec4(vec3(0.5f), 1.f)));

	//// Left wall plane
	//objects.push_back(AnimationObject(AnimationObjectType::quad, vec3(-6.f, 0.f, 0.f), vec3(0.f, 0.f, 0.f), vec3(12.f), nullptr, vec4(1.f, 0.33f, 0.33f, 1.f)));

	//// Right wall plane
	//objects.push_back(AnimationObject(AnimationObjectType::quad, vec3(6.f, 0.f, 0.f), vec3(0.f, 180.f, 0.f), vec3(12.f), nullptr, vec4(0.33f, 0.33f, 1.f, 1.f)));

	//// Back wall plane
	//objects.push_back(AnimationObject(AnimationObjectType::quad, vec3(0.f, 0.f, -6.f), vec3(0.f, 270.f, 0.f), vec3(12.f), nullptr, vec4(0.33f, 1.f, 0.33f, 1.f)));

	// Top light
	//objects.push_back(AnimationObject(AnimationObjectType::sphere, vec3(0.f, 20.f, -50.f), vec3(0.f), vec3(20.f), nullptr, vec4(1.f, 1.f, 0.f, 1.f), 0));

	assignments.push_back(std::make_shared<FinalProject>());

	log("Ready to rock!\n");
}

void Application::update()
{
	// Gets the time (in seconds) since GLFW was initialized
	double nowish = getTime();

	// Gets the time since the application began updating
	timeSinceStart = nowish - startedAt;

	// TODO: fix our timesteps? https://gafferongames.com/post/fix_your_timestep/
	deltaTime = nowish - lastFrameAt;


	glfwGetWindowSize(window, &windowSize.x, &windowSize.y);
	

	// Handle mouse/keyboard/device input events
	auto & input = Input::get();
	// Clear input values from last frame
	input.clear();
	glfwPollEvents();
	input.update();
	
	// Check for quit event
	if (glfwGetKey(window, GLFW_KEY_ESCAPE))
	{
		quit = true;
	}

	// Process any pending commands
	if (!commands.empty()) {
		for (auto& c : commands) {
			c();
		}

		commands.clear();
	}

	// Handle tool updates and possibly consume input events

	auto events = input.getEvents();

	for(auto & tool : tools) {
		tool->update(events);
	}

	for (auto& assignment : assignments) {
		assignment->update();
	}

	for (auto& s : objects) {
		s.transformUpdatedThisFrame = false;
		s.update();
	}

	for (auto& s : objects) {
		s.updateMatrix();
	}

	input.setEvents(events);
		
	updateTime = getTime() - nowish;
	lastFrameAt = nowish;
	frameCounter++;
}

void Application::render()
{
	if (!renderer) return; 

	auto t = getTime();

	renderer->ImGuiNewFrame();

	if (renderer->showImGui) {
		renderUI();
	}
	
	
	renderTime = renderer->Render(*this);

	renderer->endRender();

	renderTime = getTime() - t;

	//Profiler::get().add(rendererType._to_string(), Profiled{ renderTime, frameCounter });

	
}


void Application::renderUI()
{
	if (!renderer) return;

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Load")) {
				std::string loadFilename = Util::LoadFile({ "glTF file (.gltf)", "*.gltf", "json file (.json)", "*.json", "All files (*)", "*" }, IO::getAssetRoot());
				if (loadFilename != "") {
					gltf = GLTFImporter::import(loadFilename);
				}
				
			}
			if (ImGui::MenuItem("Exit")) {
				this->quit = true;
			}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
	

	ImGui::Begin(PROJECT_NAME);

	if (ImGui::CollapsingHeader("Stats"))
	{
		ImGui::Text("Frame counter: %lu, time since start: %.4f s", frameCounter, timeSinceStart);
		ImGui::Text("Update time: %.2f ms", updateTime * 1000);
		ImGui::Text("Render time: %.2f ms", renderTime * 1000);
		ImGui::Text("Delta time: %.2f ms", deltaTime * 1000);
		ImGui::Text("FPS (from delta time): %.2f", 1.0 / deltaTime);

		ImGui::InputDouble("Target FPS", &targetFPS);
		ImGui::InputDouble("Sleep time (ms)", &sleepTime);
	}

	if (ImGui::CollapsingHeader("Objects")) {
		IMDENT;

		static AnimationObjectType st = AnimationObjectType::sphere;
		renderEnumDropDown<AnimationObjectType>("Shape type", st);
		ImGui::SameLine();
		if (ImGui::Button("Add new")) {
			objects.push_back(AnimationObject(st));
		}
		if (ImGui::Button("Load OBJ")) {
			std::string loadFilename = Util::LoadFile({ "obj file (.obj)", "*.obj", "All files (*)", "*" }, IO::getAssetRoot());
			if (loadFilename != "") {
				//meshes.push_back(LoadOBJModel(loadFilename));
				objects.push_back(AnimationObject(loadFilename));
			}
		}

		static bool showSelected = false;
		ImGui::Checkbox("Show selected shapes only", &showSelected);

		size_t toDelete = objects.size() + 1;

		for (size_t i = 0; i < objects.size(); i++) {
			if (showSelected && objects[i].selected == false) continue;
			IMDENT;
			ImGui::PushID((const void*)&i);
			auto shapeText = fmt::format("{0}: Shape {1} ({2})", i, objects[i].index, objects[i].shapeType._to_string());
			if (ImGui::CollapsingHeader(shapeText.c_str())) {
				objects[i].renderUI();
				if (ImGui::Button("Delete")) {
					toDelete = i;
				}
			}
			ImGui::PopID();
			IMDONT;
		}

		if (toDelete >= 0 && toDelete < objects.size()) {
			auto d = objects.begin() + toDelete;
			objects.erase(d);
		}
	}

	Input::get().renderUI();

	if (ImGui::CollapsingHeader("Tools")) {
		IMDENT;
		for (auto & t : tools)
		{
			ImGui::PushID((const void*)t.get());
			if (ImGui::CollapsingHeader(t->name.c_str()))
			{
				t->renderUI();
			}
			ImGui::PopID();
		}
		IMDONT;
	}
	

	if (renderer) renderer->renderUI();

	AnimationObjectRenderer::get().renderUI();
	/*SimpleShapeRenderer::get().renderUI();
	LineRenderer::get().renderUI();*/


	//Input::get().renderUI();

	if (!assignments.empty()) {
		if (ImGui::CollapsingHeader("Assignments")) {
			IMDENT;
			for (auto& assignment : assignments) {
				ImGui::PushID((const void*)assignment.get());
				if (ImGui::CollapsingHeader(assignment->name.c_str())) {
					IMDENT;
					assignment->renderUI();
					IMDONT;
				}
				ImGui::PopID();
			}

			IMDONT;
		}
	}

	if (gltf) {
		if (ImGui::CollapsingHeader("GLTF")) {
			GLTFRenderer::get().renderUI(gltf);
		}
	}

	if (console && renderer)
	{
		static uvec2 screenSize = renderer->resolution;
		static vec2 consoleSize = vec2(screenSize.x * 0.3f, screenSize.y * 0.4f);

		ImGui::SetNextWindowSize({ consoleSize.x, consoleSize.y }, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos({ (screenSize.x - consoleSize.x) * 0.5f, screenSize.y - consoleSize.y * 1.1f },
			ImGuiCond_FirstUseEver);
		console->Draw("Console");
	}

	ImGui::End();

	//ECS::get().updateUI();
}

void Application::selectObject(int shapeIndex) {
	bool found = false;

	AnimationObject* foundObj = nullptr;
	for (auto& s : objects) {
		if (s.index == shapeIndex && s.selectable) {
			foundObj = &s;
			break;
		}
	}

	if (foundObj == nullptr) {
		for (auto& a : assignments) {
			auto aObjects = a->getObjects();
			for (auto& ao : aObjects) {
				if (ao->index == shapeIndex && ao->selectable) {
					foundObj = ao;
					break;
				}
			}
		}
	}

	if (foundObj) {
		foundObj->selected = !foundObj->selected;

		log("{0} shape {1}\n", foundObj->selected ? "selecting" : "deselecting", foundObj->index);

		auto so = std::find(selectionOrder.begin(), selectionOrder.end(), foundObj);
		if (so != selectionOrder.end()) selectionOrder.erase(so);

		if (foundObj->selected) {
			selectionOrder.push_back(foundObj);
		}
		else {
			auto si = std::find(selectionOrder.begin(), selectionOrder.end(), foundObj);
			if (si != selectionOrder.end()) {
				selectionOrder.erase(si);
			}
		}
	}
}

std::vector<AnimationObject*> Application::getObjects() {
	std::vector<AnimationObject*> objectPtrs;
	for (auto& a : assignments) {
		auto aObjects = a->getObjects();
		if (aObjects.size() > 0) {
			objectPtrs.insert(objectPtrs.end(), aObjects.begin(), aObjects.end());
		}
	}
	return objectPtrs;
}