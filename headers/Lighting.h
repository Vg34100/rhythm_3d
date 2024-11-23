#pragma once

#include "globals.h"
#include "Buffer.h"

class OpenGLRenderer;
class Shader;

namespace GPU
{
#include "../shaders/datatypes.glsl"

	struct Lighting {

		static Lighting& get();

		SceneLight sceneLight;
		Light lights[GPU_LIGHT_MAX_COUNT];

		ProjectionType projectionType = ProjectionType::Orthographic;
		float fov = 60.0f;
		vec3 position = vec3(0.f, 4.f, 0.f);
		vec3 center = vec3(0.f);
		vec3 up = vec3(0.f, 0.f, 1.f);
		vec2 nearFar = vec2(0.001f, 1000.0f);
		mat4 view;
		mat4 proj;

		spBuffer gpu;

		bool init();
		void bind(s_ptr<Shader> shader);
		void update();

		void renderUI(OpenGLRenderer * ogl);
	};
}