// CMPS 4480 Lab 04
// Name: 

#include "Lab04.h"
#include "Application.h"
#include "AnimationObjectRenderer.h"
#include "InputOutput.h"
#include "Lighting.h"
#include "Prompts.h"
#include "UIHelpers.h"

#include <cassert>

#include <glm/gtc/random.hpp>


// Variables for Lab 04
namespace cmps_4480_lab_04 {
	// TODO: change this to not be a nullptr
	const char* yourName = "Nick Toothman";

	// Type definitions for lab 04

// Enum to choose a type of easing function
	MAKE_ENUM(EasingFunctionType, int, StepMin, StepMax, Linear, Hermite, BounceOut, Custom);

	// Enum to help select a target for interpolation
	MAKE_ENUM(InterpTarget, int, PosX, PosY, PosZ, RotX, RotY, RotZ, ScaleX, ScaleY, ScaleZ, ColorR, ColorG, ColorB, ColorA);


	// A keyframe is just a designated frame number and value
	struct Keyframe {
		int frame = 0;
		float value = 0;
	};

	// An interpolation command has:
	// keyframes for the start and end values,
	// a function to transform t as it changes between 0 and 1
	// and a pointer to the variable it is modifying. 
	struct InterpCommand {
		Keyframe start = { 0, 0.0f };
		Keyframe end = { 0, 0.0f };
		EasingFunctionType easing = EasingFunctionType::Linear;
		InterpTarget target = InterpTarget::PosX;
		int objectId = 0;
		// Pointer to target
		float* ptr = nullptr;
	};

	// Helper functions to represent interpolation presets as json
	void to_json(json& j, const Keyframe frame) {
		j = json{ { "frame", frame.frame }, { "value", frame.value } };
	}

	void from_json(const json& j, Keyframe& frame) {
		j.at("frame").get_to(frame.frame);
		j.at("value").get_to(frame.value);
	}

	void to_json(json& j, const InterpCommand& cmd) {
		j = json{
			{ "objectId", cmd.objectId },
			{ "start", cmd.start },
			{ "end", cmd.end },
			{ "easing", cmd.easing },
			{ "target", cmd.target }
		};
	}

	void from_json(const json& j, InterpCommand& cmd) {
		j.at("objectId").get_to(cmd.objectId);
		j.at("start").get_to(cmd.start);
		j.at("end").get_to(cmd.end);
		j.at("easing").get_to(cmd.easing);
		j.at("target").get_to(cmd.target);
	}


	// Easing functions: stepmin, stepmax, hermite, bounceOut, and customInterp
	// The following functions all use the same signature: 
	// Input value: float t, in the range of [0, 1]
	// Return value: result of interpolation between 0 and 1 using t. 

	// Returns the minimum value except for when t >= 1
	float stepmin(float t) {
		if (t >= 1.f) return 1.0f;
		return 0.0f;
	}

	// Returns the maximum value except for when t <= 0
	float stepmax(float t) {
		if (t <= 0.f) return 0.0f;
		return 1.0f;
	}

	// Linear interpolation of t between 0 and 1 is... just t
	float linear(float t) {
		return t;
	}

	// Returns the smoothed hermite interpolation between 0 and 1
	float hermite(float t) {
		if (t <= 0.f) return 0.0f;
		if (t >= 1.f) return 1.0f;

		return t * t * (3.f - 2.f * t);
	}

	// Returns a bounce toward the target that repeats and settles
	float bounceOut(float t)
	{
		const float a = 4.0 / 11.0;
		const float b = 8.0 / 11.0;
		const float c = 9.0 / 10.0;

		const float ca = 4356.0 / 361.0;
		const float cb = 35442.0 / 1805.0;
		const float cc = 16061.0 / 1805.0;

		float t2 = t * t;

		return t < a
			? 7.5625 * t2
			: t < b
			? 9.075 * t2 - 9.9 * t + 3.4
			: t < c
			? ca * t2 - cb * t + cc
			: 10.8 * t * t - 20.52 * t + 10.72;
	}

	// Implement your own easing function here!
	float customInterp(float t) {
		return 0;
	}

	// Alias for functions that accept a float and return a float
	using EasingFunction = float(*)(float);

	std::map<EasingFunctionType, EasingFunction> easingFunctions = {
		{ EasingFunctionType::StepMin, stepmin },
		{ EasingFunctionType::StepMax, stepmax },
		{ EasingFunctionType::Linear, linear },
		{ EasingFunctionType::Hermite, hermite },
		{ EasingFunctionType::BounceOut, bounceOut },
		{ EasingFunctionType::Custom, customInterp }
	};

	// Linear interpolation function: computes an intermediate value
	// between min and max using t in [0, 1] for a weighted average.
	// Does NOT assume min == 0 and max == 1
	float lerp(float min, float max, float t) {
		return t * max + (1.0f - t) * min;
	}

	// Variables for time and frame progression
	int numFrames = 240;
	int currentFrame = 0;
	int FPS = 24;					// frames per second
	double SPF = 1.0 / 24;			// seconds per frame (1/FPS)
	double lastFrameChange = 0;		// timestamp of the last frame change
	double fixedSecondsElapsed = 0;

	// Variables for play and interpolation state
	bool isPlaying = false;
	bool useDefaultInterpolations = true;
	bool useCustomInterpolations = false;

	InterpCommand ballBounce = {
		{ 0, 0.0f },				// the start keyframe
		{ numFrames, -5.0f }		// the end keyframe
	};

	// Objects to animate
	std::vector<AnimationObject> objects;
	// Interpolation commands for these objects
	// Key is object's index value, value is list of interpolations to apply to object
	std::map<int, std::vector<InterpCommand>> interpCommands;

	

	void addInterpCommandToObject(AnimationObject& object, InterpCommand command) {
		switch (command.target) {
			case InterpTarget::PosX:
				command.ptr = &(object.localPosition.x);
				break;
			case InterpTarget::PosY:
				command.ptr = &(object.localPosition.y);
				break;
			case InterpTarget::PosZ:
				command.ptr = &(object.localPosition.z);
				break;
			case InterpTarget::RotX:
				command.ptr = &(object.localRotation.x);
				break;
			case InterpTarget::RotY:
				command.ptr = &(object.localRotation.y);
				break;
			case InterpTarget::RotZ:
				command.ptr = &(object.localRotation.z);
				break;
			case InterpTarget::ScaleX:
				command.ptr = &(object.localScale.x);
				break;
			case InterpTarget::ScaleY:
				command.ptr = &(object.localScale.y);
				break;
			case InterpTarget::ScaleZ:
				command.ptr = &(object.localScale.z);
				break;
			case InterpTarget::ColorR:
				command.ptr = &(object.color.r);
				break;
			case InterpTarget::ColorG:
				command.ptr = &(object.color.g);
				break;
			case InterpTarget::ColorB:
				command.ptr = &(object.color.b);
				break;
			case InterpTarget::ColorA:
				command.ptr = &(object.color.a);
				break;
		}
		auto& commands = interpCommands[object.index];
		commands.push_back(command);
	}

	std::string defaultName() {
		return fmt::format("cmps-4480-lab04-{0}.json", yourName);
	}
}

using namespace cmps_4480_lab_04;

void Lab04::init() {
	assert(yourName != nullptr);

	float start = -5.0f;
	float dx = 2.0f;

	for (int i = 0; i < 6; i++) {
		vec3 color = vec3(glm::linearRand<float>(0, 1), glm::linearRand<float>(0, 1), glm::linearRand<float>(0, 1));
		objects.push_back(AnimationObject(AnimationObjectType::sphere, vec3(start + dx * i, 0, 0)));
		objects.back().color = vec4(color, 1.0f);
		
	}

	initialized = true;
}

void Lab04::update() {
	if (!initialized) init();

	double nowish = getTime();

	double dt = nowish - lastFrameChange;

	if (isPlaying) {
		if (dt >= SPF) {
			int framesToAdvance = glm::round(dt / SPF);
			framesToAdvance = glm::clamp<int>(framesToAdvance, 1, 20);
			fixedSecondsElapsed += SPF * framesToAdvance;
			currentFrame += framesToAdvance;

			if (currentFrame > numFrames) {
				currentFrame = 0;
			}

			lastFrameChange = nowish;
		}
	}

	if (useDefaultInterpolations) {
		// TODO: compute t from frame numbers
		float t = (currentFrame - ballBounce.start.frame) / (float)(ballBounce.end.frame - ballBounce.start.frame);

		// Linear interpolation on first object
		if (objects.size() > 0) {
			objects[0].localPosition.y = lerp(ballBounce.start.value, ballBounce.end.value, t);
		}

		// Easing functions for rest of objects
		for (int i = 0; i < objects.size(); i++) {
			EasingFunction f = easingFunctions[EasingFunctionType::_from_integral(i)];

			objects[i].localPosition.y = lerp(ballBounce.start.value, ballBounce.end.value, f(t));
		}
	}

	else if (useCustomInterpolations) {
		for (auto& object : interpCommands) {
			for (auto& command : object.second) {
				float t = (currentFrame - command.start.frame) / (float)(command.end.frame - command.start.frame);
				EasingFunction f = easingFunctions[command.easing];
				float ft = f(t);
				if (command.ptr) {
					*command.ptr = lerp(command.start.value, command.end.value, ft);
				}
			}
		}
	}
}

// Render the lab. This is called twice from the main renderer: once to make the shadow map, and again
// to render the scene using the shadow data.
void Lab04::render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow) {
	auto& jr = AnimationObjectRenderer::get();

	// Render all simple shapes
	for (auto st : AnimationObjectType::_values()) {
		if (st == +AnimationObjectType::model) continue;
		jr.beginBatchRender(st, false, vec4(1.f), isShadow);

		if (!isShadow) {
			GPU::Lighting::get().bind(jr.currentMesh->shader);
		}

		if (isShadow) {
			glEnable(GL_CULL_FACE);
			if (st == +AnimationObjectType::quad || st == +AnimationObjectType::tri) {
				glCullFace(GL_BACK);
			}
			else {
				glCullFace(GL_FRONT);
			}
		}

		for (auto & object : objects) {
			object.updateMatrix(true);
			if (isShadow && object.lightIndex != -1) continue;
			if (st == +AnimationObjectType::model) continue;
			if (object.shapeType == st) {
				jr.renderBatchWithOwnColor(object, isShadow);
			}
		}

		jr.endBatchRender(isShadow);
	}
}

bool renderInterpCommandUI(InterpCommand& cmd, bool showDelete = false) {
	ImGui::PushID((const void*)&cmd);
	renderEnumDropDown<InterpTarget>("Target", cmd.target);
	ImGui::InputInt("Start frame", &cmd.start.frame);
	ImGui::InputFloat("Start value", &cmd.start.value);
	ImGui::InputInt("End frame", &cmd.end.frame);
	ImGui::InputFloat("End value", &cmd.end.value);
	renderEnumDropDown<EasingFunctionType>("Easing", cmd.easing);
	if (showDelete && ImGui::Button("Delete")) {
		ImGui::PopID();
		return true;
	}
	else {
		ImGui::PopID();
		return false;
	}
	
}

void renderNewInterpCommandUI(AnimationObject & object) {
	static InterpCommand newCmd;

	ImGui::PushID((const void*)&object);

	renderInterpCommandUI(newCmd);

	if (ImGui::Button("Add interpolation")) {
		newCmd.objectId = object.index;
		addInterpCommandToObject(object, newCmd);
		newCmd = InterpCommand();
	}

	ImGui::PopID();
}

// Renders the UI controls for the lab.
void Lab04::renderUI() {

	if (ImGui::Button("Save scene")) {
		saveScene();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load scene")) {
		loadScene();
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear scene")) {
		objects.clear();
	}

	ImGui::Checkbox("Default interpolations", &useDefaultInterpolations);

	if (useDefaultInterpolations) {
		if (ImGui::CollapsingHeader("Ball bounce interpolation")) {
			renderInterpCommandUI(ballBounce);
		}
		
	}
	else {
		ImGui::Checkbox("Custom interpolations", &useCustomInterpolations);
	}

	// Frame control UI
	//// Beginning
	if (ImGui::Button("<<")) {
		currentFrame = 0;
	}
	ImGui::SameLine();
	// Previous frame
	if (ImGui::Button("<")) {
		currentFrame--;
		if (currentFrame < 0) currentFrame = 0;
	}
	ImGui::SameLine();
	//// Play/stop button
	if (isPlaying) {
		if (ImGui::Button("Stop")) {
			isPlaying = false;
		}
	}
	else {
		if (ImGui::Button("Play")) {
			isPlaying = true;
			lastFrameChange = getTime();
		}
	}
	ImGui::SameLine();
	//// Next frame
	if (ImGui::Button(">")) {
		currentFrame++;
		if (currentFrame > numFrames) currentFrame = numFrames;
	}
	ImGui::SameLine();
	//// End
	if (ImGui::Button(">>")) {
		currentFrame = numFrames;
		isPlaying = false;
	}

	

	// Slider for playback
	ImGui::SliderInt("Frame position", &currentFrame, 0, numFrames);

	int fps = FPS;
	if (ImGui::InputInt("FPS", &fps)) {
		if (fps > 0) {
			FPS = fps;
			SPF = 1.0 / (double)FPS;
		}
	}
	ImGui::SameLine();

	ImGui::Text("SPF: %.2f", SPF);

	ImGui::Separator();
	if (ImGui::Button("Add object")) {
		objects.push_back(AnimationObject(AnimationObjectType::sphere));
	}

	if (ImGui::CollapsingHeader("Objects")) {
		IMDENT;
		int i = 0;
		for (auto& o : objects) {
			if (i > 100) break;
			std::string label = fmt::format("Object {0}", o.index);
			if (ImGui::CollapsingHeader(label.c_str())) {
				IMDENT;
				o.renderUI();
				if (useCustomInterpolations) {
					ImGui::Separator();
					renderNewInterpCommandUI(o);
				}
				auto& commands = interpCommands[o.index];
				if (!commands.empty()) {
					if (ImGui::CollapsingHeader("Interpolations")) {
						auto toDelete = commands.end();

						int c = 0;
						for (auto it = commands.begin(); it != commands.end(); ++it) {

							ImGui::Separator();
							ImGui::Text("Interpolation %d", c++);
							if (renderInterpCommandUI(*it, true)) {
								toDelete = it;
								break;
							}
						}

						if (toDelete != commands.end()) {
							commands.erase(toDelete);
						}
					}
				}
				IMDONT;
			}
			
		}
		IMDONT;
	}
}

ptr_vector<AnimationObject> Lab04::getObjects() {
	ptr_vector<AnimationObject> ptrs;

	for (auto& o : objects) {
		ptrs.push_back(&o);
	}

	return ptrs;
}


void Lab04::saveScene() {
	std::string saveFilename = Util::SaveFile({ "json file (.json)", "*.json", "All files (*)", "*" }, IO::getAssetRoot(), defaultName());
	if (saveFilename != "") {
		json saveData;
		auto& configData = saveData["config"];
		configData["name"] = yourName;
		configData["numFrames"] = numFrames;
		configData["FPS"] = FPS;

		auto& objectData = saveData["objects"];
		for (auto& obj : objects) {
			objectData.push_back(obj);
		}

		auto& interpData = saveData["interpolations"];
		for (auto& objectCommands : interpCommands) {
			for (const auto& command : objectCommands.second) {
				interpData.push_back(command);
			}
		}
		/*configData["maxAnimationFrames"] = maxAnimationFrames;
		configData["animationFPS"] = animationFPS;
		auto& shapesData = saveData["shapes"];
		for (auto& s : shapes) {
			shapesData.push_back(s);
		}*/

		/*auto& animData = saveData["animations"];
		for (auto& a : animations) {
			animData.push_back(a);
		}*/

		std::ofstream fout(saveFilename);

		fout << saveData.dump(4) << std::endl;
	}
}

void Lab04::loadScene() {
	std::string loadFilename = Util::LoadFile({ "json file (.json)", "*.json", "All files (*)", "*" }, IO::getAssetRoot());
	if (loadFilename != "") {
		std::ifstream fin(loadFilename);
		json loadData;
		fin >> loadData;

		try {
			auto& configData = loadData["config"];
			configData.at("numFrames").get_to(numFrames);
			configData.at("FPS").get_to(FPS);
			SPF = 1.0 / FPS;
		}
		catch (json::type_error te) {
			log("{0}\n", te.what());
		}
		catch (json::out_of_range oor) {
			log("{0}\n", oor.what());
		}

		objects.clear();

		for (auto& js : loadData["objects"]) {
			objects.push_back(js);
			objects.back().updateMatrix();
		}

		for (auto& object: objects) {
			if (object.parentIndex >= 0 && object.parentIndex < objects.size() && object.parent == nullptr) {
				object.setParent(&objects[object.parentIndex], false);
			}
		}

		for (auto& interp : loadData["interpolations"]) {
			InterpCommand fileCmd = interp;
			// Find object by this id
			for (auto& obj : objects) {
				if (obj.index == fileCmd.objectId) {
					addInterpCommandToObject(obj, fileCmd);
					break;
				}
			}
		}

		//for (auto& ja : loadData["animations"]) {
		//	animations.push_back(ja);
		//	animations.back().updatePointers();
		//}
	}
}