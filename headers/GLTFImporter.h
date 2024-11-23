#pragma once

#include "globals.h"
#include "Texture.h"

MAKE_ENUM(GLTFNodeType, int, node, mesh, camera, skin);

MAKE_ENUM(GLTFPrimitiveMode, int, POINTS, LINES, LINE_LOOP, LINE_STRIP, TRIANGLES, TRIANGLE_STRIP, TRIANGLE_FAN);

MAKE_ENUM(GLTFBufferTarget, uint32_t, ARRAY_BUFFER = GL_ARRAY_BUFFER, ELEMENT_ARRAY_BUFFER = GL_ELEMENT_ARRAY_BUFFER);

MAKE_ENUM(GLTFAccessorComponentType, uint32_t, BYTE = GL_BYTE, UNSIGNED_BYTE = GL_UNSIGNED_BYTE, SHORT = GL_SHORT, UNSIGNED_SHORT = GL_UNSIGNED_SHORT, UNSIGNED_INT = GL_UNSIGNED_INT, FLOAT = GL_FLOAT);

MAKE_ENUM(GLTFAccessorType, uint32_t, SCALAR, VEC2, VEC3, VEC4, MAT2, MAT3, MAT4);

MAKE_ENUM(GLTFAnimationTargetPath, uint32_t, translation, rotation, scale, weights);

MAKE_ENUM(GLTFAnimationSamplerInterpolation, uint32_t, LINEAR, STEP, CUBICSPLINE);


struct GLTFData;

struct GLTFSkin {
	std::string name;
	std::vector<mat4> inverseBindMatrices;
	std::vector<mat4> jointMatrices;
	int32_t inverseBindMatricesIndex = -1;
	int32_t skeleton = -1;
	std::vector<uint32_t> joints;
};

struct GLTFAnimationChannelTarget {
	uint32_t node;
	GLTFAnimationTargetPath path;
};

struct GLTFAnimationChannel {
	uint32_t sampler;
	GLTFAnimationChannelTarget target;
};

struct GLTFAnimationSampler {
	uint32_t input;
	GLTFAnimationSamplerInterpolation interpolation = GLTFAnimationSamplerInterpolation::LINEAR;
	uint32_t output;
};

struct GLTFAnimation {
	std::string name;
	std::vector<GLTFAnimationChannel> channels;
	std::vector<GLTFAnimationSampler> samplers;
};

struct GLTFBufferView {
	uint32_t bufferIndex = 0;
	uint32_t byteOffset = 0;
	uint32_t byteLength = 0;
	uint32_t byteStride = 0;
	uint32_t target = GLTFBufferTarget::ARRAY_BUFFER;
};

struct GLTFAccessor {
	int32_t bufferView = -1;
	uint32_t byteOffset = 0;
	GLTFAccessorComponentType componentType;
	bool normalized = false;
	uint32_t count = 0;
	GLTFAccessorType type;
	std::vector<float> max, min;
};

struct GLTFSampler {
	TextureFilterMode magFilter = TextureFilterMode::Nearest;
	TextureFilterMode minFilter = TextureFilterMode::Nearest;
	TextureWrapMode wrapS = TextureWrapMode::Repeat;
	TextureWrapMode wrapT = TextureWrapMode::Repeat;
	std::string name;

	void renderUI();
};

struct GLTFTextureInfo {
	int32_t index = -1;		// default: unused.
	uint32_t texCoord = 0;	// index of attribute: TEXCOORD_0 and higher
	float scale = 1.0f;		// for normal textures
	float strength = 1.0f;	// for occlusion textures

	void renderUI(s_ptr<GLTFData> gltf);
};

struct GLTFImage {
	std::string name;
	std::string uri;
	std::string mimeType;
	int32_t bufferView = -1;

	void renderUI(s_ptr<GLTFData> gltf);
};

struct GLTFTexture {
	std::string name;
	int32_t sampler = -1;
	int32_t source = -1;

	void renderUI(s_ptr<GLTFData> gltf);
};

struct GLTFPBRMetallicRoughness {
	vec4 baseColorFactor = vec4(1);
	GLTFTextureInfo baseColorTexture;
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
	GLTFTextureInfo metallicRoughnessTexture;

	void renderUI(s_ptr<GLTFData> gltf);
};



struct GLTFMaterial {
	std::string name;
	bool doubleSided = false;
	GLTFPBRMetallicRoughness pbr;
	GLTFTextureInfo normalTexture;
	GLTFTextureInfo occlusionTexture;
	GLTFTextureInfo emissiveTexture;
	vec3 emissiveFactor = vec3(0);
	TextureAlphaMode alphaMode;
	float alphaCutoff = 0.5f;

	// Non-GLTF properties:
	bool useLighting = true;

	void renderUI(s_ptr<GLTFData> gltf);
};

struct GLTFPrimitive {
	json attributes;
	int32_t indices = -1;
	int32_t material = -1;
	GLTFPrimitiveMode mode = GLTFPrimitiveMode::TRIANGLES;
};

struct GLTFMesh {
	std::string name;
	std::vector<GLTFPrimitive> primitives;
};

struct GLTFNode {
	uint32_t index = 0;
	uint32_t meshIndex;
	uint32_t cameraIndex;
	uint32_t skinIndex;
	GLTFNodeType type = GLTFNodeType::node;
	std::string name;
	vec3 translation = vec3(0);
	quaternion rotation = glm::identity<quaternion>();
	vec3 scale = vec3(1);

	vec3 bindTranslation = vec3(0);
	quaternion bindRotation = glm::identity<quaternion>();
	vec3 bindScale = vec3(1);

	mat4 matrix;

	s_ptr<GLTFNode> parent;
	bool updated = false;

	std::vector<uint32_t> children;

	void reset() {
		translation = bindTranslation;
		rotation = bindRotation;
		scale = bindScale;
	}

	std::string label = "";
};

using GLTFBuffer = std::vector<uint8_t>;

// Handles of GPU objects (vertex array object, index buffer object, and vertex buffer objects)
struct GLTFGPUData {
	GLuint VAO;
	GLuint IBO;
	std::vector<GLuint> VBOs;
};

class Shader;

struct GLTFRenderContext {
	s_ptr<Shader> shader;

	// Key: mesh index.
	// Value: collection of GPU data for primitives of this mesh
	std::map<uint32_t, std::vector<GLTFGPUData>> meshPrimitives;

	// Key: image index
	// Value: loaded texture
	std::map<uint32_t, s_ptr<Texture>> imageTextures;
};

struct GLTFData {
	std::string filename;
	s_ptr_vector<GLTFNode> nodes;
	s_ptr_vector<GLTFMesh> meshes;
	std::vector<GLTFAnimation> animations;
	std::vector<GLTFAccessor> accessors;
	std::vector<GLTFBufferView> bufferViews;
	std::vector<GLTFBuffer> buffers;
	std::vector<GLTFSkin> skins;
	std::vector<GLTFSampler> samplers;
	std::vector<GLTFMaterial> materials;
	std::vector<GLTFTexture> textures;
	std::vector<GLTFImage> images;

	s_ptr<GLTFRenderContext> context;
};

class GLTFImporter {
public:
	static s_ptr<GLTFData> import(std::string file);
};


class Framebuffer;
class GLTFRenderer {
	bool initialized = false;

public:
	static GLTFRenderer& get() {
		static GLTFRenderer renderer;
		return renderer;
	}
	void init(s_ptr<GLTFData> gltf);
	void render(const mat4& projection, const mat4& view, s_ptr<GLTFData> gltf, s_ptr<Framebuffer> framebuffer, bool isShadow);

	static void renderUI(s_ptr<GLTFData> gltf);
};
