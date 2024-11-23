//#include "Renderer.h"
//
//#include "Application.h"
//#include "Assignment.h"
//#include "Framebuffer.h"
//#include "Input.h"
//#include "InputOutput.h"
//#include "Texture.h"
//#include "Prompts.h"
//#include "Properties.h"
//#include "Tool.h"
//
//#include "imgui.h"
//#include "imgui_impl_glfw.h"
//#include "imgui_impl_opengl3.h"
//
//#include "ImGuizmo.h"
//
//#include <GL/glew.h>
//#include <GLFW/glfw3.h>
//
//#include <iostream>
//
//bool SoftwareRenderer::readyToRock = false;
//SoftwareRenderer* SoftwareRenderer::instance = nullptr;
//
//struct SoftwareRenderer::Settings : public IProperties
//{
//	BoolProp runEveryFrame = BoolProp("runEveryFrame", true);
//	IntProp swapInterval = IntProp("swapInterval", 1);
//	Property<GLenum> blitFilter = Property<GLenum>("blitFilter", GL_NEAREST)
//		.SetRange({ { "GL_NEAREST", GL_NEAREST}, {"GL_LINEAR", GL_LINEAR} });
//
//	Settings() : IProperties("Settings")
//	{
//		runEveryFrame.AddTo(this);
//		swapInterval.AddTo(this).AddChangeEvent([](IProperties* p, const int& oldval, const int& newval) {
//			glfwSwapInterval(newval);
//		});
//		blitFilter.AddTo(this);
//	}
//};
//
//void SoftwareRenderer::initImGui() {
//	auto window = Application::get().window;
//
//	// Setup Platform/Renderer backends
//	ImGui_ImplGlfw_InitForOpenGL(window, true);
//	const char* glsl_version = shaderVersion.c_str();
//	ImGui_ImplOpenGL3_Init(glsl_version);
//}
//
//void SoftwareRenderer::init()
//{
//	instance = this;
//
//	lockResolutionToWindow = false;
//	readyToRock = true;
//	name = "Software Renderer";
//	Application& application = Application::get();
//	glfwGetFramebufferSize(application.window, &resolution.x, &resolution.y);
//	glGetIntegerv(GL_VIEWPORT, glm::value_ptr(viewport));
//
//	gbuffer = std::make_shared<Framebuffer>(256, 128, 2, std::vector<GBufferMode>{ GBufferMode::Rendered, GBufferMode::Rendered });
//
//	settings = std::make_shared<SoftwareRenderer::Settings>();
//
//	glfwSwapInterval(settings->swapInterval);
//
//	glfwSetFramebufferSizeCallback(application.window, [](GLFWwindow* glfw, int newWidth, int newHeight) {
//		if (newWidth <= 0 || newHeight <= 0) return;
//
//		if (auto sr = SoftwareRenderer::instance) {
//			sr->resolution = ivec2(newWidth, newHeight);
//			sr->viewport = ivec4(0, 0, newWidth, newHeight);
//			if (sr->lockResolutionToWindow) {
//				//ogl->gbuffer = std::make_shared<Framebuffer> (newWidth, newHeight);
//				sr->gbuffer = std::make_shared<Framebuffer>(newWidth, newHeight, 2, std::vector<GBufferMode>{ GBufferMode::Rendered, GBufferMode::Rendered });
//			}
//			sr->refresh = true;
//		}
//		});
//
//	Input::get().init();
//
//	initImGui();
//
//	initialized = true;
//}
//
//void SoftwareRenderer::ImGuiNewFrame() {
//	ImGui_ImplOpenGL3_NewFrame();
//	ImGui_ImplGlfw_NewFrame();
//	ImGui::NewFrame();
//	ImGuizmo::BeginFrame();
//
//}
//
//double SoftwareRenderer::Render(Application& application)
//{
//	_time nowish = _clock::now();
//
//	if (!initialized)
//	{
//		init();
//	}
//
//	if (settings->runEveryFrame || refresh) {
//		gbuffer->bind(GL_DRAW_FRAMEBUFFER);
//		gbuffer->setAllBuffers();
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//		//drawOnCPU();
//
//		// Only render assignments that don't use OpenGL
//		for (auto& assignment : application.assignments) {
//			if (!assignment->useOpenGL) {
//				assignment->render(gbuffer->textures[0]);
//			}
//		}
//
//		if (!settings->runEveryFrame) {
//			refresh = false;
//		}
//
//		gbuffer->unbind(GL_DRAW_FRAMEBUFFER);
//		gbuffer->bind(GL_READ_FRAMEBUFFER);
//		glReadBuffer(GL_COLOR_ATTACHMENT0);
//
//		float gw = gbuffer->width;
//		float gh = gbuffer->height;
//		float ga = gw / gh;
//
//		float ww = resolution.x;
//		float wh = resolution.y;
//		float wa = ww / wh;
//
//		GLint srcX0 = 0;
//		GLint srcY0 = 0;
//		GLint srcX1 = gbuffer->width;
//		GLint srcY1 = gbuffer->height;
//
//		GLint dstX0 = 0;
//		GLint dstY0 = 0;
//		GLint dstX1 = resolution.x;
//		GLint dstY1 = resolution.y;
//
//
//
//		/*
//		// Gbuffer aspect ratio is larger than window - that means we need letterbox on top and bottom
//		// dstX0 and dstX1 will not change, but dstY0 and dstY1 need to.
//		if (ga > wa) {
//			// First, figure out the pixel count difference that 
//			int numY = glm::round(dstX1 / ga);
//			int diff = dstY1 - numY;
//			dstY0 = diff / 2;
//			dstY1 = dstY0 + numY;
//		}
//		// Gbuffer aspect ratio is smaller than window - that means we need letterbox on left and right
//		// dstY0 and dstY1 will not change, but dstX0 and dstX1 need to.
//		else if (wa > ga) {
//			// Gbuffer aspect ratio is still favoring width, so make sure all of y is covered
//			if (ga >= 1.0f) {
//				int numX = glm::round(dstY1 * ga);
//				int diff = dstX1 - numX;
//				dstX0 = diff / 2;
//				dstX1 = dstX0 + numX;
//			}
//			else {
//				int numX = glm::round(dstX1 / wa);
//				int diff = dstX1 - numX;
//				dstX0 = diff / 2;
//				dstX1 = dstX0 + numX;
//			}
//		}
//		*/
//
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//		glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1,
//			dstX0, dstY0, dstX1, dstY1,
//			GL_COLOR_BUFFER_BIT, settings->blitFilter.value);
//
//		/*glBlitFramebuffer(0, 0, gbuffer->width, gbuffer->height,
//			0, 0, resolution.x, resolution.y, GL_COLOR_BUFFER_BIT, settings->blitFilter.value);*/
//
//		gbuffer->unbind(GL_READ_FRAMEBUFFER);
//
//		glFinish();
//
//
//		glBindFramebuffer(GL_FRAMEBUFFER, 0);
//	}
//
//
//
//	_elapsed renderDiff = _clock::now() - nowish;
//	return renderDiff.count();
//}
//
//void SoftwareRenderer::endRender() {
//	presentUI();
//
//	glfwSwapBuffers(Application::get().window);
//}
//
//void SoftwareRenderer::renderUI()
//{
//	if (ImGui::CollapsingHeader(name.c_str()))
//	{
//		ImGui::Indent(16.0f);
//
//		if (ImGui::Button("Force refresh")) {
//			refresh = true;
//		}
//
//		ImGui::Checkbox("Lock resolution to window size", &lockResolutionToWindow);
//
//		if (gbuffer) gbuffer->renderUI("Framebuffer");
//
//		ImGui::ColorEdit4("Clear color", glm::value_ptr(clearColor));
//
//		//camera.renderUI();
//
//		if (ImGui::CollapsingHeader("Settings"))
//		{
//			settings->renderUI();
//		}
//
//		ImGui::Unindent(16.0f);
//	}
//}
//
//void SoftwareRenderer::presentUI() {
//	ImGui::Render();
//
//	if (!showImGui) return;
//
//	int display_w, display_h;
//	glfwGetFramebufferSize(Application::get().window, &display_w, &display_h);
//
//	glViewport(0, 0, display_w, display_h);
//	ImVec4 clear_color = { 0, 0, 0, 0 };
//	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//}
//
//SoftwareRenderer::~SoftwareRenderer() {
//	readyToRock = false;
//}