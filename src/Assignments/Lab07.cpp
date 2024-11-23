// CMPS 4480 Lab 07
// Name: 

#include "Lab07.h"
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


// Variables for Lab 07
namespace cmps_4480_lab_07 {
	// TODO: change this to not be a nullptr
	const char* yourName = nullptr;


	// Variables for time and frame progression
	int numFrames = 240;
	int currentFrame = 0;
	int FPS = 24;					// frames per second
	double SPF = 1.0 / 24;			// seconds per frame (1/FPS)
	double lastFrameChange = 0;		// timestamp of the last frame change
	double fixedSecondsElapsed = 0;

	bool isPlaying = false;

	/*AnimationObject sky;
	s_ptr<Texture> skyTexture;
	std::string skyTextureFilename = IO::getAssetRoot() + "/" + "sky.png";*/
	std::string skyGLTFPath = IO::getAssetRoot() + "/" + "sky.gltf";

	s_ptr<GLTFData> sky;

	AnimationObject sun;
	AnimationObject ground;

	// Driven key: this one float should control 1) the position of the sun and 2) the color of the sky
	// 0 indicates high noon. 1 indicates midnight
	float timeOfDay = 0.0f;

	bool autoChangeTimeOfDay = false;
}

using namespace cmps_4480_lab_07;

void Lab07::init() {
	assert(yourName != nullptr);

	sun = AnimationObject(AnimationObjectType::sphere, vec3(0.f, 90.0f, 0.f), vec3(0.f), vec3(10.f), nullptr, vec4(1.f, 1.f, 0.f, 1.f), -1);
	sun.useLighting = false;
	ground = AnimationObject(AnimationObjectType::quad, vec3(0.f, -6.f, 0.f), vec3(0.f, 0.f, 90.f), vec3(250), nullptr, vec4(vec3(0.5f), 1.f));
	ground.cullFace = true;

	// TODO 1: create a sky.gltf file
	if (sky = GLTFImporter::import(skyGLTFPath)) {
		for (auto& mat : sky->materials) mat.useLighting = false;
	}
	else {
		log("Unable to load {0}\n", skyGLTFPath);
		exit(1);
	}

	auto context = std::make_shared<GLTFRenderContext>();

	// Init shader
	std::string vertText, fragText;

// TODO 3: change the shader code below to offset UV coordinates by timeOfDay
	{
		vertText = R"VERTEXSHADER(
__VERSION__

uniform mat4 mvp;
uniform mat4 model;
uniform float timeOfDay = 0.;

// Static attributes
in vec3 position;
in vec2 uv;
in vec3 normal;

out vec4 v_position;
out vec2 v_uv;

void main()
{
	vec3 p = position;

	v_position = model * vec4(p, 1.0);
	gl_Position = mvp * vec4(p, 1.0);
	v_uv = uv;
}
)VERTEXSHADER";


		fragText = R"FRAGMENTSHADER(
__VERSION__

// Input attributes from vertex shader
in vec4 v_position;
in vec2 v_uv;

// Renderer expects two outputs of fragment shader: color and primitive data
out vec4 frag_color;
out vec4 frag_prim;

// Global variables shared by all fragments
uniform sampler2D inputTexture;
uniform vec4 baseColorFactor = vec4(1.0);
uniform float alphaCutoff;
uniform float timeOfDay = 0.;

void main()
{
	vec4 result = baseColorFactor;
	vec2 uv = v_uv;

	vec4 texResult = texture(inputTexture, uv);
	if (texResult.a < alphaCutoff) discard;
	result *= texResult;

	frag_color = result;

	frag_prim = vec4(0.0, 0.0, gl_PrimitiveID, 1.0);
}

)FRAGMENTSHADER";
	}

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

	for (auto& node : sky->nodes) {
		if (node->type == +GLTFNodeType::mesh) {
			auto& mesh = sky->meshes[node->meshIndex];

			if (context->meshPrimitives.find(node->meshIndex) != context->meshPrimitives.end()) continue;

			std::vector<GLTFGPUData> gpu;

			for (auto& prim : mesh->primitives) {

				GLTFGPUData primGPU;
				glGenVertexArrays(1, &primGPU.VAO);

				glBindVertexArray(primGPU.VAO);

				for (auto& attribute : prim.attributes.items()) {
					const auto& attribName = attribute.key();

					auto& accessor = sky->accessors[attribute.value()];

					if (accessor.bufferView >= 0) {
						const auto& bufferView = sky->bufferViews[accessor.bufferView];

						const auto& buffer = sky->buffers[bufferView.bufferIndex];

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
					}
				}

				if (prim.indices >= 0) {
					const auto& primIndicesAccessor = sky->accessors[prim.indices];
					if (primIndicesAccessor.bufferView) {
						const auto& primIndicesBufferView = sky->bufferViews[primIndicesAccessor.bufferView];
						const auto& primIndicesBuffer = sky->buffers[primIndicesBufferView.bufferIndex];

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

	for (uint32_t i = 0; i < sky->images.size(); i++) {
		auto& image = sky->images[i];
		if (image.bufferView >= 0) {
			// Need to create a texture from this image using data from its bufferView
			const auto& imgBufferView = sky->bufferViews[image.bufferView];
			// Get pointer to data
			const uint8_t* data = sky->buffers[imgBufferView.bufferIndex].data() + imgBufferView.byteOffset;

			context->imageTextures[i] = std::make_shared<Texture>(data, imgBufferView.byteLength);
		}
	}

	sky->context = context;
	

	initialized = true;
}

void Lab07::update() {
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

	if (autoChangeTimeOfDay) {
		// TODO 4: change timeOfDay so it smoothly progresses from 0 to 1.
		// You can do this with keyframes, InterpCommands borrowed from
		// previous labs, functions, etc.
	}

	// TODO 2: change the sun position with respect to the timeOfDay variable



	auto gltf = Application::get().gltf;

	if (gltf == nullptr) return;

	float currentTime = (float)(currentFrame * SPF);

	// Update GLTF animations
	for (auto& anim : gltf->animations) {
		for (auto& channel : anim.channels) {
			uint32_t nodeIndex = channel.target.node;
			s_ptr<GLTFNode> node = gltf->nodes[nodeIndex];

			const auto& sampler = anim.samplers[channel.sampler];

			const auto& keyframeTimeAccessor = gltf->accessors[sampler.input];
			int numKeyframes = keyframeTimeAccessor.count;
			const auto& keyframeTimeBufferView = gltf->bufferViews[keyframeTimeAccessor.bufferView];
			const auto& keyframeTimeBuffer = gltf->buffers[keyframeTimeBufferView.bufferIndex];

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

			const auto& keyframeValueAccessor = gltf->accessors[sampler.output];
			const auto& keyframeValueBufferView = gltf->bufferViews[keyframeValueAccessor.bufferView];
			const auto& keyframeValueBuffer = gltf->buffers[keyframeValueBufferView.bufferIndex];
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
					int bo = beforeIndex * 3;
					int ao = afterIndex * 3;
					quaternion bv, av;
					bv.x = keyframeValues[bo];
					bv.y = keyframeValues[bo + 1];
					bv.z = keyframeValues[bo + 2];
					bv.w = keyframeValues[bo + 2];

					av.x = keyframeValues[ao];
					av.y = keyframeValues[ao + 1];
					av.z = keyframeValues[ao + 2];
					av.w = keyframeValues[ao + 3];

					quaternion interpolated = glm::slerp(bv, av, localT);

					node->rotation = interpolated;
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
}

void Lab07::render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow) {

	if (sky) {
		if (isShadow) return;

		sky->context->shader->start();

		for (auto& node : sky->nodes) {
			if (node->type == +GLTFNodeType::mesh) {
				auto& mesh = sky->meshes[node->meshIndex];

				if (sky->context->meshPrimitives.find(node->meshIndex) == sky->context->meshPrimitives.end()) continue;

				const auto& gpu = sky->context->meshPrimitives[node->meshIndex];

				node->matrix = glm::translate(node->translation) * glm::toMat4(node->rotation) * glm::scale(node->scale);

				if (node->parent) {
					node->matrix = node->parent->matrix * node->matrix;
				}

				mat4 mvp = projection * view * node->matrix;

				auto mvpLoc = glGetUniformLocation(sky->context->shader->program, "mvp");
				glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
				auto modelLoc = glGetUniformLocation(sky->context->shader->program, "model");
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(node->matrix));

				for (uint32_t i = 0; i < mesh->primitives.size(); i++) {
					auto& prim = mesh->primitives[i];
					const auto& primGPU = gpu[i];

					static GLTFMaterial defaultMaterial;
					const GLTFMaterial* primMaterial = &defaultMaterial;
					if (prim.material >= 0) {
						primMaterial = &sky->materials[prim.material];

						if (primMaterial->doubleSided) {
							glDisable(GL_CULL_FACE);
						}
						else {
							glEnable(GL_CULL_FACE);
						}

						auto itfLoc = glGetUniformLocation(sky->context->shader->program, "timeOfDay");
						glUniform1f(itfLoc, timeOfDay);

						auto bcfLoc = glGetUniformLocation(sky->context->shader->program, "baseColorFactor");
						glUniform4fv(bcfLoc, 1, glm::value_ptr(primMaterial->pbr.baseColorFactor));

						auto acLoc = glGetUniformLocation(sky->context->shader->program, "alphaCutoff");
						glUniform1f(acLoc, primMaterial->alphaCutoff);


						glActiveTexture(GL_TEXTURE0);
						GLint itLoc = glGetUniformLocation(sky->context->shader->program, "inputTexture");
						glUniform1i(itLoc, 0);

						GLint utLoc = glGetUniformLocation(sky->context->shader->program, "useTexture");
						if (primMaterial->pbr.baseColorTexture.index >= 0) {

							const GLTFTexture& tex = sky->textures[primMaterial->pbr.baseColorTexture.index];

							if (tex.source >= 0) {
								GLuint texture = sky->context->imageTextures[tex.source]->id;
								glUniform1i(utLoc, 1);
								glBindTexture(GL_TEXTURE_2D, texture);
								// Set sampler
								if (tex.sampler >= 0) {
									const GLTFSampler& sampler = sky->samplers[tex.sampler];
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
					//GLint stLoc = glGetUniformLocation(gltf->context->shader->program, "shadowTexture");
					//glUniform1i(stLoc, 1);

					glBindVertexArray(primGPU.VAO);

					// If we have an index buffer, draw this primitive with glDrawElements
					if (prim.indices >= 0) {
						const auto& primIndicesAccessor = sky->accessors[prim.indices];
						if (primIndicesAccessor.bufferView) {
							const auto& primIndicesBufferView = sky->bufferViews[primIndicesAccessor.bufferView];
							const auto& primIndicesBuffer = sky->buffers[primIndicesBufferView.bufferIndex];
							glDrawElements(GL_TRIANGLES, primIndicesAccessor.count, primIndicesAccessor.componentType._to_integral(), 0);
						}
					}
					// Without an index buffer, we assume we're drawing triangles using sets of 3 vertices
					// at a time from the established buffer.
					else {
						// How many triangles to draw? This probably comes from the primitive attribute data, namely position.
						if (prim.attributes.contains("POSITION")) {
							const auto& primPositionAccessor = sky->accessors[prim.attributes["POSITION"].get<uint32_t>()];
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

		sky->context->shader->stop();
	}
	
	auto& jr = AnimationObjectRenderer::get();

	// Render sun
	jr.beginBatchRender(sun.shapeType, false, vec4(1.f), isShadow);

	if (!isShadow) {
		GPU::Lighting::get().bind(jr.currentMesh->shader);
	}

	if (isShadow) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}
	else {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}

	sun.updateMatrix(true);
	jr.renderBatchWithOwnColor(sun, isShadow);
	jr.endBatchRender(isShadow);

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
}

// Renders the UI controls for the lab.
void Lab07::renderUI() {

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
	
	ImGui::SliderFloat("Time of day", &timeOfDay, 0.0f, 1.0f);
	ImGui::Checkbox("Auto-update time of day", &autoChangeTimeOfDay);

	if (ImGui::CollapsingHeader("Sun")) {
		sun.renderUI();
	}
	if (ImGui::CollapsingHeader("Ground")) {
		ground.renderUI();
	}

	if (sky) {
		if (ImGui::CollapsingHeader("Sky")) {
			IMDENT;
			
			GLTFRenderer::get().renderUI(sky);
			IMDONT;
		}
	}
}

ptr_vector<AnimationObject> Lab07::getObjects() {
	ptr_vector<AnimationObject> object_ptrs = { &sun, &ground };
	return object_ptrs;
}