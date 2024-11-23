// CMPS 4480 Lab 09
// Name: 

#include "Lab09.h"
#include "Application.h"
#include "AnimationObjectRenderer.h"
#include "GLTFImporter.h"
#include "InputOutput.h"
#include "Lighting.h"
#include "Prompts.h"
#include "UIHelpers.h"
#include "Textures.h"

#include <cassert>

#include <glm/gtc/random.hpp>

#include "implot.h"


// Variables for Lab 09
namespace cmps_4480_lab_09 {
	// TODO: change this to not be a nullptr
	const char* yourName = "Nick";


	// Variables for time and frame progression
	int numFrames = 120;
	int currentFrame = 0;
	int FPS = 24;					// frames per second
	double SPF = 1.0 / 24;			// seconds per frame (1/FPS)
	double lastFrameChange = 0;		// timestamp of the last frame change
	double fixedSecondsElapsed = 0;

	bool isPlaying = false;

	std::string skinnedMeshPath = IO::getAssetRoot() + "/" + "cylinder.gltf";
	std::string shaderPath = IO::getAssetRoot() + "/src/Assignments/skinnedMesh";
	s_ptr<GLTFData> meshWithSkin;

	AnimationObject ground;	
	AnimationObject joint = AnimationObject(AnimationObjectType::sphere);

	std::vector<vec4> jointColors;

	void initContext();
	void updateNodeMatrix(s_ptr<GLTFNode> node);
}

using namespace cmps_4480_lab_09;

namespace cmps_4480_lab_09 {
	void initContext() {
		auto context = std::make_shared<GLTFRenderContext>();

		// Init shader
		std::string vertText = IO::readText(shaderPath + ".vert");
		std::string fragText = IO::readText(shaderPath + ".frag");

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



		for (auto& node : meshWithSkin->nodes) {
			if (node->type == +GLTFNodeType::mesh || node->type == +GLTFNodeType::skin) {
				auto& mesh = meshWithSkin->meshes[node->meshIndex];

				if (context->meshPrimitives.find(node->meshIndex) != context->meshPrimitives.end()) continue;

				std::vector<GLTFGPUData> gpu;

				for (auto& prim : mesh->primitives) {

					GLTFGPUData primGPU;
					glGenVertexArrays(1, &primGPU.VAO);

					glBindVertexArray(primGPU.VAO);

					for (auto& attribute : prim.attributes.items()) {
						const auto& attribName = attribute.key();

						auto& accessor = meshWithSkin->accessors[attribute.value()];

						if (accessor.bufferView >= 0) {
							const auto& bufferView = meshWithSkin->bufferViews[accessor.bufferView];

							const auto& buffer = meshWithSkin->buffers[bufferView.bufferIndex];

							GLuint attribBuffer = 0;
							glGenBuffers(1, &attribBuffer);
							primGPU.VBOs.push_back(attribBuffer);
							glBindBuffer(GL_ARRAY_BUFFER, attribBuffer);

							glBufferData(GL_ARRAY_BUFFER, bufferView.byteLength, buffer.data() + bufferView.byteOffset, GL_STATIC_DRAW);

							if (attribName == "POSITION") {
								auto loc = glGetAttribLocation(context->shader->program, "position");
								if (loc >= 0) {
									glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, bufferView.byteStride, nullptr);
									glEnableVertexAttribArray(loc);
								}
							}
							else if (attribName == "NORMAL") {
								auto loc = glGetAttribLocation(context->shader->program, "normal");
								if (loc >= 0) {
									glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, bufferView.byteStride, nullptr);
									glEnableVertexAttribArray(loc);
								}

							}
							else if (attribName == "TEXCOORD_0") {
								auto loc = glGetAttribLocation(context->shader->program, "uv");
								if (loc >= 0) {
									glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, bufferView.byteStride, nullptr);
									glEnableVertexAttribArray(loc);
								}
							}
							// TODO: skinning attributes (joints and indices)
							else if (attribName == "JOINTS_0") {
								auto loc = glGetAttribLocation(context->shader->program, "indices");
								if (loc >= 0) {
									glVertexAttribIPointer(loc, 4, accessor.componentType, 0, nullptr);
									glEnableVertexAttribArray(loc);
								}
							}
							else if (attribName == "WEIGHTS_0") {
								auto loc = glGetAttribLocation(context->shader->program, "weights");
								if (loc >= 0) {
									glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, bufferView.byteStride, nullptr);
									glEnableVertexAttribArray(loc);
								}
							}
						}
					}

					if (prim.indices >= 0) {
						const auto& primIndicesAccessor = meshWithSkin->accessors[prim.indices];
						if (primIndicesAccessor.bufferView) {
							const auto& primIndicesBufferView = meshWithSkin->bufferViews[primIndicesAccessor.bufferView];
							const auto& primIndicesBuffer = meshWithSkin->buffers[primIndicesBufferView.bufferIndex];

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

		for (uint32_t i = 0; i < meshWithSkin->images.size(); i++) {
			auto& image = meshWithSkin->images[i];
			if (image.bufferView >= 0) {
				// Need to create a texture from this image using data from its bufferView
				const auto& imgBufferView = meshWithSkin->bufferViews[image.bufferView];
				// Get pointer to data
				const uint8_t* data = meshWithSkin->buffers[imgBufferView.bufferIndex].data() + imgBufferView.byteOffset;

				context->imageTextures[i] = std::make_shared<Texture>(data, imgBufferView.byteLength);
			}
		}

		meshWithSkin->context = context;
	}

	void updateNodeMatrix(s_ptr<GLTFNode> node) {
		if (node->updated) return;

		mat4 m = glm::translate(node->translation)
			* glm::toMat4(node->rotation)
			* glm::scale(node->scale);

		if (node->parent) {
			updateNodeMatrix(node->parent);
			m = node->parent->matrix * m;
		}

		node->matrix = m;
		node->updated = true;
	}
}

void Lab09::init() {
	assert(yourName != nullptr);

	ground = AnimationObject(AnimationObjectType::quad, vec3(0.f, -0.01f, 0.f), vec3(0.f, 0.f, 90.f), vec3(250), nullptr, vec4(vec3(0.5f), 1.f));
	ground.cullFace = true;

	joint.size = 0.2f;

	meshWithSkin = GLTFImporter::import(skinnedMeshPath);
	if (meshWithSkin == nullptr) {
		log("Unable to load {0}\n", skinnedMeshPath);
		exit(1);
	}

	//  TODO: parse skinning data
	initContext();

	for (auto& skin : meshWithSkin->skins) {
		auto& accessor = meshWithSkin->accessors[skin.inverseBindMatricesIndex];
		auto& bufferView = meshWithSkin->bufferViews[accessor.bufferView];
		auto& buffer = meshWithSkin->buffers[bufferView.bufferIndex];

		const mat4* values = (const mat4*)(buffer.data() + bufferView.byteOffset);

		if (accessor.count < skin.joints.size()) {
			log("Error! Not enough matrices for {0} joints!\n", skin.joints.size());
			exit(1);
		}

		for (auto& j : skin.joints) {
			skin.inverseBindMatrices[j] = *values;
			values++;
		}
	}

	jointColors.resize(100, vec4(1.f));
	for (auto& j : jointColors) {
		j = vec4(glm::linearRand(vec3(0.f), vec3(1.f)), 1.f);
	}

	initialized = true;
}

void Lab09::update() {
	if (!initialized) init();

	double nowish = getTime();

	double dt = nowish - lastFrameChange;

	if (isPlaying) {
		if (dt >= SPF) {
			int framesToAdvance = glm::round(dt / SPF);
			framesToAdvance = glm::clamp<int>(framesToAdvance, 1, 20);
			fixedSecondsElapsed += SPF * framesToAdvance;
			currentFrame += framesToAdvance;

			if (currentFrame > numFrames) {
				currentFrame = 0;
			}

			lastFrameChange = nowish;
		}
	}


	float currentTime = (float)(currentFrame * SPF);

	// Update GLTF animations
	for (auto& anim : meshWithSkin->animations) {
		for (auto& channel : anim.channels) {
			uint32_t nodeIndex = channel.target.node;
			s_ptr<GLTFNode> node = meshWithSkin->nodes[nodeIndex];

			const auto& sampler = anim.samplers[channel.sampler];

			const auto& keyframeTimeAccessor = meshWithSkin->accessors[sampler.input];
			int numKeyframes = keyframeTimeAccessor.count;
			const auto& keyframeTimeBufferView = meshWithSkin->bufferViews[keyframeTimeAccessor.bufferView];
			const auto& keyframeTimeBuffer = meshWithSkin->buffers[keyframeTimeBufferView.bufferIndex];

			const float* keyframeTimes = (const float*)
				(keyframeTimeBuffer.data() + keyframeTimeBufferView.byteOffset);

			int beforeIndex = 0, afterIndex = 1;
			for (; afterIndex < numKeyframes; afterIndex++) {
				if (currentTime < keyframeTimes[afterIndex]) break;
			}
			beforeIndex = afterIndex - 1;

			float bt = keyframeTimes[beforeIndex];
			float at = keyframeTimes[afterIndex];

			float localT = (currentTime - bt) / (at - bt);
			localT = glm::clamp<float>(localT, 0.0f, 1.0f);

			const auto& keyframeValueAccessor = meshWithSkin->accessors[sampler.output];
			const auto& keyframeValueBufferView = meshWithSkin->bufferViews[keyframeValueAccessor.bufferView];
			const auto& keyframeValueBuffer = meshWithSkin->buffers[keyframeValueBufferView.bufferIndex];
			const float* keyframeValues = (const float*)
				(keyframeValueBuffer.data() + keyframeValueBufferView.byteOffset);

			switch (channel.target.path) {
			case GLTFAnimationTargetPath::translation:
			{
				if (sampler.interpolation == +GLTFAnimationSamplerInterpolation::LINEAR) {
					int bo = beforeIndex * 3;
					int ao = afterIndex * 3;
					vec3 bv, av;
					bv.x = keyframeValues[bo];
					bv.y = keyframeValues[bo + 1];
					bv.z = keyframeValues[bo + 2];

					av.x = keyframeValues[ao];
					av.y = keyframeValues[ao + 1];
					av.z = keyframeValues[ao + 2];

					vec3 interpolated = (av * localT) + (bv * (1.0f - localT));

					node->translation = interpolated;
				}
				else if (sampler.interpolation == +GLTFAnimationSamplerInterpolation::CUBICSPLINE) {
					float deltaTime = at - bt;

					int bo = beforeIndex * 9;
					vec3 beforeValue, beforeOutputTangent;
					//glm::from_pointer(keyframeValues + bo, beforeInputTangent);
					glm::from_pointer(keyframeValues + bo + 3, beforeValue);
					glm::from_pointer(keyframeValues + bo + 6, beforeOutputTangent);
					beforeOutputTangent *= deltaTime;

					int ao = afterIndex * 9;
					vec3 afterInputTangent, afterValue;
					glm::from_pointer(keyframeValues + ao, afterInputTangent);
					glm::from_pointer(keyframeValues + ao + 3, afterValue);
					afterInputTangent *= deltaTime;
					//glm::from_pointer(keyframeValues + ao + 6, afterOutputTangent);

					float t = localT;
					float t2 = t * t;
					float t3 = t * t * t;

					vec3 interpolated =
						(2 * t3 - 3 * t2 + 1) * beforeValue
						+ (t3 - 2 * t2 + t) * beforeOutputTangent
						+ (-2 * t3 + 3 * t2) * afterValue
						+ (t3 - t2) * afterInputTangent;

					node->translation = interpolated;

				}

				break;
			}
			case GLTFAnimationTargetPath::rotation:
			{
				if (sampler.interpolation == +GLTFAnimationSamplerInterpolation::LINEAR) {
					int bo = beforeIndex * 4;
					int ao = afterIndex * 4;
					quaternion bv, av;
					bv.x = keyframeValues[bo];
					bv.y = keyframeValues[bo + 1];
					bv.z = keyframeValues[bo + 2];
					bv.w = keyframeValues[bo + 3];

					av.x = keyframeValues[ao];
					av.y = keyframeValues[ao + 1];
					av.z = keyframeValues[ao + 2];
					av.w = keyframeValues[ao + 3];

					quaternion interpolated = glm::slerp(bv, av, localT);

					node->rotation = interpolated;
				}
				else if (sampler.interpolation == +GLTFAnimationSamplerInterpolation::CUBICSPLINE) {
					float deltaTime = at - bt;

					int bo = beforeIndex * 12;
					vec3 beforeValue, beforeOutputTangent;
					//glm::from_pointer(keyframeValues + bo, beforeInputTangent);
					glm::from_pointer(keyframeValues + bo + 4, beforeValue);
					glm::from_pointer(keyframeValues + bo + 8, beforeOutputTangent);
					beforeOutputTangent *= deltaTime;

					int ao = afterIndex * 12;
					vec3 afterInputTangent, afterValue;
					glm::from_pointer(keyframeValues + ao, afterInputTangent);
					glm::from_pointer(keyframeValues + ao + 4, afterValue);
					afterInputTangent *= deltaTime;
					//glm::from_pointer(keyframeValues + ao + 8, afterOutputTangent);

					float t = localT;
					float t2 = t * t;
					float t3 = t * t * t;

					quaternion interpolated =
						(2 * t3 - 3 * t2 + 1) * beforeValue
						+ (t3 - 2 * t2 + t) * beforeOutputTangent
						+ (-2 * t3 + 3 * t2) * afterValue
						+ (t3 - t2) * afterInputTangent;

					node->rotation = interpolated;

				}


				break;
			}
			case GLTFAnimationTargetPath::scale:
			{
				if (sampler.interpolation == +GLTFAnimationSamplerInterpolation::LINEAR) {
					int bo = beforeIndex * 3;
					int ao = afterIndex * 3;
					vec3 bv, av;
					bv.x = keyframeValues[bo];
					bv.y = keyframeValues[bo + 1];
					bv.z = keyframeValues[bo + 2];

					av.x = keyframeValues[ao];
					av.y = keyframeValues[ao + 1];
					av.z = keyframeValues[ao + 2];

					vec3 interpolated = (av * localT) + (bv * (1.0f - localT));

					node->scale = interpolated;
				}
				else if (sampler.interpolation == +GLTFAnimationSamplerInterpolation::CUBICSPLINE) {
					float deltaTime = at - bt;

					int bo = beforeIndex * 9;
					vec3 beforeValue, beforeOutputTangent;
					//glm::from_pointer(keyframeValues + bo, beforeInputTangent);
					glm::from_pointer(keyframeValues + bo + 3, beforeValue);
					glm::from_pointer(keyframeValues + bo + 6, beforeOutputTangent);
					beforeOutputTangent *= deltaTime;

					int ao = afterIndex * 9;
					vec3 afterInputTangent, afterValue;
					glm::from_pointer(keyframeValues + ao, afterInputTangent);
					glm::from_pointer(keyframeValues + ao + 3, afterValue);
					afterInputTangent *= deltaTime;
					//glm::from_pointer(keyframeValues + ao + 6, afterOutputTangent);

					float t = localT;
					float t2 = t * t;
					float t3 = t * t * t;

					vec3 interpolated =
						(2 * t3 - 3 * t2 + 1) * beforeValue
						+ (t3 - 2 * t2 + t) * beforeOutputTangent
						+ (-2 * t3 + 3 * t2) * afterValue
						+ (t3 - t2) * afterInputTangent;

					node->scale = interpolated;

				}
				break;
			}
			}
		}
	}

	// TODO: update node matrices
	for (auto& node : meshWithSkin->nodes) {
		node->updated = false;
	}

	for (auto& node : meshWithSkin->nodes) {
		updateNodeMatrix(node);
	}
}

void Lab09::render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow) {

	if (meshWithSkin) {
		if (isShadow) return;

		meshWithSkin->context->shader->start();

		for (auto& node : meshWithSkin->nodes) {
			if (node->type == +GLTFNodeType::skin) {
				auto& mesh = meshWithSkin->meshes[node->meshIndex];

				if (meshWithSkin->context->meshPrimitives.find(node->meshIndex) == meshWithSkin->context->meshPrimitives.end()) continue;

				const auto& gpu = meshWithSkin->context->meshPrimitives[node->meshIndex];


				auto& skin = meshWithSkin->skins[node->skinIndex];

				auto invNode = glm::inverse(node->matrix);

				GPU::Lighting::get().bind(meshWithSkin->context->shader);

				auto joLoc = glGetUniformLocation(meshWithSkin->context->shader->program, "joints");
				if (joLoc >= 0) {

					int i = 0; 
					for (auto j : skin.joints) {
						auto& jNode = meshWithSkin->nodes[j];
						skin.jointMatrices[i] = //invNode * 
							jNode->matrix * skin.inverseBindMatrices[j];
						i++;
					}

					glUniformMatrix4fv(joLoc, skin.jointMatrices.size(), GL_FALSE, (const GLfloat*)skin.jointMatrices.data());
				}

				auto jcLoc = glGetUniformLocation(meshWithSkin->context->shader->program, "jointColors");
				if (jcLoc >= 0) {
					glUniform4fv(jcLoc, jointColors.size(), (const GLfloat*)jointColors.data());
				}

				auto vpLoc = glGetUniformLocation(meshWithSkin->context->shader->program, "viewproj");
				if (vpLoc >= 0) {
					auto viewproj = projection * view;
					glUniformMatrix4fv(vpLoc, 1, GL_FALSE, glm::value_ptr(viewproj));
				}

				for (uint32_t i = 0; i < mesh->primitives.size(); i++) {
					auto& prim = mesh->primitives[i];
					const auto& primGPU = gpu[i];

					static GLTFMaterial defaultMaterial;
					const GLTFMaterial* primMaterial = &defaultMaterial;
					if (prim.material >= 0) {
						primMaterial = &meshWithSkin->materials[prim.material];

						if (primMaterial->doubleSided) {
							glDisable(GL_CULL_FACE);
						}
						else {
							glEnable(GL_CULL_FACE);
						}

						auto bcfLoc = glGetUniformLocation(meshWithSkin->context->shader->program, "baseColorFactor");
						glUniform4fv(bcfLoc, 1, glm::value_ptr(primMaterial->pbr.baseColorFactor));

						auto acLoc = glGetUniformLocation(meshWithSkin->context->shader->program, "alphaCutoff");
						glUniform1f(acLoc, primMaterial->alphaCutoff);


						glActiveTexture(GL_TEXTURE0);
						GLint itLoc = glGetUniformLocation(meshWithSkin->context->shader->program, "inputTexture");
						glUniform1i(itLoc, 0);

						GLint utLoc = glGetUniformLocation(meshWithSkin->context->shader->program, "useTexture");
						if (primMaterial->pbr.baseColorTexture.index >= 0) {

							const GLTFTexture& tex = meshWithSkin->textures[primMaterial->pbr.baseColorTexture.index];

							if (tex.source >= 0) {
								GLuint texture = meshWithSkin->context->imageTextures[tex.source]->id;
								glUniform1i(utLoc, 1);
								glBindTexture(GL_TEXTURE_2D, texture);
								// Set sampler
								if (tex.sampler >= 0) {
									const GLTFSampler& sampler = meshWithSkin->samplers[tex.sampler];
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


					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, 0);

					//glActiveTexture(GL_TEXTURE1);
					//glBindTexture(GL_TEXTURE_2D, 0);
					//GLint stLoc = glGetUniformLocation(meshWithSkin->context->shader->program, "shadowTexture");
					//glUniform1i(stLoc, 1);

					glBindVertexArray(primGPU.VAO);

					// If we have an index buffer, draw this primitive with glDrawElements
					if (prim.indices >= 0) {
						const auto& primIndicesAccessor = meshWithSkin->accessors[prim.indices];
						if (primIndicesAccessor.bufferView) {
							const auto& primIndicesBufferView = meshWithSkin->bufferViews[primIndicesAccessor.bufferView];
							const auto& primIndicesBuffer = meshWithSkin->buffers[primIndicesBufferView.bufferIndex];
							glDrawElements(GL_TRIANGLES, primIndicesAccessor.count, primIndicesAccessor.componentType._to_integral(), 0);
						}
					}
					// Without an index buffer, we assume we're drawing triangles using sets of 3 vertices
					// at a time from the established buffer.
					else {
						// How many triangles to draw? This probably comes from the primitive attribute data, namely position.
						if (prim.attributes.contains("POSITION")) {
							const auto& primPositionAccessor = meshWithSkin->accessors[prim.attributes["POSITION"].get<uint32_t>()];
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

		meshWithSkin->context->shader->stop();
	}

	auto& jr = AnimationObjectRenderer::get();
	// Render ground
	jr.beginBatchRender(ground.shapeType, false, vec4(1.f), isShadow);

	if (!isShadow) {
		GPU::Lighting::get().bind(jr.currentMesh->shader);
		if (ground.cullFace)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
		}
		else {
			glDisable(GL_CULL_FACE);
		}
	}

	if (isShadow) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}

	ground.updateMatrix(true);
	jr.renderBatchWithOwnColor(ground, isShadow);
	jr.endBatchRender(isShadow);

	// TODO: render joints

	glClear(GL_DEPTH_BUFFER_BIT);
	jr.beginBatchRender(AnimationObjectType::sphere);

	for (auto& node : meshWithSkin->nodes) {
		joint.transform = node->matrix;

		jr.renderBatchWithOwnColor(joint);
	}

	jr.endBatchRender();
}

// Renders the UI controls for the lab.
void Lab09::renderUI() {

	{
		ImGui::Begin("Playbar");
		// Frame control UI
		//// Beginning
		if (ImGui::Button("<<")) {
			currentFrame = 0;
		}
		ImGui::SameLine();
		// Previous frame
		if (ImGui::Button("<")) {
			currentFrame--;
			if (currentFrame < 0) currentFrame = 0;
		}
		ImGui::SameLine();
		//// Play/stop button
		if (isPlaying) {
			if (ImGui::Button("Stop")) {
				isPlaying = false;
			}
		}
		else {
			if (ImGui::Button("Play")) {
				isPlaying = true;
				lastFrameChange = getTime();
			}
		}
		ImGui::SameLine();
		//// Next frame
		if (ImGui::Button(">")) {
			currentFrame++;
			if (currentFrame > numFrames) currentFrame = numFrames;
		}
		ImGui::SameLine();
		//// End
		if (ImGui::Button(">>")) {
			currentFrame = numFrames;
			isPlaying = false;
		}

		ImGui::InputInt("Number of frames", &numFrames);

		// Slider for playback
		ImGui::SliderInt("Frame position", &currentFrame, 0, numFrames);

		int fps = FPS;
		if (ImGui::InputInt("FPS", &fps)) {
			if (fps > 0) {
				FPS = fps;
				SPF = 1.0 / (double)FPS;
			}
		}
		ImGui::SameLine();

		ImGui::Text("SPF: %.2f", SPF);

		ImGui::End();
	}

	if (meshWithSkin) {
		if (ImGui::CollapsingHeader("skinnedMesh")) {
			IMDENT;

			GLTFRenderer::get().renderUI(meshWithSkin);
			IMDONT;
		}
	}

	if (ImGui::CollapsingHeader("Joint")) {
		joint.renderUI();
	}

	if (ImGui::CollapsingHeader("Ground")) {
		ground.renderUI();
	}

	if (ImGui::Button("Refresh shader")) {
		Application::get().addCommand([&]() {
			initContext();
		});
	}
}

ptr_vector<AnimationObject> Lab09::getObjects() {
	ptr_vector<AnimationObject> object_ptrs = { &ground };
	return object_ptrs;
}