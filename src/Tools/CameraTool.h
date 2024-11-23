#pragma once

#include "Tool.h"
#include "Camera.h"

#include <map>

struct AnimationObject;

class CameraTool : public Tool
{
	public:
		class CameraToolAction : public ToolAction
		{
			public:
				CameraTool * cameraTool = nullptr;
				Camera before;
				Camera after;

				CameraToolAction() { }

				CameraToolAction(const CameraToolAction & cta)
					: cameraTool(cta.cameraTool), before(cta.before),
					after(cta.after)
				{

				}

				virtual ~CameraToolAction() { }

				virtual void apply();
				virtual void undo();
		};

		Camera * camera = nullptr;

		bool isOrbiting = false;
		bool isPanning = false;
		bool isZooming = false;
		bool isFollowing = false;

		std::vector<AnimationObject*> followedObjects;

		CameraToolAction pendingAction;

		CameraTool(Camera * cam = nullptr);

		virtual ~CameraTool() { }

		virtual EventActions defaultEvents();

		void updateClick(const InputEvent & ie);

		virtual void update(std::vector<InputEvent> & events);

		void centerFocus();

		virtual void addAction();

		virtual void activate();
		virtual void deactivate();

		virtual void renderUI();

	private:
		std::map<Tool*, bool> toolStates;

		vec3 cameraOffset = vec3(0.f, 0.f, -1.f);
};