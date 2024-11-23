// CMPS 4480 Lab 06
// Name: 

#include "Lab06.h"
#include "Application.h"
#include "AnimationObjectRenderer.h"
#include "GLTFImporter.h"
#include "InputOutput.h"
#include "Lighting.h"
#include "Prompts.h"
#include "UIHelpers.h"

#include <cassert>

#include <glm/gtc/random.hpp>

#include "implot.h"


// Variables for Lab 06
namespace cmps_4480_lab_06 {
	// TODO: change this to not be a nullptr
	const char* yourName = "Nick";


	// Variables for time and frame progression
	int numFrames = 240;
	int currentFrame = 0;
	int FPS = 24;					// frames per second
	double SPF = 1.0 / 24;			// seconds per frame (1/FPS)
	double lastFrameChange = 0;		// timestamp of the last frame change
	double fixedSecondsElapsed = 0;

	bool isPlaying = false;
}

using namespace cmps_4480_lab_06;

void Lab06::init() {
	assert(yourName != nullptr);

	initialized = true;
}

void Lab06::update() {
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

	auto gltf = Application::get().gltf;

	if (gltf == nullptr) return;

	float currentTime = (float)(currentFrame * SPF);

	// TODO: animate the gltf scene!
	// Iterate over animations
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
			for (; afterIndex < numKeyframes ; afterIndex++) {
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
				else if(sampler.interpolation == +GLTFAnimationSamplerInterpolation::CUBICSPLINE) {
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

// Renders the UI controls for the lab.
void Lab06::renderUI() {

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

ptr_vector<AnimationObject> Lab06::getObjects() {
	return {};
}