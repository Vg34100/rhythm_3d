#pragma once

#include "globals.h"
#include "Meshing.h"
#include "Buffer.h"
#include "Shader.h"
#include "Uniform.h"



struct QuadRenderData {

	GLuint vao = 0;

	s_ptr<VectorBuffer<GLuint>> indexVBO;
	s_ptr<VectorBuffer<GLuint>> vertexVBO;
	s_ptr<Shader> shader;

	QuadRenderData();

	bool initShader();
};


class QuadRenderer
{
	s_ptr<QuadRenderData> renderData;
public:
	bool readyToRender = false;

	static QuadRenderer& get();

	UniformMap uniforms;

	bool initialized = false;

	vec4 globalColor = vec4(1.f);
	float globalWidth = 0.1f;

	bool init();

	void beginBatch(bool showBackface = true, unsigned int textureID = 0, bool allowBlend = true);
	void render(const vec3& position, const vec3& rotation, const vec3& scale, const vec4& color = vec4(1.0f));
	void endBatch();

	void renderUI();

private:
	int wireframeMode = 0;
	QuadRenderer() { }
	virtual ~QuadRenderer() { }
};

struct InstanceQuadRenderData {

	GLuint vao = 0;

	s_ptr<VectorBuffer<GLuint>> indexVBO;
	s_ptr<VectorBuffer<GLuint>> vertexVBO;
	s_ptr<Shader> shader;

	InstanceQuadRenderData();

	bool initShader();
};

struct ParticleSystem;
struct ParticleRenderData;

class InstanceQuadRenderer
{
	s_ptr<InstanceQuadRenderData> renderData;
public:
	bool readyToRender = false;

	spVectorBuffer<ParticleRenderData> currentParticles;
	size_t currentParticleCount = 0;

	static InstanceQuadRenderer& get();

	UniformMap uniforms;

	bool initialized = false;

	vec4 globalColor = vec4(1.f);
	float globalWidth = 0.1f;

	bool init();

	void beginBatch(ParticleSystem* ps, bool showBackface = true, unsigned int textureID = 0, bool allowBlend = true);
	void render(const vec4& mainColor = vec4(1.f), const vec4 uvOffset = vec4(0, 0, 1, 1));
	void endBatch();

	void renderUI();

private:
	int wireframeMode = 0;
	InstanceQuadRenderer() { }
	virtual ~InstanceQuadRenderer() { }
};