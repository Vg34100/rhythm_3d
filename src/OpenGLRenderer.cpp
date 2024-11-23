#include "Renderer.h"

#include "Application.h"
#include "AnimationObjectRenderer.h"
#include "Assignment.h"
#include "GLTFImporter.h"
#include "Framebuffer.h"
#include "Input.h"
#include "InputOutput.h"
#include "Lighting.h"
#include "Texture.h"
#include "Prompts.h"
#include "Properties.h"
#include "Textures.h"
#include "Tool.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "ImGuizmo.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

bool OpenGLRenderer::readyToRock = false;
OpenGLRenderer* OpenGLRenderer::instance = nullptr;

std::string Renderer::shaderVersion = "#version 130";

struct OpenGLRenderer::Settings : public IProperties
{
	BoolProp runEveryFrame = BoolProp("runEveryFrame", true);
	IntProp swapInterval = IntProp("swapInterval", 1);
	BoolProp glEnableCulling = BoolProp("glEnableCulling", true);
	Property<GLenum> glFrontFace = Property<GLenum>("glFrontFace", GL_CCW)
		.SetRange({ { "GL_CCW", GL_CCW }, { "GL_CW", GL_CW} });
	Property<GLenum> glCullFace = Property<GLenum>("glCullFace", GL_BACK)
		.SetRange({ { "GL_BACK", GL_BACK }, { "GL_FRONT", GL_FRONT }, { "GL_FRONT_AND_BACK", GL_FRONT_AND_BACK } })
		;

	Property<GLenum> blitFilter = Property<GLenum>("blitFilter", GL_NEAREST)
		.SetRange({ { "GL_NEAREST", GL_NEAREST}, {"GL_LINEAR", GL_LINEAR} });

	std::vector<Property<GLenum>> blendRange = {
		Property<GLenum>("GL_ZERO", GL_ZERO),
		Property<GLenum>("GL_ONE", GL_ONE),
		Property<GLenum>("GL_SRC_COLOR", GL_SRC_COLOR),
		Property<GLenum>("GL_ONE_MINUS_SRC_COLOR", GL_ONE_MINUS_SRC_COLOR),
		Property<GLenum>("GL_DST_COLOR", GL_DST_COLOR),
		Property<GLenum>("GL_ONE_MINUS_DST_COLOR", GL_ONE_MINUS_DST_COLOR),
		Property<GLenum>("GL_SRC_ALPHA", GL_SRC_ALPHA),
		Property<GLenum>("GL_ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA),
		Property<GLenum>("GL_DST_ALPHA", GL_DST_ALPHA),
		Property<GLenum>("GL_ONE_MINUS_DST_ALPHA", GL_ONE_MINUS_DST_ALPHA),
		Property<GLenum>("GL_CONSTANT_COLOR", GL_CONSTANT_COLOR),
		Property<GLenum>("GL_ONE_MINUS_CONSTANT_COLOR", GL_ONE_MINUS_CONSTANT_COLOR),
		Property<GLenum>("GL_CONSTANT_ALPHA", GL_CONSTANT_ALPHA),
		Property<GLenum>("GL_ONE_MINUS_CONSTANT_ALPHA", GL_ONE_MINUS_CONSTANT_ALPHA),
		Property<GLenum>("GL_SRC_ALPHA_SATURATE", GL_SRC_ALPHA_SATURATE),
		Property<GLenum>("GL_SRC1_COLOR", GL_SRC1_COLOR)
	};

	std::vector<Property<GLenum>> blendEquations = {
		Property<GLenum>("GL_FUNC_ADD", GL_FUNC_ADD),
		Property<GLenum>("GL_FUNC_SUBTRACT", GL_FUNC_SUBTRACT),
		Property<GLenum>("GL_FUNC_REVERSE_SUBTRACT", GL_FUNC_REVERSE_SUBTRACT),
		Property<GLenum>("GL_MIN", GL_MIN),
		Property<GLenum>("GL_MAX", GL_MAX),
	};

	Property<bool> blend = Property<bool>("blend", false);
	Property<GLenum> blendFuncSrc = Property<GLenum>("blendFuncSrc", GL_SRC_ALPHA).SetRange(blendRange);
	Property<GLenum> blendFuncDst = Property<GLenum>("blendFuncDst", GL_ONE_MINUS_SRC_ALPHA).SetRange(blendRange);
	Property<GLenum> blendEquation = Property<GLenum>("blendEquation", GL_FUNC_ADD).SetRange(blendEquations);

	std::vector<Property<GLenum>> depthFuncs = {
		Property<GLenum>("GL_NEVER", GL_NEVER),
		Property<GLenum>("GL_ALWAYS", GL_ALWAYS),
		Property<GLenum>("GL_LESS", GL_LESS),
		Property<GLenum>("GL_LEQUAL", GL_LEQUAL),
		Property<GLenum>("GL_GREATER", GL_GREATER),
		Property<GLenum>("GL_GEQUAL", GL_GEQUAL),
		Property<GLenum>("GL_EQUAL", GL_EQUAL),
		Property<GLenum>("GL_NOTEQUAL", GL_NOTEQUAL),
	};

	Property<bool> depthTest = Property<bool>("depthTest", true);
	Property<GLenum> depthFunc = Property<GLenum>("depthFunc", GL_GREATER).SetRange(depthFuncs);


	Settings() : IProperties("Settings")
	{
		runEveryFrame.AddTo(this);
		swapInterval.AddTo(this).AddChangeEvent([](IProperties* p, const int& oldval, const int& newval) {
			glfwSwapInterval(newval);
			});
		glEnableCulling.AddTo(this);
		glFrontFace.AddTo(this);
		glCullFace.AddTo(this);
		blitFilter.AddTo(this);
	}
};

void OpenGLRenderer::initImGui() {
	auto window = Application::get().window;

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	const char* glsl_version = shaderVersion.c_str();
	ImGui_ImplOpenGL3_Init(glsl_version);
}

void OpenGLRenderer::init()
{
	instance = this;
	lockResolutionToWindow = true;
	readyToRock = true;
	name = "OpenGL Renderer";
	Application& application = Application::get();
	glfwGetFramebufferSize(application.window, &resolution.x, &resolution.y);
	glGetIntegerv(GL_VIEWPORT, glm::value_ptr(viewport));

	gbuffer = std::make_shared<Framebuffer>(resolution.x, resolution.y, 2, std::vector<GBufferMode>{ GBufferMode::Rendered, GBufferMode::Rendered });
	input = std::make_shared<Framebuffer>(resolution.x, resolution.y, 1, std::vector<GBufferMode>{}, false, false);
	output = std::make_shared<Framebuffer>(resolution.x, resolution.y, 1, std::vector<GBufferMode>{}, false, false);
	shadowMap = std::make_shared<Framebuffer>(4096, 4096, 0);
	dummyInput = std::make_shared<Framebuffer>(1, 1);

	settings = std::make_shared<OpenGLRenderer::Settings>();

	glfwSwapInterval(settings->swapInterval);

	glfwSetFramebufferSizeCallback(application.window, [](GLFWwindow* glfw, int newWidth, int newHeight) {
		if (newWidth <= 0 || newHeight <= 0) return;

		if (auto sr = OpenGLRenderer::instance) {
			sr->resolution = ivec2(newWidth, newHeight);
			sr->viewport = ivec4(0, 0, newWidth, newHeight);
			if (sr->lockResolutionToWindow) {
				//ogl->gbuffer = std::make_shared<Framebuffer> (newWidth, newHeight);
				sr->gbuffer = std::make_shared<Framebuffer>(newWidth, newHeight, 2, std::vector<GBufferMode>{ GBufferMode::Rendered, GBufferMode::Rendered });
				sr->input = std::make_shared<Framebuffer>(newWidth, newHeight);
				sr->output = std::make_shared<Framebuffer>(newWidth, newHeight);
			}
			sr->refresh = true;
		}
		});

	Input::get().init();

	glDepthRange(-1, 1);

	vec3 levelSize = vec3(32.f);

	//float defaultMaxDist = sqrt(levelSize.x * levelSize.y * levelSize.z * 0.25);

	//camera.init(vec3(0, 1, 0));
	camera = Camera(60.0f, resolution.x, resolution.y, 0.01f, 1000.f, 0.01f, 250,
		vec3(0.f, 0.f, levelSize.z * 0.5f),
		vec3(0.f),
		vec3(0, 1, 0));

	initImGui();

	initialized = true;
}

void OpenGLRenderer::ImGuiNewFrame() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

}

double OpenGLRenderer::Render(Application& application)
{
	_time nowish = _clock::now();


	camera.update();


	auto& lighting = GPU::Lighting::get();

	// Update any light positions that are bound to shapes
	for (auto& s : application.objects) {
		if (s.lightIndex >= 0 && s.lightIndex < GPU_LIGHT_MAX_COUNT) {
			auto& l = lighting.lights[s.lightIndex];

			l.position = vec4(s.position, l.position.w);
			l.direction = vec4(glm::toMat3(quaternion(glm::radians(s.rotation))) * vec3(1.f, 0.f, 0.f), 0.f);
			l.diffuse = s.color;

			if (s.lightIndex == 0) {
				lighting.view = s.transform;
			}
		}
	}

	// Tie shadow light with first GPU light


	//if (!lighting.lights.empty()) 
	{
		auto& l0 = lighting.lights[0];

		lighting.position = l0.position;
		lighting.center = l0.position + l0.direction;

		/*
		l0.position = vec4(lighting.position, l0.position.w);
		l0.direction = vec4(glm::normalize(lighting.center - lighting.position), 0.f);
		*/
	}


	lighting.update();

	if (!initialized)
	{
		init();
	}

	bool renderThisFrame = settings->runEveryFrame || refresh;

	if (!renderThisFrame) return (_clock::now() - nowish).count();

	// Render shadow map
	if (shadowMap) {

		/*
		static s_ptr<FullscreenFilter>expWipeFilter = std::make_shared<FullscreenFilter>("filters/expdepthwipe.frag");
		static bool expInitialized = false;

		if (!expInitialized) {
			expInitialized = true;
			expWipeFilter->input = nullptr;
			expWipeFilter->output = nullptr;
			expWipeFilter->useOwnInputBuffer = false;
			expWipeFilter->useOwnOutputBuffer = false;
			expWipeFilter->setGBuffer(shadowMap);
		}

		shadowMap->bind(GL_DRAW_FRAMEBUFFER);
		shadowMap->setAllBuffers();
		glDepthFunc(GL_ALWAYS);
		expWipeFilter->render();
		*/

		shadowMap->bind(GL_DRAW_FRAMEBUFFER);
		glDepthFunc(GL_LESS);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		auto shadowCam = camera;

		auto& l = GPU::Lighting::get();

		if (l.projectionType == +ProjectionType::Orthographic) {
			shadowCam.projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, l.nearFar.x, l.nearFar.y);
		}
		else {
			shadowCam.projection = glm::perspective(glm::radians(l.fov), (float)shadowMap->width / (float)shadowMap->height, l.nearFar.x, l.nearFar.y);
		}


		shadowCam.view = glm::lookAt(l.position, l.center, l.up);
		shadowCam.viewproj = shadowCam.projection * shadowCam.view;

		if (useShadow) {
			renderScene(shadowCam.projection, shadowCam.view, shadowMap);
		}

	}

	gbuffer->bind(GL_DRAW_FRAMEBUFFER);
	gbuffer->setAllBuffers();
	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (settings->depthTest) {
		glEnable(GL_DEPTH_TEST);
	}
	else {
		glDisable(GL_DEPTH_TEST);
	}

	if (settings->glEnableCulling) {
		glEnable(GL_CULL_FACE);
		glCullFace(settings->glCullFace);
	}
	else {
		glDisable(GL_CULL_FACE);
	}

	glFrontFace(settings->glFrontFace);

	if (settings->blend) {
		glEnable(GL_BLEND);
		glBlendFunc(settings->blendFuncSrc.value, settings->blendFuncDst.value);
		glBlendEquation(settings->blendEquation.value);
	}
	else {
		glDisable(GL_BLEND);
	}

	renderScene(camera.projection, camera.view, gbuffer);

	//// Only render assignments that use OpenGL
	//for (auto& assignment : application.assignments) {
	//	if (assignment->useOpenGL) {
	//		assignment->render(camera.projection, camera.view, gbuffer, false);
	//	}
	//}

	if (!settings->runEveryFrame) {
		refresh = false;
	}

	gbuffer->unbind(GL_DRAW_FRAMEBUFFER);
	gbuffer->bind(GL_READ_FRAMEBUFFER);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	glBlitFramebuffer(0, 0, gbuffer->width, gbuffer->height,
		0, 0, resolution.x, resolution.y, GL_COLOR_BUFFER_BIT, settings->blitFilter.value);

	gbuffer->unbind(GL_READ_FRAMEBUFFER);

	glFinish();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);



	_elapsed renderDiff = _clock::now() - nowish;
	return renderDiff.count();
}

void OpenGLRenderer::endRender() {
	presentUI();

	glfwSwapBuffers(Application::get().window);
}

double OpenGLRenderer::renderScene(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer) {
	double nowish = getTime();

	auto currCam = camera;
	camera.projection = projection;
	camera.view = view;
	camera.viewproj = projection * view;


	auto& application = Application::get();
	auto& jr = AnimationObjectRenderer::get();

	bool isShadow = framebuffer == shadowMap;

	bool cullShadowFace = false;

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




		for (auto& shape : application.objects) {
			if (isShadow && shape.lightIndex != -1) continue;
			if (st == +AnimationObjectType::model) continue;
			if (shape.shapeType == st) {
				int tex = long(shape.texture);
				//jr.renderBatchWithOwnColor(shape.transform, shape.color, (GLuint)tex, isShadow);
				jr.renderBatchWithOwnColor(shape, isShadow);
			}
		}

		jr.endBatchRender(isShadow);
	}

	// Render all mesh shapes
	for (auto& shape : application.objects) {
		if (isShadow && shape.lightIndex != -1) continue;
		if (shape.shapeType != +AnimationObjectType::model) continue;
		jr.beginBatchRender(shape, false, shape.color, isShadow);
		int tex = long(shape.texture);
		jr.renderBatchWithOwnColor(shape, isShadow);
		jr.endBatchRender(isShadow);
	}

	// Render all assignments
	for (auto& assignment: application.assignments) {
		assignment->render(projection, view, framebuffer, isShadow);
	}

	// Render all GLTF stuff
	if (application.gltf) {
		GLTFRenderer::get().render(projection, view, application.gltf, framebuffer, isShadow);
	}


	if (!isShadow) {
		/*
		auto & l = GPU::Lighting::get();
		//mat4 view = glm::lookAt(l.position, l.center, l.up);
		vec3 aimDir = glm::normalize(l.center - l.position);
		mat4 lightTf = glm::translate(l.position) * glm::toMat4(glm::rotation(vec3(1.f, 0.f, 0.f), aimDir));
		//shadowCam.viewproj = shadowCam.projection * shadowCam.view;
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		jr.renderLight(lightTf);
		*/

		for (auto& tool : application.tools) {
			tool->render();
		}

	}

	camera = currCam;

	return getTime() - nowish;
}

void OpenGLRenderer::renderUI()
{
	if (ImGui::CollapsingHeader(name.c_str()))
	{
		ImGui::Indent(16.0f);
		if (ImGui::Button("Reload shaders")) {
			Application::get().addCommand([&]() {
				for (auto& sm : AnimationObjectRenderer::get().shapeCatalog) {
					sm.second.initShader();
				}
			});
		}

		if (ImGui::Button("Force refresh")) {
			refresh = true;
		}

		ImGui::Checkbox("Lock resolution to window size", &lockResolutionToWindow);

		if (gbuffer) gbuffer->renderUI("Framebuffer");

		ImGui::ColorEdit4("Clear color", glm::value_ptr(clearColor));

		GPU::Lighting::get().renderUI(this);

		camera.renderUI();

		if (ImGui::CollapsingHeader("Textures")) {
			if (ImGui::Button("Load new texture")) {
				auto texFile = Util::LoadFile({ "Common image files (.jpg .png .bmp .gif)", "*.jpg *.png *.bmp *.gif", "All files (*)", "*" }, ".");
				if (texFile != "") {
					if (s_ptr<Texture> t = std::make_shared<Texture>(texFile)) {
						TextureRegistry::addTexture(t);
						//textures[t->id] = t;
					}
				}
			}

			for (auto& t : textures) {
				t.second->renderUI();
			}
		}

		if (ImGui::CollapsingHeader("Settings"))
		{
			settings->renderUI();
		}

		ImGui::Unindent(16.0f);
	}
}

void OpenGLRenderer::presentUI() {
	ImGui::Render();

	if (!showImGui) return;

	int display_w, display_h;
	glfwGetFramebufferSize(Application::get().window, &display_w, &display_h);

	glViewport(0, 0, display_w, display_h);
	ImVec4 clear_color = { 0, 0, 0, 0 };
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

OpenGLRenderer::~OpenGLRenderer() {
	readyToRock = false;
}