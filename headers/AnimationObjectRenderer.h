#pragma once
#include "Buffer.h"
#include "Meshing.h"
#include "Shader.h"
#include "Uniform.h"


struct AnimationObjectRenderData {
	GLuint vao = 0;
	GLuint shadowVAO = 0;

	AnimationObjectType shapeType = AnimationObjectType::sphere;
	std::string objectName = "sphere";
	s_ptr<VectorBuffer<GLuint>> indexVBO;
	s_ptr<VectorBuffer<PlainOldVertex>> vertexVBO;
	s_ptr<Shader> shader;
	s_ptr<Shader> shadowShader;
	s_ptr<MeshData> mesh;

	AnimationObjectRenderData() { }
	AnimationObjectRenderData(AnimationObjectType st);
	AnimationObjectRenderData(std::string filename);


	bool initShader();
};


class AnimationObjectRenderer
{
public:
	static AnimationObjectRenderer& get();

	std::map<AnimationObjectType, AnimationObjectRenderData> shapeCatalog;
	std::map<std::string, AnimationObjectRenderData> meshCatalog;

	UniformMap uniforms;

	AnimationObjectRenderData* currentMesh = nullptr;

	bool initialized = false;

	vec4 globalColor = vec4(1.f);
	vec2 shadowBias = vec2(0.005f);

	bool init();

	void beginBatchRender(AnimationObjectType shapeType, bool overrideColor = false, const vec4& batchColor = vec4(1.f), bool isShadow = false);
	void beginBatchRender(const AnimationObject& shape, bool overrideColor = false, const vec4& batchColor = vec4(1.f), bool isShadow = false);
	void render(AnimationObjectType shapeType, const mat4& mat, bool singular = true, bool isShadow = false);
	void renderLight(const mat4& mat);
	void renderBatch(const mat4& mat, bool isShadow = false);
	void renderBatchWithOwnColor(const mat4& mat, const vec4& color, const GLuint texture = 0, bool isShadow = false);
	void renderBatchWithOwnColor(const AnimationObject& shape, bool isShadow = false, bool useShapeColor = true);
	void endBatchRender(bool isShadow = false);

	void renderUI();

private:
	int wireframeMode = 0;
	AnimationObjectRenderer() { }
	virtual ~AnimationObjectRenderer() { }
};