// CMPS 4480 Lab 05
// Name: 

#include "Lab05.h"
#include "Application.h"
#include "AnimationObjectRenderer.h"
#include "InputOutput.h"
#include "Lighting.h"
#include "Prompts.h"
#include "UIHelpers.h"

#include <cassert>

#include <glm/gtc/random.hpp>

#include "implot.h"


// Variables for Lab 05
namespace cmps_4480_lab_05 {
	// TODO: change this to not be a nullptr
	const char* yourName = "Nick Toothman";

	// Type definitions for Lab 05

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
		// Pointer to object
		AnimationObject* object = nullptr;

		void recomputePtr(AnimationObject * newObject = nullptr) {
			if (newObject) object = newObject;
			switch (target) {
			case InterpTarget::PosX:
				ptr = &(object->localPosition.x);
				break;
			case InterpTarget::PosY:
				ptr = &(object->localPosition.y);
				break;
			case InterpTarget::PosZ:
				ptr = &(object->localPosition.z);
				break;
			case InterpTarget::RotX:
				ptr = &(object->localRotation.x);
				break;
			case InterpTarget::RotY:
				ptr = &(object->localRotation.y);
				break;
			case InterpTarget::RotZ:
				ptr = &(object->localRotation.z);
				break;
			case InterpTarget::ScaleX:
				ptr = &(object->localScale.x);
				break;
			case InterpTarget::ScaleY:
				ptr = &(object->localScale.y);
				break;
			case InterpTarget::ScaleZ:
				ptr = &(object->localScale.z);
				break;
			case InterpTarget::ColorR:
				ptr = &(object->color.r);
				break;
			case InterpTarget::ColorG:
				ptr = &(object->color.g);
				break;
			case InterpTarget::ColorB:
				ptr = &(object->color.b);
				break;
			case InterpTarget::ColorA:
				ptr = &(object->color.a);
				break;
			}
		}
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
	// selects a random value and multiplies it by t to make an erratic, 
	// unstable growth over time.
	float customInterp(float t) {
		return glm::linearRand<float>(0, 1) * t;
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
		command.recomputePtr(&object);
		auto& commands = interpCommands[object.index];
		commands.push_back(command);
	}

	std::string defaultName() {
		return fmt::format("cmps-4480-Lab05-{0}.json", yourName);
	}
}

using namespace cmps_4480_lab_05;

void Lab05::init() {
	assert(yourName != nullptr);

	initialized = true;
}

void Lab05::update() {
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

	for (auto& object : interpCommands) {
		for (auto& command : object.second) {
			float t = (currentFrame - command.start.frame) / (float)(command.end.frame - command.start.frame);
			if (t >= 0 && t <= 1) {
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
void Lab05::render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow) {
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


// Return value: 0 for no change, 1 for delete, 2 for duplicate
int renderInterpCommandUI(InterpCommand& cmd, bool showDelete = false) {
	ImGui::PushID((const void*)&cmd);
	if (renderEnumDropDown<InterpTarget>("Target", cmd.target)) {
		cmd.recomputePtr();
	}
	ImGui::InputInt("Start frame", &cmd.start.frame);
	ImGui::InputFloat("Start value", &cmd.start.value);
	ImGui::InputInt("End frame", &cmd.end.frame);
	ImGui::InputFloat("End value", &cmd.end.value);
	renderEnumDropDown<EasingFunctionType>("Easing", cmd.easing);

	int ret = 0;
	if (showDelete) {
		if (ImGui::Button("Delete")) {
			ret = 1;
		}
		ImGui::SameLine();
		if (ImGui::Button("Duplicate")) {
			ret = 2;
		}
	}

	ImGui::PopID();
	return ret;
	
}

void renderNewerInterpCommandUI(AnimationObject & object) {
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
void Lab05::renderUI() {

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

	ImGui::Begin("Playbar");
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

	ImGui::End();

	if (ImGui::Button("Add object")) {
		objects.push_back(AnimationObject(AnimationObjectType::sphere));
	}

	if (ImGui::CollapsingHeader("Objects")) {
		IMDENT;
		static bool selectedOnly = false;
		ImGui::Checkbox("Selected objects only", &selectedOnly);
		for (auto& o : objects) {
			if (selectedOnly && !o.selected) continue;
			std::string label = fmt::format("Object {0}", o.index);
			if (ImGui::CollapsingHeader(label.c_str())) {
				IMDENT;
				o.renderUI();
				ImGui::Separator();
				renderNewerInterpCommandUI(o);
				ImGui::Separator();
				auto& commands = interpCommands[o.index];
				if (!commands.empty()) {
					if (ImGui::CollapsingHeader("Interpolations")) {
						auto toDelete = commands.end();
						auto toCopy = commands.end();

						int c = 0;
						for (auto it = commands.begin(); it != commands.end(); ++it) {

							ImGui::Separator();
							ImGui::Text("Interpolation %d: %s: %.2f (%d) to %.2f (%d)", c++, it->target._to_string(), it->start.value,
								it->start.frame, it->end.value, it->end.frame);
							int whatToDo = renderInterpCommandUI(*it, true);
							if (whatToDo == 1) {
								toDelete = it;
								break;
							}
							else if (whatToDo == 2) {
								toCopy = it;
								break;
							}
						}

						if (toDelete != commands.end()) {
							commands.erase(toDelete);
						}
						else if (toCopy != commands.end()) {
							InterpCommand newCmd = *toCopy;
							if (newCmd.object) {
								addInterpCommandToObject(*newCmd.object, newCmd);
							}
						}
					}
				}
				IMDONT;
			}
			
		}
		IMDONT;
	}
}

ptr_vector<AnimationObject> Lab05::getObjects() {
	ptr_vector<AnimationObject> ptrs;

	for (auto& o : objects) {
		ptrs.push_back(&o);
	}

	return ptrs;
}


void Lab05::saveScene() {
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

void Lab05::loadScene() {
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
			if (object.parentIndex >= 0) {
				for (auto& other : objects) {
					if (&other == &object) continue;
					if (other.index == object.parentIndex) {
						object.setParent(&other, false);
						break;
					}
				}
			}
		}

		interpCommands.clear();

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