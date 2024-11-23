#include "SelectTool.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Application.h"
#include "Framebuffer.h"
#include "Input.h"
#include "Renderer.h"
#include "Texture.h"
#include "imgui.h"

Tool::EventActions SelectTool::defaultEvents() {
	EventActions ea = Tool::defaultEvents();

	ea.push_back(Tool::InputActions(InputEvent::KeyEvent(GLFW_KEY_S, GLFW_PRESS),
		[this](Tool* tool, const InputEvent& ie)
		{
			if (ie.mods  == GLFW_MOD_CONTROL) {
				return false;
			}
			if (active) {
				deactivate();
			}
			else {
				activate();
			}
			log("{0}: {1}\n", name, (active ? "active" : "inactive"));
			return true;
		}));

	// Mouse controls
	ea.push_back(Tool::InputActions(InputEvent::ClickEvent(GLFW_MOUSE_BUTTON_LEFT),
		[this](Tool* tool, const InputEvent& ie)
		{
			if (!active) return false;
			selecting = (ie.action == GLFW_PRESS);
			return true;
		}));

	return ea;
}

void SelectTool::update(std::vector<InputEvent>& events) {
	Tool::update(events);

	if (!active) return;

	if (selecting && !lastFrameSelecting) {

		auto ogl = Application::get().getRenderer<OpenGLRenderer>();
		auto& input = Input::get();

		ivec2 nextMousePos = input.current.mousePos;

		if (mousePos != nextMousePos) {
			mousePos = nextMousePos;
			int w = 0, h = 0;
			glfwGetWindowSize(Application::get().window, &w, &h);
			mousePos.y = h - mousePos.y;

			vec2 texCoord = vec2((float)mousePos.x / w, (float)mousePos.y / h);

			auto mainTex = ogl->gbuffer->textures[0];
			mainTex->copyToMemory();
			//currentColorData = mainTex->getColor(mousePos);
			currentColorData = mainTex->getColorUV(texCoord);

			auto primTex = ogl->gbuffer->textures[1];
			primTex->copyToMemory();
			//currentSelectData = primTex->getColor(mousePos);
			currentSelectData = primTex->getColorUV(texCoord);

			int shapeID = (int)currentSelectData.x;

			Application::get().selectObject(shapeID);
		}
	}

	lastFrameSelecting = selecting;
}

void SelectTool::renderUI() {
	ImGui::Checkbox("Active", &active);
	ImGui::Checkbox("Selecting", &selecting);

	ImGui::InputInt("Texture channel to read from", &textureToReadFrom);
	ImGui::InputInt2("Mouse pos", glm::value_ptr(mousePos));

	ImGui::ColorEdit4("Current color data", glm::value_ptr(currentColorData));

	ImGui::InputInt4("Current select data", glm::value_ptr(currentSelectData));
}