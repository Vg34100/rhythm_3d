#pragma once

#include "globals.h"
#include "Meshing.h"
#include "Buffer.h"
#include "Shader.h"
#include "Uniform.h"



struct LineRenderData {

	GLuint vao = 0;

	s_ptr<VectorBuffer<GLuint>> indexVBO;
	s_ptr<VectorBuffer<GLuint>> vertexVBO;
	s_ptr<Shader> shader;

	LineRenderData();

	bool initShader();
};


class LineRenderer
{
	s_ptr<LineRenderData> renderData;
public:
	static LineRenderer& get();

	UniformMap uniforms;

	bool initialized = false;

	vec4 globalColor = vec4(1.f);
	float globalWidth = 0.1f;

	bool init();

	void begin();
	void render(const vec3& p1, const vec3& p2, const vec4& color = vec4(1.0f), float width = 0.1f);
	void renderSingle(const vec3& p1, const vec3& p2, const vec4& color = vec4(1.0f), float width = 0.1f);
	void end();

	void renderUI();

private:
	int wireframeMode = 0;
	LineRenderer() { }
	virtual ~LineRenderer() { }
};