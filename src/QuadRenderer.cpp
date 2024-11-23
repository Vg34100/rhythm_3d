#include "QuadRenderer.h"

#include "Application.h"
#include "Camera.h"
#include "Renderer.h"
#include "Lighting.h"
#include "Texture.h"
#include "Framebuffer.h"
#include "StringUtil.h"
#include "InputOutput.h"
#include "Renderer.h"
#include "Assignments/ParticleSystemDemo.h"

#include "imgui.h"

QuadRenderData::QuadRenderData() {

	glGenVertexArrays(1, &vao);

	vertexVBO = spVectorBuffer<GLuint>(
		new VectorBuffer<GLuint>(GL_ARRAY_BUFFER, { 0, 1, 2, 3 }, GL_STATIC_DRAW));

	indexVBO = spVectorBuffer<GLuint>(
		new VectorBuffer<GLuint>(GL_ELEMENT_ARRAY_BUFFER, { 0, 1, 2, 0, 2, 3 }, GL_STATIC_DRAW));

	initShader();
}

bool QuadRenderData::initShader() {
	auto vertText = Shader::LoadText("objects/quad.vert");
	auto fragText = Shader::LoadText("objects/quad.frag");

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

QuadRenderer& QuadRenderer::get()
{
	static QuadRenderer instance;
	return instance;
}

bool QuadRenderer::init()
{
	renderData = std::make_shared<QuadRenderData>();
	return renderData != nullptr;
}

void QuadRenderer::beginBatch(bool showBackface, unsigned int textureID, bool allowBlend) {
	if (!initialized) {
		initialized = init();

		if (!initialized) return;
	}

	glBindVertexArray(renderData->vao);
	renderData->indexVBO->bind();
	renderData->shader->start();
	renderData->shader->binding.refreshUniforms();

	if (showBackface) {
		glDisable(GL_CULL_FACE);
	}

	if (allowBlend) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	GLint utLoc = glGetUniformLocation(renderData->shader->program, "useTexture");
	glActiveTexture(GL_TEXTURE0);
	if (textureID) {
		glUniform1i(utLoc, 1);
		glBindTexture(GL_TEXTURE_2D, textureID);
	}
	else {
		glUniform1i(utLoc, 0);
		auto ogl = std::dynamic_pointer_cast<OpenGLRenderer>(Application::get().renderer);
		glBindTexture(GL_TEXTURE_2D, ogl->dummyInput->textures[0]->id);
	}

	readyToRender = true;
}

void QuadRenderer::render(const vec3& position, const vec3& rotation, const vec3& scale, const vec4& color) {
	if (!readyToRender) beginBatch();



	mat4 quadPos = glm::translate(position);
	mat4 quadRot = glm::toMat4(quaternion(glm::radians(rotation)));
	mat4 quadScale = glm::scale(scale);


	//auto rotation = glm::rotation(vec3(1.f, 0.f, 0.f), glm::normalize(diff));

	mat4 model = quadPos * quadRot * quadScale;


	auto& cam = Application::get().getRenderer<OpenGLRenderer>()->camera;

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

void QuadRenderer::endBatch() {
	glBindTexture(GL_TEXTURE_2D, 0);
	renderData->shader->stop();
	glBindVertexArray(0);

	glDisable(GL_BLEND);

	readyToRender = false;
}

void QuadRenderer::renderUI() {
	if (ImGui::CollapsingHeader("QuadRenderer")) {
		ImGui::ColorEdit4("Global color", glm::value_ptr(globalColor));
		ImGui::InputFloat("Global width", &globalWidth);
	}
}




InstanceQuadRenderData::InstanceQuadRenderData() {

	glGenVertexArrays(1, &vao);

	vertexVBO = spVectorBuffer<GLuint>(
		new VectorBuffer<GLuint>(GL_ARRAY_BUFFER, { 0, 1, 2, 3 }, GL_STATIC_DRAW));

	indexVBO = spVectorBuffer<GLuint>(
		new VectorBuffer<GLuint>(GL_ELEMENT_ARRAY_BUFFER, { 0, 1, 2, 0, 2, 3 }, GL_STATIC_DRAW));

	initShader();
}

bool InstanceQuadRenderData::initShader() {

	auto vertText = Shader::LoadText("objects/quad_instance.vert");
	auto fragText = Shader::LoadText("objects/quad_instance.frag");

	shader = s_ptr<Shader>(new Shader(vertText, fragText));

	if (!shader->init()) {
		log("Unable to load InstanceQuadRenderData shader!\n");
		return false;
	}

	ShaderBinding binding;
	binding.addVertexAttribute("int", "index", [this](GLint loc) {
		vertexVBO->bind();
		glVertexAttribIPointer(loc, 1, GL_UNSIGNED_INT, sizeof(GLuint), (const GLvoid*)0);
		glEnableVertexAttribArray(loc);
		});

	binding.addVertexAttribute("vec4", "m0", [this](GLint loc) {
		InstanceQuadRenderer::get().currentParticles->bind();
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleRenderData), (const GLvoid*)offsetof(ParticleRenderData, transform));
		glEnableVertexAttribArray(loc);
		glVertexAttribDivisor(loc, 1);
		});

	binding.addVertexAttribute("vec4", "m1", [this](GLint loc) {
		InstanceQuadRenderer::get().currentParticles->bind();
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleRenderData), (const GLvoid*)(offsetof(ParticleRenderData, transform) + sizeof(vec4)));
		glEnableVertexAttribArray(loc);
		glVertexAttribDivisor(loc, 1);
		});

	binding.addVertexAttribute("vec4", "m2", [this](GLint loc) {
		InstanceQuadRenderer::get().currentParticles->bind();
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleRenderData), (const GLvoid*)(offsetof(ParticleRenderData, transform) + sizeof(vec4) * 2));
		glEnableVertexAttribArray(loc);
		glVertexAttribDivisor(loc, 1);
		});

	binding.addVertexAttribute("vec4", "m3", [this](GLint loc) {
		InstanceQuadRenderer::get().currentParticles->bind();
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleRenderData), (const GLvoid*)(offsetof(ParticleRenderData, transform) + sizeof(vec4) * 3));
		glEnableVertexAttribArray(loc);
		glVertexAttribDivisor(loc, 1);
		});


	binding.addVertexAttribute("vec4", "color", [this](GLint loc) {
		InstanceQuadRenderer::get().currentParticles->bind();
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleRenderData), (const GLvoid*)offsetof(ParticleRenderData, color));
		glEnableVertexAttribArray(loc);
		glVertexAttribDivisor(loc, 1);
		});

	binding.addVertexAttribute("vec4", "data", [this](GLint loc) {
		InstanceQuadRenderer::get().currentParticles->bind();
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleRenderData), (const GLvoid*)offsetof(ParticleRenderData, data));
		glEnableVertexAttribArray(loc);
		glVertexAttribDivisor(loc, 1);
		});

	glBindVertexArray(vao);
	shader->bind(binding);
	indexVBO->bind();
	glBindVertexArray(0);

	return true;
}

InstanceQuadRenderer& InstanceQuadRenderer::get()
{
	static InstanceQuadRenderer instance;
	static int didItOnce = [&]() {
		std::vector<ParticleRenderData> prd(1000, { mat4(1.f), vec4(1.f), vec4(0.f) });
		instance.currentParticles = spVectorBuffer<ParticleRenderData>(new VectorBuffer<ParticleRenderData>(GL_ARRAY_BUFFER, prd, GL_DYNAMIC_DRAW));
		return 1;
		}();
	return instance;
}

bool InstanceQuadRenderer::init()
{
	renderData = std::make_shared<InstanceQuadRenderData>();
	return renderData != nullptr;
}

void InstanceQuadRenderer::beginBatch(ParticleSystem* ps, bool showBackface, unsigned int textureID, bool allowBlend) {

	currentParticles = ps->renderData;
	currentParticleCount = ps->maxParticles;

	if (!initialized) {
		initialized = init();

		if (!initialized) return;
	}

	glBindVertexArray(renderData->vao);
	renderData->indexVBO->bind();
	renderData->shader->start();
	renderData->shader->binding.refresh();

	if (showBackface) {
		glDisable(GL_CULL_FACE);
	}

	if (allowBlend) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	GLint utLoc = glGetUniformLocation(renderData->shader->program, "useTexture");
	glActiveTexture(GL_TEXTURE0);
	if (textureID) {
		glUniform1i(utLoc, 1);
		glBindTexture(GL_TEXTURE_2D, textureID);
	}
	else {
		glUniform1i(utLoc, 0);
		auto ogl = std::dynamic_pointer_cast<OpenGLRenderer>(Application::get().renderer);
		glBindTexture(GL_TEXTURE_2D, ogl->dummyInput->textures[0]->id);
	}

	readyToRender = true;
}

void InstanceQuadRenderer::render(const vec4& mainColor, const vec4 uvOffset) {
	if (!readyToRender) return;

	auto& cam = Application::get().getRenderer<OpenGLRenderer>()->camera;

	mat4 viewproj = cam.viewproj;

	auto mvpLoc = glGetUniformLocation(renderData->shader->program, "vp");
	if (mvpLoc != -1) {
		glUniformMatrix4fv(mvpLoc, 1, false, glm::value_ptr(viewproj));
	}

	auto mcLoc = glGetUniformLocation(renderData->shader->program, "mainColor");
	if (mcLoc != -1) {
		glUniform4fv(mcLoc, 1, glm::value_ptr(mainColor));
	}

	glUniform4fv(glGetUniformLocation(renderData->shader->program, "uvOffset"), 1, glm::value_ptr(uvOffset));

	//glDrawElements(GL_TRIANGLES, renderData->indexVBO->data.size(), GL_UNSIGNED_INT, (const GLvoid*)0);
	glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)renderData->indexVBO->data.size(), GL_UNSIGNED_INT,
		(const GLvoid*)0, (GLsizei)currentParticleCount);
}

void InstanceQuadRenderer::endBatch() {
	glBindTexture(GL_TEXTURE_2D, 0);
	renderData->shader->stop();
	glBindVertexArray(0);

	glDisable(GL_BLEND);

	readyToRender = false;
}

void InstanceQuadRenderer::renderUI() {
	if (ImGui::CollapsingHeader("InstanceQuadRenderer")) {
		ImGui::ColorEdit4("Global color", glm::value_ptr(globalColor));
		ImGui::InputFloat("Global width", &globalWidth);
	}
}