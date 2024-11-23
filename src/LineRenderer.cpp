#include "LineRenderer.h"

#include "Application.h"
#include "Camera.h"
#include "Renderer.h"
#include "Lighting.h"
#include "Texture.h"
#include "Framebuffer.h"
#include "StringUtil.h"
#include "InputOutput.h"

#include "imgui.h"

LineRenderData::LineRenderData() {

	glGenVertexArrays(1, &vao);

	vertexVBO = spVectorBuffer<GLuint>(
		new VectorBuffer<GLuint>(GL_ARRAY_BUFFER, { 0, 1, 2, 3 }, GL_STATIC_DRAW));

	indexVBO = spVectorBuffer<GLuint>(
		new VectorBuffer<GLuint>(GL_ELEMENT_ARRAY_BUFFER, { 0, 1, 2, 0, 2, 3 }, GL_STATIC_DRAW));

	initShader();
}

bool LineRenderData::initShader() {
	auto vertText = Shader::LoadText("objects/line.vert");
	auto fragText = Shader::LoadText("objects/line.frag");

	shader = s_ptr<Shader>(new Shader(vertText, fragText));

	if (!shader->init()) {
		log("Unable to load LineRenderData shader!\n");
		return false;
	}

	ShaderBinding binding;
	binding.addVertexAttribute("int", "index", [this](GLint loc) {
		vertexVBO->bind();
		glVertexAttribIPointer(loc, 1, GL_UNSIGNED_INT, sizeof(GLuint), (const GLvoid*)0);
		glEnableVertexAttribArray(loc);
		});

	glBindVertexArray(vao);
	shader->bind(binding);
	indexVBO->bind();
	glBindVertexArray(0);

	return true;
}

LineRenderer& LineRenderer::get()
{
	static LineRenderer instance;
	return instance;
}

bool LineRenderer::init()
{
	renderData = std::make_shared<LineRenderData>();
	return renderData != nullptr;
}

void LineRenderer::begin() {
	if (!initialized) {
		initialized = init();

		if (!initialized) return;
	}

	glBindVertexArray(renderData->vao);
	renderData->indexVBO->bind();
	renderData->shader->start();
	renderData->shader->binding.refreshUniforms();
}


void LineRenderer::render(const vec3& p1, const vec3& p2, const vec4& color, float width) {

	begin();
	renderSingle(p1, p2, color, width);
	end();
}

void LineRenderer::renderSingle(const vec3& p1, const vec3& p2, const vec4& color, float width) {
	vec3 xr = p2 - p1;
	float diffLength = glm::length(xr);
	xr = glm::normalize(xr);

	auto& cam = Application::get().getRenderer<OpenGLRenderer>()->camera;

	vec3 zr = glm::normalize(cam.Position - cam.Lookat);

	vec3 yr = glm::cross(zr, xr);

	mat3 r;
	r[0] = xr;
	r[1] = yr;
	r[2] = zr;

	mat4 rotation = r;



	//auto rotation = glm::rotation(vec3(1.f, 0.f, 0.f), glm::normalize(diff));

	mat4 model =
		glm::translate(p1)
		* rotation
		* glm::scale(vec3(diffLength, globalWidth * width, 1.0f));


	/*
	vec3 rp1 = model * vec4{ 0.0, 1.0, 0.0, 1.0};
	vec3 rp2 = model * vec4 { 0.0, -1.0, 0.0, 1.0 };
	vec3 rp3 = model * vec4 { 1.0, -1.0, 0.0, 1.0 };

	vec3 a = glm::normalize(rp2 - rp1);
	vec3 b = glm::normalize(rp3 - rp2);
	vec3 c = glm::normalize(glm::cross(a, b));

	float angleDiff = glm::angle(c, toCam);

	auto toCamRotation = glm::angleAxis(angleDiff, glm::normalize(diff));

	rotation = toCamRotation * rotation;

	model =
		 glm::translate(p1)
		* glm::toMat4(rotation)
		* glm::scale(vec3(diffLength, globalWidth, 1.0f));

	*/

	mat4 viewproj = cam.viewproj;

	mat4 mvp = viewproj * model;

	auto mvpLoc = glGetUniformLocation(renderData->shader->program, "mvp");
	if (mvpLoc != -1) {
		glUniformMatrix4fv(mvpLoc, 1, false, glm::value_ptr(mvp));
	}

	auto colorLoc = glGetUniformLocation(renderData->shader->program, "color");
	if (colorLoc != -1) {
		glUniform4fv(colorLoc, 1, glm::value_ptr(color));
	}

	glDrawElements(GL_TRIANGLES, renderData->indexVBO->data.size(), GL_UNSIGNED_INT, (const GLvoid*)0);
}

void LineRenderer::end() {
	renderData->shader->stop();
	glBindVertexArray(0);
}

void LineRenderer::renderUI() {
	if (ImGui::CollapsingHeader("LineRenderer")) {
		ImGui::ColorEdit4("Global color", glm::value_ptr(globalColor));
		ImGui::InputFloat("Global width", &globalWidth);
	}
}