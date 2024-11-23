#pragma once

#include "AnimationObject.h"


struct PlainOldVertex
{
	vec3 position = vec3(0.f);
	vec3 normal = vec3(0.f);
	vec2 uv = vec2(0.f);
	vec4 color = vec4(1.f);
};

class Texture;

struct MaterialData {
	std::string filename;
	std::string materialName = "material";
	vec3 ambient = vec3(1.f);
	vec3 diffuse = vec3(1.f);
	vec3 specular = vec3(1.f);
	float dissolve = 1.0;
	vec3 transmissionFilter = vec3(1.f);
	// Index of refraction
	float Ni = 1.f;
	// Shininess
	float Ns = 1.f;
	uint32_t illum = 2;

	std::string ambientTextureFile;
	std::string diffuseTextureFile;
	std::string specularColorTextureFile;
	std::string specularHighlightTextureFile;
	std::string alphaTextureFile;
	std::string bumpTextureFile;
	std::string displacementTextureFile;
	std::string stencilTextureFile;

	s_ptr<Texture> ambientTexture;
	s_ptr<Texture> diffuseTexture;
	s_ptr<Texture> specularColorTexture;
	s_ptr<Texture> specularHighlightTexture;
	s_ptr<Texture> alphaTexture;
	s_ptr<Texture> bumpTexture;
	s_ptr<Texture> displacementTexture;
	s_ptr<Texture> stencilTexture;

	static std::vector<MaterialData> LoadFromFile(const std::string& filename);
};

struct MeshData {

	AnimationObject shape = AnimationObject(AnimationObjectType::model);

	// If true, all your data's array of structs (vertex_data).
	// If false, it's struct of arrays (positions, normals, uvs)
	bool isPacked = true;

	std::vector<PlainOldVertex> vertex_data;
	std::vector<vec3> positions;
	std::vector<vec3> normals;
	std::vector<vec2> uvs;
	std::vector<uint32_t> indices;

	std::vector<MaterialData> materials;
};

s_ptr<MeshData> LoadOBJModel(std::string filename, bool packIt = true);