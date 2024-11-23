#include "TransformTool.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Application.h"
#include "AnimationGlobals.h"
#include "Framebuffer.h"
#include "Input.h"
#include "Renderer.h"
#include "Texture.h"
#include "imgui.h"
#include "UIHelpers.h"


#include "implot.h"

Tool::EventActions TransformTool::defaultEvents() {
	EventActions ea = Tool::defaultEvents();

	ea.push_back(Tool::InputActions(InputEvent::KeyEvent(),
		[&](Tool* tool, const InputEvent& ie)
		{
			// Peer-pressured by the shortcuts in Unity and Maya
			const int tfTranslate = GLFW_KEY_W;
			const int tfRotate = GLFW_KEY_E;
			const int tfScale = GLFW_KEY_R;

			if (ie.action == GLFW_PRESS) {
				bool handled = false;
				switch (ie.button) {
					case tfTranslate:
						if (mode != +TransformToolMode::Translate) {
							mode = TransformToolMode::Translate;
						}
						else {
							mode = TransformToolMode::Off;
						}
						handled = true;
						break;
					case tfRotate:
						if (mode != +TransformToolMode::Rotate) {
							mode = TransformToolMode::Rotate;
						}
						else {
							mode = TransformToolMode::Off;
						}
						handled = true;
						break;
					case tfScale: 
						if (mode != +TransformToolMode::Scale) {
							mode = TransformToolMode::Scale;
						}
						else {
							mode = TransformToolMode::Off;
						}
						handled = true;
						break;
					default:
						mode = TransformToolMode::Off;
						break;
				}

				if (handled) {
					log("{0}: {1}\n", name, mode._to_string());

					
					if (mode == +TransformToolMode::Off) {
						deactivate();
					}
					else if (!active) {
						activate();
						deactivate();
					}
					
				}


				return handled;
			}
			return false;
		}));

	// Parenting shortcut
	const int tfParent = GLFW_KEY_P;
	ea.push_back(Tool::InputActions(InputEvent::KeyEvent(GLFW_KEY_P, GLFW_PRESS),
		[&](Tool* tool, const InputEvent& ie)
		{
			auto& app = Application::get();
			auto& shapesOrder = app.selectionOrder;

			// Need at least 2 shapes selected to use parenting shortcut
			if (shapesOrder.size() < 2) return false;

			log("{0}\n", ie.mods);


			// No modifier: chain the selected shapes together 
			if (ie.mods == 0) {
				for (size_t i = 1; i < shapesOrder.size(); i++) {
					auto parent = shapesOrder[i - 1];
					auto child = shapesOrder[i];

					child->setParent(parent);
				}
			}
			// Shift + P: parent all shapes to the first selected shape
			else if (ie.mods == 1) {
				auto parent = shapesOrder[0];

				for (size_t i = 1; i < shapesOrder.size(); i++) {
					auto child = shapesOrder[i];
					child->setParent(parent);
				}
			}
			// 0: no mod
			// 1: shift
			// 2: ctrl
			// 4: alt

			return false;
		}));

	// Mouse controls
	ea.push_back(Tool::InputActions(InputEvent::ClickEvent(GLFW_MOUSE_BUTTON_LEFT),
		[this](Tool* tool, const InputEvent& ie)
		{
			if (mode != +TransformToolMode::Off) {
				if (!active && ie.action == GLFW_PRESS) {
					mousePos = ie.pos;
					lastMousePos = ie.pos;
					selectMousePos = ie.pos;
					activate();
					
				}
				else if (active && ie.action != GLFW_PRESS) {
					deactivate();
				}

				return true;
			}

			return false;
		}));

	ea.push_back(Tool::InputActions(InputEvent::MoveEvent(),
		[this](Tool* tool, const InputEvent& ie)
		{
			if (active) {
				lastMousePos = lastMousePos;
				mousePos = Input::get().current.mousePos;

				return true;
			}
			return true;
		}));

	ea.push_back(Tool::InputActions(InputEvent::ScrollEvent(),
		[this](Tool* tool, const InputEvent& ie)
		{
			if (active) {
				lastScroll = scroll;
				scroll = ie.scroll;

				return true;
			}
			return true;
		}));

	return ea;
}

void TransformTool::activate() {
	Tool::activate();

	selectedOffsets.clear();

	auto& app = Application::get();
	auto& cam = app.getRenderer<OpenGLRenderer>()->camera;
	auto selectedShapes = app.getSelectedObjects();

	for (auto s : selectedShapes) {
		vec3 projectedShapePos = cam.project(s->localPosition);
		vec3 worldMousePos = cam.getWorldPoint(vec3(selectMousePos.x, selectMousePos.y, projectedShapePos.z));
		
		selectedOffsets[s] = worldMousePos - s->localPosition;
	}
}

template<size_t S = 1000>
struct HistoryBuffer {
	size_t added = 0;
	size_t current = 0;
	std::array<double, S> frame;
	std::array<double, S> data;
	
	void add(double v) {
		data[current] = v;
		frame[current] = current;
		current++;
		if (current >= data.size()) {
			current = 0;
		}
		added++;
	}

	size_t used() {
		if (added >= data.size()) return data.size();
		else return current;
	}

	static constexpr size_t size() {
		return S;
	}
};

using ChannelHistory = std::map<AnimationKeyChannel, HistoryBuffer<>>;
using ShapeHistory = std::map<AnimationObject*, ChannelHistory>;

ShapeHistory shapeHistory;

void TransformTool::update(std::vector<InputEvent>& events) {
	Tool::update(events);

	if (!active) return;

	if (mode == +TransformToolMode::Off) return;

	auto& input = Input::get();

	auto& app = Application::get();
	auto selectedShapes = app.getSelectedObjects();
	if (selectedShapes.empty()) return;

	auto& cam = app.getRenderer<OpenGLRenderer>()->camera;

	vec2 dm = mousePos - lastMousePos;
	vec2 d = dm / vec2(app.windowSize);
	vec2 dmag = glm::abs(d);
	float dlen = glm::length(d);
	float dsign = 1.f;
	if (dmag.x > dmag.y && d.x < 0) dsign = -1.f;
	if (dmag.y > dmag.x && d.y < 0) dsign = -1.f;

	switch (mode) {
		case TransformToolMode::Translate:
			if (mousePos != lastMousePos && dlen > 0.f) {
				for (auto s : selectedShapes) {
					vec3 nextPos = cam.project(s->localPosition);
					vec3 worldSpaceMousePos = cam.getWorldPoint(vec3(mousePos.x, mousePos.y, nextPos.z));
					s->localPosition = worldSpaceMousePos - selectedOffsets[s];
					//s->updateMatrix();

					auto& sh = shapeHistory[s];
					auto& chx = sh[AnimationKeyChannel::TranslateX];
					auto& chy = sh[AnimationKeyChannel::TranslateY];
					auto& chz = sh[AnimationKeyChannel::TranslateZ];

					chx.add(s->localPosition.x);
					chy.add(s->localPosition.y);
					chz.add(s->localPosition.z);
				}
			}
			break;
		case TransformToolMode::Rotate:
			if (mousePos != lastMousePos) {
				auto axis = glm::normalize(cam.Lookat - cam.Position);
				auto rot = glm::angleAxis(dlen * dsign * transformWeights.y, axis);
				for (auto s : selectedShapes) {
					auto nextRot = rot * quaternion(glm::radians(s->localRotation));
					s->localRotation = glm::degrees(glm::eulerAngles(nextRot));
					//s->updateMatrix();

					auto& sh = shapeHistory[s];
					auto& chx = sh[AnimationKeyChannel::RotateX];
					auto& chy = sh[AnimationKeyChannel::RotateY];
					auto& chz = sh[AnimationKeyChannel::RotateZ];

					chx.add(s->localRotation.x);
					chy.add(s->localRotation.y);
					chz.add(s->localRotation.z);
				}
			}
			break;
		case TransformToolMode::Scale:
			if (mousePos != lastMousePos) {
				auto scaleChange = (dlen * dsign * transformWeights.z);
				for (auto s : selectedShapes) {
					s->localScale = s->localScale + vec3(scaleChange);
					//s->updateMatrix();

					auto& sh = shapeHistory[s];
					auto& chx = sh[AnimationKeyChannel::ScaleX];
					auto& chy = sh[AnimationKeyChannel::ScaleY];
					auto& chz = sh[AnimationKeyChannel::ScaleZ];

					chx.add(s->localScale.x);
					chy.add(s->localScale.y);
					chz.add(s->localScale.z);
				}
			}
			break;
	}

	lastMousePos = mousePos;
}

void TransformTool::renderUI() {
	ImGui::Checkbox("Active", &active);

	renderEnumDropDown<TransformToolMode>("Mode", mode);

	ImGui::InputFloat2("Mouse pos", glm::value_ptr(mousePos));
	//ImGui::InputFloat("Translate scale", &transformWeights.x);
	ImGui::InputFloat("Rotate scale", &transformWeights.y);
	ImGui::InputFloat("Scale scale", &transformWeights.z);

	static bool showMotionPlot = false;

	ImGui::Checkbox("Show motion plot", &showMotionPlot);

	static AnimationKeyChannel channelToShow = AnimationKeyChannel::TranslateX;

	if (showMotionPlot) {
		renderEnumDropDown<AnimationKeyChannel>("Channel", channelToShow);

		auto ss = Application::get().getSelectedObjects();
		if (!ss.empty()) {
			auto& sh = shapeHistory[ss.front()];

			if (channelToShow == +AnimationKeyChannel::Translate) {
				auto& chx = sh[AnimationKeyChannel::TranslateX];
				auto& chy = sh[AnimationKeyChannel::TranslateY];
				auto& chz = sh[AnimationKeyChannel::TranslateZ];

				if (ImPlot::BeginPlot("Motion plot")) {
					ImPlot::SetupAxes("Frame", "Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

					ImPlot::PlotLine((+AnimationKeyChannel::TranslateX)._to_string(), chx.frame.data(), chx.data.data(), chx.used());
					ImPlot::PlotLine((+AnimationKeyChannel::TranslateY)._to_string(), chy.frame.data(), chy.data.data(), chy.used());
					ImPlot::PlotLine((+AnimationKeyChannel::TranslateZ)._to_string(), chz.frame.data(), chz.data.data(), chz.used());

					ImPlot::EndPlot();
				}
			}
			else if (channelToShow == +AnimationKeyChannel::Rotate) {
				auto& chx = sh[AnimationKeyChannel::RotateX];
				auto& chy = sh[AnimationKeyChannel::RotateY];
				auto& chz = sh[AnimationKeyChannel::RotateZ];

				if (ImPlot::BeginPlot("Motion plot")) {
					ImPlot::SetupAxes("Frame", "Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

					ImPlot::PlotLine((+AnimationKeyChannel::RotateX)._to_string(), chx.frame.data(), chx.data.data(), chx.used());
					ImPlot::PlotLine((+AnimationKeyChannel::RotateY)._to_string(), chy.frame.data(), chy.data.data(), chy.used());
					ImPlot::PlotLine((+AnimationKeyChannel::RotateZ)._to_string(), chz.frame.data(), chz.data.data(), chz.used());

					ImPlot::EndPlot();
				}
			}
			else if (channelToShow == +AnimationKeyChannel::Scale) {
				auto& chx = sh[AnimationKeyChannel::ScaleX];
				auto& chy = sh[AnimationKeyChannel::ScaleY];
				auto& chz = sh[AnimationKeyChannel::ScaleZ];

				if (ImPlot::BeginPlot("Motion plot")) {
					ImPlot::SetupAxes("Frame", "Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

					ImPlot::PlotLine((+AnimationKeyChannel::ScaleX)._to_string(), chx.frame.data(), chx.data.data(), chx.used());
					ImPlot::PlotLine((+AnimationKeyChannel::ScaleY)._to_string(), chy.frame.data(), chy.data.data(), chy.used());
					ImPlot::PlotLine((+AnimationKeyChannel::ScaleZ)._to_string(), chz.frame.data(), chz.data.data(), chz.used());

					ImPlot::EndPlot();
				}
			}
			else {
				auto& ch = sh[channelToShow];

				if (ch.added > 0) {
					//static std::vector<ImPlotPoint> plotPoints = std::vector<ImPlotPoint>(ch.max, ImPlotPoint());
					static double plotY[HistoryBuffer<>::size()];
					static double plotX[HistoryBuffer<>::size()];
					size_t plotPointCount = 0;

					for (size_t i = 0; i < ch.added && i < ch.data.size(); i++) {
						plotX[i] = (double)i;
						plotY[i] = ch.data[i];
						plotPointCount = i + 1;
					}

					if (ImPlot::BeginPlot("Motion plot")) {
						ImPlot::SetupAxes("Frame", "Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

						ImPlot::PlotLine(channelToShow._to_string(), ch.frame.data(), ch.data.data(), ch.used());

						ImPlot::EndPlot();
					}
				}
			}
		}

	}
}