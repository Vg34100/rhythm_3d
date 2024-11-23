#include "Lighting.h"

#include "Framebuffer.h"
#include "Renderer.h"
#include "Shader.h"
#include "UIHelpers.h"

#include "imgui.h"

using namespace GPU;

bool Lighting::init()
{
	// diffuse, specular, ambient
	sceneLight = { vec4(1.0), vec4(vec3(0.2), 1.0), vec4(vec3(0.1), 1.0) };
	//lights.resize(5);

	// First one is a spotlight
	lights[0] = {
		vec4(0.0,  4.0,  0.0, 1.0),
		vec4(0.0f, -1.0f, 0.0f, 0.0f),
		sceneLight.mainDiffuse,
		sceneLight.mainSpecular,
		vec4(0.0f, 1.0f, 0.0f, 0.f),
		vec4(60.f, 0.0f, 0.0f, 0.0f),

	};

	// Rest are directional
	lights[1] = {
		vec4(1.0, 1.0,  -1.0, 0.0),
		vec4(-1.0f, -1.0f, 1.0f, 0.0f),
		sceneLight.mainDiffuse,
		sceneLight.mainSpecular,
		vec4(0.0f, 1.0f, 0.0f, 0.f),
		vec4(80.0f, 10.0f, 0.0f, 0.0f),

	};

	lights[2] = {
		vec4(-1.0,  1.0,  1.0, 0.0),
		vec4(1.0f, -1.0f, -1.0f, 0.0f),
		sceneLight.mainDiffuse,
		sceneLight.mainSpecular,
		vec4(0.0f, 1.0f, 0.0f, 0.f),
		vec4(180.0f, 0.0f, 0.0f, 0.0f),

	};

	//lights[3] = {
	//	vec4(-1.0, 1.0,  -1.0, 0.0),
	//	vec4(0.0f, 0.0f, 0.0f, 0.0f),
	//	sceneLight.mainDiffuse,
	//	sceneLight.mainSpecular,
	//	vec4(0.0f, 1.0f, 0.0f, 0.f),
	//	vec4(80.0f, 10.0f, 0.0f, 0.0f),

	//};

	//lights[4] = {
	//	vec4(0.0,  -1.0,  0.0, 0.0),
	//	vec4(0.0f, 0.0f, 0.0f, 0.0f),
	//	sceneLight.mainDiffuse,
	//	sceneLight.mainSpecular,
	//	vec4(0.0f, 1.0f, 0.0f, 0.f),
	//	vec4(180.0f, 0.0f, 0.0f, 0.0f),

	//};

	//lights.resize(3);

	size_t dataSize = sizeof(SceneLight) + (sizeof(Light) * GPU_LIGHT_MAX_COUNT);

	gpu = spBuffer(new Buffer(GL_UNIFORM_BUFFER,
		dataSize, nullptr, GL_DYNAMIC_COPY));

	return true;
}

void Lighting::bind(s_ptr<Shader> shader)
{	
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bufferIndex, gpu->buffer);

	unsigned int lights_index = glGetUniformBlockIndex(shader->program, "LightData");
	glUniformBlockBinding(shader->program, lights_index, GPU_LIGHT_BINDING_SPOT);
	
	glBindBufferBase(GL_UNIFORM_BUFFER, GPU_LIGHT_BINDING_SPOT, gpu->buffer);
}

void Lighting::update()
{
	//size_t offset = 0;
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gpu->buffer);
	//glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(SceneLight), &sceneLight);
	//offset += sizeof(SceneLight);
	//glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(Light) * lights.size(), lights.data());


	glBindBuffer(GL_UNIFORM_BUFFER, gpu->buffer);
	glBufferSubData(gpu->bindTarget, 0, sizeof(SceneLight), (const void*)&sceneLight);
	glBufferSubData(gpu->bindTarget, sizeof(SceneLight), sizeof(Light) * GPU_LIGHT_MAX_COUNT, (const void*)lights);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

}

Lighting& Lighting::get()
{
	static Lighting instance;
	static bool initialized = ([&]() {
		return instance.init();
		})();

	return instance;
}

void Lighting::renderUI(OpenGLRenderer* ogl) {
	if (ImGui::CollapsingHeader("Lights"))
	{
		ImGui::Checkbox("Use shadow?", &ogl->useShadow);
		if (ogl->shadowMap) ogl->shadowMap->renderUI("Shadow map");
		ImGui::Text("Shadow light projection type");
		ImGui::SameLine();
		renderEnumButton<ProjectionType>(projectionType);
		ImGui::InputFloat("FOV", &fov);
		ImGui::InputFloat3("Shadow light position", glm::value_ptr(position));
		ImGui::SliderFloat3("Shadow light position s", glm::value_ptr(position), -12.0f, 12.0f, "", 1.0f);
		ImGui::InputFloat3("Shadow light center", glm::value_ptr(center));
		ImGui::SliderFloat3("Shadow light center s", glm::value_ptr(center), -12.0f, 12.0f, "", 1.0f);
		ImGui::InputFloat3("Shadow light up", glm::value_ptr(up));
		ImGui::SliderFloat3("Shadow light up s", glm::value_ptr(up), -1.0f, 1.0f, "", 1.0f);
		ImGui::InputFloat2("Shadow light nearfar", glm::value_ptr(nearFar));

		bool anyObjectChange = false;

		for (int i = 0; i < GPU_LIGHT_MAX_COUNT; i++)
		{
			if (ImGui::CollapsingHeader(fmt::format("Light {0}", i).c_str()))
			{
				auto& l = lights[i];

				ImGui::PushID(i);
				anyObjectChange |= ImGui::InputFloat4("Position", (float*)glm::value_ptr(l.position));
				anyObjectChange |= ImGui::InputFloat4("Direction", (float*)glm::value_ptr(l.direction));
				anyObjectChange |= ImGui::ColorEdit4("Diffuse", (float*)glm::value_ptr(l.diffuse));
				anyObjectChange |= ImGui::ColorEdit4("Specular", (float*)glm::value_ptr(l.specular));

				anyObjectChange |= ImGui::InputFloat("Const. Attenuation", &l.attenuation.x);
				anyObjectChange |= ImGui::InputFloat("Lin. Attenuation", &l.attenuation.y);
				anyObjectChange |= ImGui::InputFloat("Quad. Attenuation", &l.attenuation.z);
				anyObjectChange |= ImGui::InputFloat("Spot cutoff", &l.spot.x);
				anyObjectChange |= ImGui::InputFloat("Spot exponent", &l.spot.y);



				ImGui::PopID();
			}
		}
	}
}