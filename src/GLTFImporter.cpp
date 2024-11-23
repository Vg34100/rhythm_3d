#include "GLTFImporter.h"

#include "AnimationObjectRenderer.h"
#include "Application.h"
#include "Buffer.h"
#include "Framebuffer.h"
#include "Lighting.h"
#include "Renderer.h"
#include "Shader.h"
#include "StringUtil.h"

#include <glm/gtx/string_cast.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "UIHelpers.h"

// Basic elements in GLTF file:
// https://github.khronos.org/glTF-Tutorials/gltfTutorial/gltfTutorial_002_BasicGltfStructure.html
const char* gltfElements[] = {
	"scene",
	"node",
	"mesh",
	"camera",
	"skin",
	"material",
	"accessor",
	"texture",
	"sampler",
	"image",
	"bufferView",
	"buffer",
	"animation"
};


/*
* struct GLTFAnimationChannelTarget {
	uint32_t node;
	GLTFAnimationTargetPath path;
};


GLTFAnimationSampler parseAnimationSampler(json & sampler){
	uint32_t input;
	GLTFAnimationSamplerInterpolation interpolation = GLTFAnimationSamplerInterpolation::LINEAR;
	uint32_t output;
};

struct GLTFAnimation {
	std::string name;
	std::vector<GLTFAnimationChannel> channels;
	std::vector<GLTFAnimationSampler> samplers;
};
*/

GLTFSkin parseSkin(json& skin) {
	log("Parsing skin\n");
	GLTFSkin newSkin;

	if (skin.contains("name")) newSkin.name = skin["name"];
	if (skin.contains("inverseBindMatrices")) newSkin.inverseBindMatricesIndex = skin["inverseBindMatrices"];
	if (skin.contains("skeleton")) newSkin.skeleton = skin["skeleton"];
	if (skin.contains("joints")) newSkin.joints = skin["joints"].get<std::vector<uint32_t>>();

	auto maxJoint = *std::max_element(newSkin.joints.begin(), newSkin.joints.end());

	newSkin.inverseBindMatrices.resize(maxJoint + 1, mat4(1.f));
	newSkin.jointMatrices.resize(maxJoint + 1, mat4(1.f));

	return newSkin;
}


GLTFAnimationSampler parseAnimationSampler(json& sampler) {
	log("Parsing animation sampler\n");
	GLTFAnimationSampler newSampler;
	if (sampler.contains("input")) newSampler.input = sampler["input"];
	if (sampler.contains("interpolation")) newSampler.interpolation = GLTFAnimationSamplerInterpolation::_from_string(sampler["interpolation"].get<std::string>().c_str());
	if (sampler.contains("output")) newSampler.output = sampler["output"];

	return newSampler;
}

GLTFAnimationChannelTarget parseAnimationChannelTarget(json& target) {
	log("Parsing animation channel target\n");
	GLTFAnimationChannelTarget newTarget;


	if (target.contains("node")) newTarget.node = target["node"];
	if (target.contains("path")) newTarget.path = GLTFAnimationTargetPath::_from_string(target["path"].get<std::string>().c_str());

	return newTarget;
}


GLTFAnimationChannel parseAnimationChannel(json& channel) {
	log("Parsing animation channel\n");
	GLTFAnimationChannel newChannel;

	if (channel.contains("sampler")) newChannel.sampler = channel["sampler"];
	if (channel.contains("target")) newChannel.target = parseAnimationChannelTarget(channel["target"]);

	return newChannel;
}

GLTFAnimation parseAnimation(json& animation) {
	log("Parsing animation\n");
	GLTFAnimation anim;

	if (animation.contains("name")) anim.name = animation["name"];

	if (animation.contains("channels")) {
		for (auto& channel : animation["channels"]) {
			anim.channels.push_back(parseAnimationChannel(channel));
		}
	}

	if (animation.contains("samplers")) {
		for (auto& sampler : animation["samplers"]) {
			anim.samplers.push_back(parseAnimationSampler(sampler));
		}
	}

	return anim;
}

GLTFBufferView parseBufferView(const json& bufferView) {
	log("Parsing bufferView\n");

	GLTFBufferView bv;

	if (bufferView.contains("buffer")) bv.bufferIndex = bufferView["buffer"];
	if (bufferView.contains("byteOffset")) bv.byteOffset = bufferView["byteOffset"];
	if (bufferView.contains("byteLength")) bv.byteLength = bufferView["byteLength"];
	if (bufferView.contains("byteStride")) bv.byteStride = bufferView["byteStride"];
	if (bufferView.contains("target")) bv.target = bufferView["target"];

	return bv;
}

static inline bool is_base64(uint8_t c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

// https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
GLTFBuffer base64_decode(const char* b64, size_t len) {
	// all base64 characters
	static const std::string base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	int in_len = len;
	int i = 0;
	int j = 0;
	int in_ = 0;
	uint8_t char_array_4[4], char_array_3[3];
	GLTFBuffer ret;

	while (in_len-- && (b64[in_] != '=') && is_base64(b64[in_])) {
		char_array_4[i++] = b64[in_]; in_++;
		if (i == 4) {
			for (i = 0; i < 4; i++)
				char_array_4[i] = base64Chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret.push_back(char_array_3[i]);
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = base64Chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
	}

	log("base64 uri decoded into {0} bytes\n", ret.size());

	return ret;
}

GLTFBuffer parseBuffer(json& buffer) {


	const std::string& uri = buffer["uri"];
	const int length = buffer["byteLength"];

	log("Parsing buffer of {0} bytes\n", length);

	// See if data is base-64 encoded
	static std::string b64DataStr = "data:application/octet-stream;base64,";
	static std::string b64GLTFStr = "data:application/gltf-buffer;base64,";
	auto b64Start = uri.find(b64DataStr, 0);
	auto b64Start2 = uri.find(b64GLTFStr, 0);
	// Decode URI 
	if (b64Start != std::string::npos) {
		const char* b64 = uri.c_str() + b64DataStr.length();

		return base64_decode(b64, uri.length() - b64DataStr.length());
	}
	// Load URI from file
	else if (b64Start2 != std::string::npos) {
		const char* b64 = uri.c_str() + b64GLTFStr.length();
		return base64_decode(b64, uri.length() - b64GLTFStr.length());
	}
	else {
		std::ifstream fin(uri, std::ios::binary);
		if (fin) {

			GLTFBuffer newBuffer(length);

			fin.read((char*)newBuffer.data(), length);

			log("Buffer uri {0} read from file, {1} bytes\n", uri, newBuffer.size());

			return newBuffer;
		}
		else {
			log("File not found: {0}\n", uri);
			exit(1);
		}
	}
}


GLTFAccessor parseAccessor(json& accessor) {
	log("Parsing accessor\n");
	GLTFAccessor acc;

	if (accessor.contains("bufferView"))		acc.bufferView = accessor["bufferView"];
	if (accessor.contains("byteOffset"))		acc.byteOffset = accessor["byteOffset"];
	if (accessor.contains("componentType"))		acc.componentType = GLTFAccessorComponentType::_from_integral(accessor["componentType"]);
	if (accessor.contains("normalized"))		acc.normalized = accessor["normalized"];
	if (accessor.contains("count"))				acc.count = accessor["count"];
	if (accessor.contains("type"))				acc.type = GLTFAccessorType::_from_string(accessor["type"].get<std::string>().c_str());
	if (accessor.contains("min"))				acc.min = accessor["min"].get<std::vector<float>>();
	if (accessor.contains("max"))				acc.max = accessor["max"].get<std::vector<float>>();

	return acc;
}

GLTFImage parseImage(json& image) {
	GLTFImage i;

	if (image.contains("name")) i.name = image["name"];
	if (image.contains("uri")) i.name = image["uri"];
	if (image.contains("mimeType")) i.mimeType = image["mimeType"];
	if (image.contains("bufferView")) i.bufferView = image["bufferView"];

	return i;
}

GLTFSampler parseSampler(json& sampler) {
	GLTFSampler s;

	if (sampler.contains("magFilter")) s.magFilter = TextureFilterMode::_from_integral(sampler["magFilter"]);
	if (sampler.contains("minFilter")) s.minFilter = TextureFilterMode::_from_integral(sampler["minFilter"]);
	if (sampler.contains("wrapS")) s.wrapS = TextureWrapMode::_from_integral(sampler["wrapS"]);
	if (sampler.contains("wrapT")) s.wrapT = TextureWrapMode::_from_integral(sampler["wrapT"]);
	if (sampler.contains("name")) s.name = sampler["name"];

	return s;
}

GLTFTextureInfo parseTextureInfo(json& texInfo) {
	log("Parsing texture info\n");
	GLTFTextureInfo info;

	if (texInfo.contains("index")) info.index = texInfo["index"];
	if (texInfo.contains("texCoord")) info.texCoord = texInfo["texCoord"];
	if (texInfo.contains("scale")) info.scale = texInfo["scale"];
	if (texInfo.contains("strength")) info.strength = texInfo["strength"];

	return info;
}

GLTFTexture parseTexture(json& texture) {
	log("Parsing texture\n");
	GLTFTexture newTex;

	if (texture.contains("name")) newTex.name = texture["name"];
	if (texture.contains("sampler")) newTex.sampler = texture["sampler"];
	if (texture.contains("source")) newTex.source = texture["source"];

	return newTex;
}

GLTFPBRMetallicRoughness parsePBR(json& pbr) {
	GLTFPBRMetallicRoughness newPBR;
	if (pbr.contains("baseColorFactor")) newPBR.baseColorFactor = pbr["baseColorFactor"];
	if (pbr.contains("baseColorTexture")) newPBR.baseColorTexture = parseTextureInfo(pbr["baseColorTexture"]);
	if (pbr.contains("metallicFactor")) newPBR.metallicFactor = pbr["metallicFactor"];
	if (pbr.contains("roughnessFactor")) newPBR.roughnessFactor = pbr["roughnessFactor"];
	if (pbr.contains("metallicRoughnessTexture")) newPBR.metallicRoughnessTexture = parseTextureInfo(pbr["metallicRoughnessTexture"]);

	return newPBR;
}

GLTFMaterial parseMaterial(json& material) {
	log("Parsing material\n");
	GLTFMaterial newMat;
	if (material.contains("name")) newMat.name = material["name"];
	if (material.contains("pbrMetallicRoughness")) newMat.pbr = parsePBR(material["pbrMetallicRoughness"]);
	if (material.contains("normalTexture")) newMat.normalTexture = parseTextureInfo(material["normalTexture"]);
	if (material.contains("occlusionTexture")) newMat.occlusionTexture = parseTextureInfo(material["occlusionTexture"]);
	if (material.contains("emissiveTexture")) newMat.emissiveTexture = parseTextureInfo(material["emissiveTexture"]);
	if (material.contains("emissiveFactor")) newMat.emissiveFactor = material["emissiveFactor"];
	if (material.contains("alphaMode")) newMat.alphaMode = TextureAlphaMode::_from_string(material["alphaMode"].get<std::string>().c_str());
	if (material.contains("alphaCutoff")) newMat.alphaCutoff = material["alphaCutoff"];
	if (material.contains("doubleSided")) newMat.doubleSided = material["doubleSided"];

	return newMat;
}

void parseCamera(json& camera) {
	log("Parsing camera\n");
}

GLTFPrimitive parsePrimitive(json& prim) {
	log("Parsing mesh primitive\n");
	GLTFPrimitive newPrim;
	if (prim.contains("attributes")) newPrim.attributes = prim["attributes"];
	if (prim.contains("indices")) newPrim.indices = prim["indices"];
	if (prim.contains("material")) newPrim.material = prim["material"];
	if (prim.contains("mode")) newPrim.mode = prim["mode"];

	for (auto& a : newPrim.attributes.items()) {
		log("\t{0}: {1}\n", a.key(), a.value().get<int>());
	}
	log("\tindices: {0}\n\tmaterial: {1}\n", newPrim.indices, newPrim.material);

	return newPrim;
}

s_ptr<GLTFMesh> parseMesh(json& mesh) {
	log("Parsing mesh\n");
	s_ptr<GLTFMesh> newMesh = std::make_shared<GLTFMesh>();


	if (mesh.contains("name")) newMesh->name = mesh["name"];

	log("mesh name {0}\n", newMesh->name);

	for (const auto& k : mesh.items()) {
		if (k.key() == "primitives") {
			for (auto& prim : k.value()) {
				newMesh->primitives.push_back(std::move(parsePrimitive(prim)));
			}
		}
	}

	return newMesh;
}

s_ptr<GLTFNode> parseNode(json& node) {

	s_ptr<GLTFNode> newNode = std::make_shared<GLTFNode>();
	log("Parsing node\n");
	for (const auto& k : node.items()) {
		if (k.key() == "children") {
			newNode->children = k.value().get<std::vector<uint32_t>>();
		}
		else if (k.key() == "mesh") {
			newNode->meshIndex = k.value();
			newNode->type = GLTFNodeType::mesh;
			//parseMesh(k.value());
		}
		else if (k.key() == "camera") {
			newNode->cameraIndex = k.value();
			newNode->type = GLTFNodeType::camera;
			//parseCamera(k.value());
		}
		else if (k.key() == "skin") {
			newNode->skinIndex = k.value();
			newNode->type = GLTFNodeType::skin;
			//parseSkin(k.value());
		}
		else if (k.key() == "translation") {
			newNode->translation = k.value();
			newNode->bindTranslation = k.value();
		}
		else if (k.key() == "rotation") {
			newNode->rotation = k.value();
			newNode->bindRotation = k.value();
		}
		else if (k.key() == "scale") {
			newNode->scale = k.value();
			newNode->bindScale = k.value();
		}

		else if (k.key() == "matrix") {
			newNode->matrix = k.value();
			auto dc = glm::decompose(newNode->matrix);
			newNode->translation = dc.position;
			newNode->rotation = dc.rotation;
			newNode->scale = dc.scale;
		}

		else if (k.key() == "name") {
			newNode->name = k.value();
		}
	}

	log("{0}:\n\ttranslate: {1}\n\trotate: {2}\n\tscale: {3}\n",
		newNode->name, glm::to_string(newNode->translation), glm::to_string(newNode->rotation), glm::to_string(newNode->scale));


	return newNode;

}

s_ptr<GLTFData> parseGLTFFile(std::string file) {
	log("Parsing {0}\n", file);
	json gltf;

	std::ifstream fin(file);
	if (!fin) {
		log("Unable to find file: {0}\n", file);
		return nullptr;
	}

	fin >> gltf;

	s_ptr<GLTFData> data = std::make_shared<GLTFData>();


	for (const auto& k : gltf.items()) {
		if (k.key() == "nodes") {
			for (auto& node : k.value()) {
				auto newNode = parseNode(node);
				newNode->index = data->nodes.size();
				data->nodes.push_back(newNode);
			}
		}

		if (k.key() == "animations") {
			for (auto& anim : k.value()) {
				data->animations.push_back(parseAnimation(anim));
			}
		}

		if (k.key() == "materials") {
			for (auto& mat : k.value()) {
				data->materials.push_back(parseMaterial(mat));
			}
		}

		if (k.key() == "meshes") {
			for (auto& mesh : k.value()) {
				data->meshes.push_back(parseMesh(mesh));
			}
		}

		if (k.key() == "textures") {
			for (auto& image : k.value()) {
				data->textures.push_back(parseTexture(image));
			}
		}
		if (k.key() == "images") {
			for (auto& image : k.value()) {
				data->images.push_back(parseImage(image));
			}
		}

		if (k.key() == "accessors") {
			for (auto& accessor : k.value()) {
				data->accessors.push_back(parseAccessor(accessor));
			}
		}

		if (k.key() == "bufferViews") {
			for (auto& bufferView : k.value()) {
				data->bufferViews.push_back(parseBufferView(bufferView));
			}
		}
		if (k.key() == "samplers") {
			for (auto& sampler : k.value()) {
				data->samplers.push_back(parseSampler(sampler));
			}
		}

		if (k.key() == "buffers") {
			for (auto& buffer : k.value()) {
				data->buffers.push_back(std::move(parseBuffer(buffer)));
			}
		}

		if (k.key() == "skins") {
			for (auto& skin : k.value()) {
				data->skins.push_back(std::move(parseSkin(skin)));
			}
		}
	}

	// Set parent pointers
	for (auto& node : data->nodes) {
		for (auto& ci : node->children) {
			data->nodes[ci]->parent = node;
		}
	}

	return data;
}

s_ptr<GLTFData> GLTFImporter::import(std::string file) {
	//std::ifstream fin(file);

	if (StringUtil::contains(file, ".gltf")) {
		auto gltf = parseGLTFFile(file);
		return gltf;
	}

	return nullptr;
}



void GLTFRenderer::init(s_ptr<GLTFData> gltf) {
	if (gltf->context) return;

	auto context = std::make_shared<GLTFRenderContext>();

	// Init shader
	auto vertText = Shader::LoadText("objects/gltfmesh.vert");
	auto fragText = Shader::LoadText("objects/gltfmesh.frag");

	context->shader = s_ptr<Shader>(new Shader(vertText, fragText));

	std::vector<ShaderFragOutputBindings> fragBindings = {
		{0, "frag_color"},
		{1, "frag_prim"}
	};

	if (!context->shader->init(false, fragBindings))
	{
		log("Unable to load GLTF shader!\n");
		return;
	}

	context->shader->start();

	for (auto& node : gltf->nodes) {
		if (node->type == +GLTFNodeType::mesh) {
			auto& mesh = gltf->meshes[node->meshIndex];

			if (context->meshPrimitives.find(node->meshIndex) != context->meshPrimitives.end()) continue;

			std::vector<GLTFGPUData> gpu;

			for (auto& prim : mesh->primitives) {

				GLTFGPUData primGPU;
				glGenVertexArrays(1, &primGPU.VAO);

				glBindVertexArray(primGPU.VAO);

				for (auto& attribute : prim.attributes.items()) {
					const auto& attribName = attribute.key();

					auto& accessor = gltf->accessors[attribute.value()];

					if (accessor.bufferView >= 0) {
						const auto& bufferView = gltf->bufferViews[accessor.bufferView];

						const auto& buffer = gltf->buffers[bufferView.bufferIndex];

						GLuint attribBuffer = 0;
						glGenBuffers(1, &attribBuffer);
						primGPU.VBOs.push_back(attribBuffer);
						glBindBuffer(GL_ARRAY_BUFFER, attribBuffer);

						glBufferData(GL_ARRAY_BUFFER, bufferView.byteLength, buffer.data() + bufferView.byteOffset, GL_STATIC_DRAW);

						if (attribName == "POSITION") {
							auto loc = glGetAttribLocation(context->shader->program, "position");
							glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, bufferView.byteStride, nullptr);
							glEnableVertexAttribArray(loc);
						}
						else if (attribName == "NORMAL") {
							auto loc = glGetAttribLocation(context->shader->program, "normal");
							glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, bufferView.byteStride, nullptr);
							glEnableVertexAttribArray(loc);
						}
						else if (attribName == "TEXCOORD_0") {
							auto loc = glGetAttribLocation(context->shader->program, "uv");
							glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, bufferView.byteStride, nullptr);
							glEnableVertexAttribArray(loc);
						}
					}
				}

				if (prim.indices >= 0) {
					const auto& primIndicesAccessor = gltf->accessors[prim.indices];
					if (primIndicesAccessor.bufferView) {
						const auto& primIndicesBufferView = gltf->bufferViews[primIndicesAccessor.bufferView];
						const auto& primIndicesBuffer = gltf->buffers[primIndicesBufferView.bufferIndex];

						glGenBuffers(1, &primGPU.IBO);
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, primGPU.IBO);
						glBufferData(GL_ELEMENT_ARRAY_BUFFER, primIndicesBufferView.byteLength, primIndicesBuffer.data() + primIndicesBufferView.byteOffset, GL_STATIC_DRAW);
					}
				}

				glBindVertexArray(0);

				gpu.push_back(primGPU);
			}

			context->meshPrimitives[node->meshIndex] = gpu;

		}
	}

	//for (auto& image : gltf->images) {
	for (uint32_t i = 0; i < gltf->images.size(); i++) {
		auto& image = gltf->images[i];
		if (image.bufferView >= 0) {
			// Need to create a texture from this image using data from its bufferView
			const auto& imgBufferView = gltf->bufferViews[image.bufferView];
			// Get pointer to data
			const uint8_t* data = gltf->buffers[imgBufferView.bufferIndex].data() + imgBufferView.byteOffset;

			context->imageTextures[i] = std::make_shared<Texture>(data, imgBufferView.byteLength);
		}
	}

	gltf->context = context;
}

void GLTFRenderer::render(const mat4& projection, const mat4& view, s_ptr<GLTFData> gltf, s_ptr<Framebuffer> framebuffer, bool isShadow) {
	if (gltf->context == nullptr) {
		init(gltf);
	}

	static mat4 lightmvp;

	if (isShadow) {
		lightmvp = Application::get().renderer->camera.viewproj;
		return;

		// No shadow rendering support yet...
	}

	gltf->context->shader->start();

	GLint nmLoc = glGetUniformLocation(gltf->context->shader->program, "normalMatrix");
	mat4 normalMatrix = glm::transpose(glm::inverse(Application::get().renderer->camera.view));
	glUniformMatrix3fv(nmLoc, 1, GL_FALSE, (const GLfloat*)glm::value_ptr(normalMatrix));

	GPU::Lighting::get().bind(gltf->context->shader);


	if (!isShadow) {
		GLint lightmvpLoc = glGetUniformLocation(gltf->context->shader->program, "lightmvp");
		glUniformMatrix4fv(lightmvpLoc, 1, GL_FALSE, glm::value_ptr(lightmvp));
	}

	for (auto& node : gltf->nodes) {
		if (node->type == +GLTFNodeType::mesh) {
			auto& mesh = gltf->meshes[node->meshIndex];

			if (gltf->context->meshPrimitives.find(node->meshIndex) == gltf->context->meshPrimitives.end()) continue;

			const auto& gpu = gltf->context->meshPrimitives[node->meshIndex];

			node->matrix = glm::translate(node->translation) * glm::toMat4(node->rotation) * glm::scale(node->scale);

			if (node->parent) {
				node->matrix = node->parent->matrix * node->matrix;
			}

			mat4 mvp = projection * view * node->matrix;

			auto mvpLoc = glGetUniformLocation(gltf->context->shader->program, "mvp");
			glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
			auto modelLoc = glGetUniformLocation(gltf->context->shader->program, "model");
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(node->matrix));

			for (uint32_t i = 0; i < mesh->primitives.size(); i++) {
				auto& prim = mesh->primitives[i];
				const auto& primGPU = gpu[i];

				static GLTFMaterial defaultMaterial;
				const GLTFMaterial* primMaterial = &defaultMaterial;
				if (prim.material >= 0) {
					primMaterial = &gltf->materials[prim.material];

					if (primMaterial->doubleSided) {
						glDisable(GL_CULL_FACE);
					}
					else {
						glEnable(GL_CULL_FACE);
					}

					GLint ulLoc = glGetUniformLocation(gltf->context->shader->program, "useLighting");
					glUniform1i(ulLoc, primMaterial->useLighting);

					auto bcfLoc = glGetUniformLocation(gltf->context->shader->program, "baseColorFactor");
					glUniform4fv(bcfLoc, 1, glm::value_ptr(primMaterial->pbr.baseColorFactor));

					auto acLoc = glGetUniformLocation(gltf->context->shader->program, "alphaCutoff");
					glUniform1f(acLoc, primMaterial->alphaCutoff);

					GLint usLoc = glGetUniformLocation(gltf->context->shader->program, "useShadow");
					glUniform1i(usLoc, 0);


					glActiveTexture(GL_TEXTURE0);
					GLint itLoc = glGetUniformLocation(gltf->context->shader->program, "inputTexture");
					glUniform1i(itLoc, 0);

					GLint utLoc = glGetUniformLocation(gltf->context->shader->program, "useTexture");
					if (primMaterial->pbr.baseColorTexture.index >= 0) {

						const GLTFTexture& tex = gltf->textures[primMaterial->pbr.baseColorTexture.index];

						if (tex.source >= 0) {
							GLuint texture = gltf->context->imageTextures[tex.source]->id;
							glUniform1i(utLoc, 1);
							glBindTexture(GL_TEXTURE_2D, texture);
							// Set sampler
							if (tex.sampler >= 0) {
								const GLTFSampler& sampler = gltf->samplers[tex.sampler];
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS._to_integral());
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT._to_integral());
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler.minFilter._to_integral());
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler.magFilter._to_integral());


							}
						}
						else {
							glUniform1i(utLoc, 0);
							glBindTexture(GL_TEXTURE_2D, 0);
						}

					}
					else {
						glUniform1i(utLoc, 0);
						glBindTexture(GL_TEXTURE_2D, 0);
					}
				}

				// Render with shadows
				GLint smLoc = glGetUniformLocation(gltf->context->shader->program, "useShadow");
				glActiveTexture(GL_TEXTURE1);
				auto ogl = std::dynamic_pointer_cast<OpenGLRenderer>(Application::get().renderer);
				if (ogl && ogl->shadowMap) {
					glUniform1i(smLoc, 1);
					GLint stLoc = glGetUniformLocation(gltf->context->shader->program, "shadowTexture");
					glUniform1i(stLoc, 1);
					glBindTexture(GL_TEXTURE_2D, ogl->shadowMap->textures.front()->id);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
					float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
					glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

					GLint sbLoc = glGetUniformLocation(gltf->context->shader->program, "shadowBias");
					auto shadowBias = AnimationObjectRenderer::get().shadowBias;
					glUniform2fv(sbLoc, 1, glm::value_ptr(shadowBias));

					auto& l = GPU::Lighting::get();
					GLint lpLoc = glGetUniformLocation(gltf->context->shader->program, "lightPosition");
					GLint ldLoc = glGetUniformLocation(gltf->context->shader->program, "lightDirection");

					glUniform3fv(lpLoc, 1, glm::value_ptr(l.position));
					auto lightDir = glm::normalize(l.center - l.position);
					glUniform3fv(ldLoc, 1, glm::value_ptr(lightDir));
				}
				else {
					glUniform1i(smLoc, 0);
					glBindTexture(GL_TEXTURE_2D, 0);
				}

				//glActiveTexture(GL_TEXTURE1);
				//glBindTexture(GL_TEXTURE_2D, 0);
				//GLint stLoc = glGetUniformLocation(gltf->context->shader->program, "shadowTexture");
				//glUniform1i(stLoc, 1);

				glBindVertexArray(primGPU.VAO);

				// If we have an index buffer, draw this primitive with glDrawElements
				if (prim.indices >= 0) {
					const auto& primIndicesAccessor = gltf->accessors[prim.indices];
					if (primIndicesAccessor.bufferView) {
						const auto& primIndicesBufferView = gltf->bufferViews[primIndicesAccessor.bufferView];
						const auto& primIndicesBuffer = gltf->buffers[primIndicesBufferView.bufferIndex];
						glDrawElements(GL_TRIANGLES, primIndicesAccessor.count, primIndicesAccessor.componentType._to_integral(), 0);
					}
				}
				// Without an index buffer, we assume we're drawing triangles using sets of 3 vertices
				// at a time from the established buffer.
				else {
					// How many triangles to draw? This probably comes from the primitive attribute data, namely position.
					if (prim.attributes.contains("POSITION")) {
						const auto& primPositionAccessor = gltf->accessors[prim.attributes["POSITION"].get<uint32_t>()];
						glDrawArrays(GL_TRIANGLES, 0, primPositionAccessor.count);
					}
				}

				glBindVertexArray(0);
			}
		}
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);

	gltf->context->shader->stop();
}

void GLTFSampler::renderUI() {
	ImGui::PushID((const void*)this);
	const char* label = name != "" ? name.c_str() : "Sampler";
	if (ImGui::CollapsingHeader(label)) {
		renderEnumDropDown<TextureFilterMode>("Mag filter", magFilter);
		renderEnumDropDown<TextureFilterMode>("Min filter", minFilter);
		renderEnumDropDown<TextureWrapMode>("wrapS", wrapS);
		renderEnumDropDown<TextureWrapMode>("wrapT", wrapT);
	}
	ImGui::PopID();
}

void GLTFTextureInfo::renderUI(s_ptr<GLTFData> gltf) {
	ImGui::PushID((const void*)this);
	if (ImGui::CollapsingHeader("Texture info")) {
		if (index >= 0) {
			auto& tex = gltf->textures[index];
			tex.renderUI(gltf);
		}
		int tc = texCoord;
		if (ImGui::InputInt("Tex Coord", &tc) && tc >= 0) {
			texCoord = tc;
		}
		ImGui::InputFloat("Scale", &scale);
		ImGui::InputFloat("Strength", &strength);
	}
	ImGui::PopID();
}

void GLTFImage::renderUI(s_ptr<GLTFData> gltf) {
	ImGui::PushID((const void*)this);
	const char* label = name != "" ? name.c_str() : "Image";
	if (ImGui::CollapsingHeader(label)) {
		ImGui::Text("Mime type: %s", mimeType.c_str());
	}
	ImGui::PopID();
}

void GLTFTexture::renderUI(s_ptr<GLTFData> gltf) {
	ImGui::PushID((const void*)this);
	const char* label = name != "" ? name.c_str() : "Texture";
	if (ImGui::CollapsingHeader(label)) {
		if (sampler >= 0) {
			auto& sampRef = gltf->samplers[sampler];
			sampRef.renderUI();
		}
		if (source >= 0) {
			auto srcRef = gltf->images[source];
			srcRef.renderUI(gltf);
			if (gltf->context->imageTextures.find(source) != gltf->context->imageTextures.end()) {
				auto& imageTex = gltf->context->imageTextures[source];
				imageTex->renderUI();
			}
		}
	}
	ImGui::PopID();
}

void GLTFPBRMetallicRoughness::renderUI(s_ptr<GLTFData> gltf) {
	ImGui::PushID((const void*)this);
	if (ImGui::CollapsingHeader("PRB")) {
		/*
		* 	vec4 baseColorFactor = vec4(1);
	GLTFTextureInfo baseColorTexture;
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
	GLTFTextureInfo metallicRoughnessTexture;
		*/

		ImGui::ColorEdit4("Base color factor", glm::value_ptr(baseColorFactor));
		if (baseColorTexture.index >= 0) {
			baseColorTexture.renderUI(gltf);
		}
		ImGui::InputFloat("Metallic factor", &metallicFactor);
		ImGui::InputFloat("Roughness factor", &roughnessFactor);
		if (metallicRoughnessTexture.index >= 0) {
			metallicRoughnessTexture.renderUI(gltf);
		}
	}
	ImGui::PopID();
}

void GLTFMaterial::renderUI(s_ptr<GLTFData> gltf) {
	ImGui::PushID((const void*)this);
	if (ImGui::CollapsingHeader(name.c_str())) {
		ImGui::Checkbox("Double-sided", &doubleSided);
		pbr.renderUI(gltf);
		ImGui::InputFloat3("Emissive factor", glm::value_ptr(emissiveFactor));
		renderEnumDropDown<TextureAlphaMode>("Alpha mode", alphaMode);
		ImGui::InputFloat("Alpha cutoff", &alphaCutoff);
		ImGui::Checkbox("Use lighting", &useLighting);
	}


	ImGui::PopID();
}

void renderUIGLTFSkin(GLTFSkin & skin, s_ptr<GLTFData> gltf) {
	ImGui::PushID((const void*)&skin);

	if (ImGui::CollapsingHeader(skin.name.c_str())) {
		IMDENT;
		ImGui::Text("Joints: ");
		for (auto& joint : skin.joints) {
			ImGui::Text("%d", joint);
		}

		IMDONT;
	}

	ImGui::PopID();
}

void renderUIGLTFNode(s_ptr<GLTFNode> node, s_ptr<GLTFData> gltf) {
	ImGui::PushID((const void*)node.get());
	if (node->label == "") {
		node->label = fmt::format("{0}: {1}", node->index, node->name);
	}
	
	if (ImGui::CollapsingHeader(node->label.c_str())) {
		IMDENT;
		ImGui::Text("Index: %d", node->index);
		ImGui::Text("Type: %s", node->type._to_string());
		ImGui::InputFloat3("Translation", glm::value_ptr(node->translation));
		ImGui::InputFloat4("Rotation", glm::value_ptr(node->rotation));
		ImGui::InputFloat3("Scale", glm::value_ptr(node->scale));


		for (auto& child : node->children) {
			auto nc = gltf->nodes[child];
			renderUIGLTFNode(nc, gltf);
		}

		IMDONT;
	}

	ImGui::PopID();
}

void GLTFRenderer::renderUI(s_ptr<GLTFData> gltf) {
	IMDENT;
	if (ImGui::CollapsingHeader("Nodes")) {
		for (s_ptr<GLTFNode> node : gltf->nodes) {
			IMDENT;
			renderUIGLTFNode(node, gltf);
			IMDONT;
		}
	}
	if (ImGui::CollapsingHeader("Skins")) {
		for (auto & skin : gltf->skins) {
			IMDENT;
			renderUIGLTFSkin(skin, gltf);
			IMDONT;
		}
	}
	if (ImGui::CollapsingHeader("Materials")) {
		for (auto& mat : gltf->materials) {
			mat.renderUI(gltf);
		}
	}
	if (ImGui::CollapsingHeader("Textures")) {
		for (auto& tex : gltf->textures) {
			tex.renderUI(gltf);
		}
	}

	IMDONT;
}