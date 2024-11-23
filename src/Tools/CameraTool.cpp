#include "CameraTool.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Renderer.h"
#include "Application.h"

#include "imgui.h"

void CameraTool::CameraToolAction::undo()
{
	cameraTool->camera->copyFrom(before);
	cameraTool->camera->change(true);
}

void CameraTool::CameraToolAction::apply()
{
	cameraTool->camera->copyFrom(after);
	cameraTool->camera->change(true);
}

CameraTool::CameraTool(Camera * cam) : Tool("CameraTool", defaultEvents()),
	camera(cam)
{
	pendingAction.tool = this;
	pendingAction.cameraTool = this;
}

void CameraTool::addAction()
{
	pendingAction.after = *camera;

	if (pendingAction.before != pendingAction.after)
	{
		createAction(s_ptr<CameraToolAction>(new CameraToolAction(pendingAction)));
	}
}

void CameraTool::updateClick(const InputEvent & ie)
{
	bool action = ie.action != GLFW_RELEASE;

	if (action)
	{
		pendingAction.before = *camera;
	}


	switch (ie.button)
	{
		case GLFW_MOUSE_BUTTON_LEFT:
			isOrbiting = action;
			break;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			isPanning = action;
			break;
		case GLFW_MOUSE_BUTTON_RIGHT:
			isZooming = action;
			break;
		default:
			break;
	}

	//if (!action)
	//{
	//	addAction();
	//}
}

Tool::EventActions CameraTool::defaultEvents()
{
	EventActions ea = Tool::defaultEvents();

	int cameraKey = GLFW_KEY_LEFT_ALT;

	int camOrbitKey = GLFW_MOUSE_BUTTON_LEFT;
	int camZoomKey = GLFW_MOUSE_BUTTON_RIGHT;
	int camPanKey = GLFW_MOUSE_BUTTON_MIDDLE;

	ea.push_back(Tool::InputActions(InputEvent::KeyEvent(cameraKey),
		[](Tool * tool, const InputEvent & ie)
	{
		//if (UIContext::get().wantsFocus()) return false;
		if (CameraTool * ct = (CameraTool*)tool)
		{
			if (ie.action == GLFW_PRESS)
			{
				ct->activate();
				//glfwSetInputMode(Application::get().window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				return true;
			}
			else if (ie.action == GLFW_RELEASE)
			{
				ct->deactivate();
				//glfwSetInputMode(Application::get().window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				return true;
			}
		}
		return false;
	}));

	ea.push_back(Tool::InputActions(InputEvent::KeyEvent(GLFW_KEY_F, GLFW_PRESS),
		[this](Tool* tool, const InputEvent& ie)
		{
			followedObjects = Application::get().getSelectedObjects();
			isFollowing = ie.mods == 1 || ie.mods == 2;
			centerFocus();
			return true;
		}));

	// Mouse controls
	ea.push_back(Tool::InputActions(InputEvent::ClickEvent(),
		[](Tool* tool, const InputEvent & ie)
	{
		//if (UIContext::get().wantsFocus()) return false;
		if (tool->active)
		{
			if (CameraTool * ct = (CameraTool*)tool)
			{
				ct->updateClick(ie);
				return true;
			}
		}
		return false;
	}));

	ea.push_back(Tool::InputActions(InputEvent::MoveEvent(),
		[this](Tool * tool, const InputEvent & ie)
	{
		//if (UIContext::get().wantsFocus()) return false;
		if (!tool->active) return false;

		vec2 d = (ie.pos - Input::get().last.mousePos) * 0.25f;

		

		if (d == vec2(0)) return false;

		float dx = d.x;
		float dy = d.y;

		if (isOrbiting)
		{
			camera->orbit((-dx * glm::pi<float>() / 180.0f), (-dy * glm::pi<float>() / 180.0f));
		}

		if (isZooming)
		{
			camera->zoom((float)-dx);
			camera->zoom((float)-dy);
		}

		if (isPanning)
		{
			camera->pan((float)-dx, (float)dy);
		}

		/*if (camera->isOrbiting || camera->isZooming || camera->isPanning)
		{
			Aestus::shouldTracePaths = true;
		}*/

		return true;
	}));

	ea.push_back(Tool::InputActions(InputEvent::KeyEvent(GLFW_KEY_ESCAPE, GLFW_PRESS),
		[](Tool * tool, const InputEvent & ie)
	{
		//if (UIContext::get().wantsFocus()) return false;
		if (CameraTool * ct = (CameraTool*)tool)
		{
			if (ct->showUI)
			{
				ct->showUI = false;
				return true;
			}
		}

		return false;
	}));

	return ea;
}

void CameraTool::activate()
{
	if (!active)
	{
		//toolStates = Toolset::get().getToolStates();

		Tool::activate();
	}
}

void CameraTool::deactivate()
{
	if (active)
	{
		//Toolset::get().setToolStates(toolStates);

		isOrbiting = false;
		isPanning = false;
		isZooming = false;

		Tool::deactivate();
	}
}

void CameraTool::renderUI()
{
	ImGui::Checkbox("Active", &active);
	auto cam = camera;


	ImGui::InputFloat3("Camera offset", glm::value_ptr(cameraOffset));
	ImGui::InputFloat3("Position", glm::value_ptr(cam->Position));
	ImGui::InputFloat3("Lookat", glm::value_ptr(cam->Lookat));
	ImGui::SliderFloat("Distance", &cam->distance, cam->distanceMinMax.x, cam->distanceMinMax.y);
	ImGui::InputFloat2("Distance limits", glm::value_ptr(cam->distanceMinMax));
	ImGui::InputFloat("Horizontal angle", &cam->horizontalAngle);
	ImGui::InputFloat("Vertical angle", &cam->verticalAngle);
	ImGui::InputFloat("Pan scale", &cam->panScale);

	ImGui::Checkbox("Orbit", &isOrbiting);
	ImGui::Checkbox("Pan", &isPanning);
	ImGui::Checkbox("Zoom", &isZooming);
	

	if (ImGui::Button(cam->projectionType._to_string()))
	{
		cam->projectionType = ProjectionType::_from_integral(1 - cam->projectionType._value);
		cam->change(true);
	}

	

	// Buttons for axes
	// i : x, y, z
	for (int i = 0; i < 3; i++)
	{
		// j : positive, negative
		for (int j = 0; j < 2; j++)
		{
			std::string ijName = fmt::format("Align to {0}{1}", j % 2 == 0 ? "-" : "", (char)((int)'X' + i));

			vec3 align = vec3(0);
			align[i] = (j % 2 == 0 ? 1 : -1);

			if (ImGui::Button(ijName.c_str()))
			{
				cam->alignToAxis(align);
				cam->change(true);
			}

			if (j < 1)
			{
				ImGui::SameLine();
			}
		}
	}

	if (ImGui::InputFloat("Near plane", &cam->nearFar.x))
	{
		cam->change(true);
	}

	if (ImGui::InputFloat("Far plane", &cam->nearFar.y))
	{
		cam->change(true);
	}
}

void CameraTool::update(std::vector<InputEvent> & events)
{
	Tool::update(events);
	if (isFollowing) {
		centerFocus();
	}
}

void CameraTool::centerFocus() {

	if (followedObjects.empty()) {
		isFollowing = false;
		return;
	}

	vec3 newFocusAt(0.f);

	for (auto& s : followedObjects) {
		newFocusAt += s->position;
	}
	newFocusAt *= (1.f / (float)followedObjects.size());
	auto& cam = Application::get().getRenderer<Renderer>()->camera;
	cam.Lookat = newFocusAt;
	cam.update();
}