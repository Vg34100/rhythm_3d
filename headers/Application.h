#pragma once

#include "globals.h"
#include "AnimationObject.h"

class Mover;
class Renderer;
class ImGuiConsole;
class FullscreenFilter;
struct Tool;

struct GLFWwindow;

// Forward declarations
class Assignment;

struct GLTFData;

class Application
{
	public:
		GLFWwindow * window = nullptr;
		ivec2 windowSize;

		bool quit = false;

		double startedAt = 0;
		double lastFrameAt = 0;
		double pausedAt = 0;
		double resumedAt = 0;

		double totalPauseTime = 0.;
		double timeSinceStart = 0.;
		double deltaTime = 0.;
		double updateTime = 0.;
		double renderTime = 0.;
		double targetFPS = 60.;
		double sleepTime = 0.;

		unsigned long frameCounter = 0;

		static Application& get()
		{
			static Application g;
			return g;
		}

		s_ptr<Renderer> renderer;

		std::vector<Command> commands;

		// A list of assignments
		s_ptr_vector<Assignment> assignments;

		// Objects for animation
		std::vector<AnimationObject> objects;

		// GLTF Data
		s_ptr<GLTFData> gltf;

		// A list of tools
		s_ptr_vector<Tool> tools;

		// Path recording variables (lab 02)
		std::vector<vec3> framePositions;
		bool isSaving = false;
		bool isPlaying = false;
		size_t playbackFrame = 0;

		// Lab 04 variables
		// Use this variable to keep track of the current animation frame. Range: 0 to maxAnimationFrames
		int currentAnimationFrame = 0;
		// Use this variable to keep track of the maximum frame range. 
		int maxAnimationFrames = 100;
		// How many frames of animation to apply per second.
		float animationFPS = 60.f;
		// How many seconds in between frames of animation
		float animationSPF = 1.f / animationFPS;
		// Use to track the time (in seconds) when currentAnimationFrame was last changed
		double lastAnimationFrameChange = 0;
		bool animationTimeContinuous = false;
		bool animationPaused = false;

		void init(GLFWwindow * w = nullptr);

		virtual void update();
		virtual void render();
		virtual void renderUI();


		virtual void addCommand(const Command& c) {
			commands.push_back(c);
		}

		template <typename T>
		s_ptr<T> getRenderer() {
			return std::dynamic_pointer_cast<T>(renderer);
		}

		template<typename T>
		s_ptr<T> getTool() {
			for (auto& tool : tools) {
				if (auto tt = std::dynamic_pointer_cast<T>(tool)) {
					return tt;
				}
			}

			return nullptr;
		}

		void selectObject(int shapeIndex);

		// First entry was selected before all others.
		// Last entry was selected after all others.
		std::vector<AnimationObject*> selectionOrder;

		std::vector<AnimationObject*> getSelectedObjects() {
			return selectionOrder;
			/*std::vector<AnimationObject*> selectedShapes;
			for (auto& s : objects) {
				if (s.selected) {
					selectedShapes.push_back(&s);
				}
			}

			return selectedShapes;*/
		}

		AnimationObject* getObject(int index) {
			for (auto& s : objects) {
				if (s.index == index) return &s;
			}
			return nullptr;
		}

		std::vector<AnimationObject*> getObjects();

		s_ptr<ImGuiConsole> console;
};
