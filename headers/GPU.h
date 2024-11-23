#pragma once

#include "globals.h"
#include "Primitives.h"
#include "Renderer.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

struct MeshVertex {
	vec3 position;
	vec3 color;
};

// The two mesh types need different vertex array and buffer objects, but they can
// use the same index buffer object. Your job will be to implement the cube indices
// inside this struct's init function.
struct IndexBuffer {
	std::vector<GLuint> indices;
	GLuint IBO = 0;

	// This is a helper function that can be called to add 3 indices
	// as a triangle in the cube mesh. 
	void addTriangle(GLuint v0, GLuint v1, GLuint v2) {
		indices.push_back(v0);
		indices.push_back(v1);
		indices.push_back(v2);
	}

	void init() {
		// Mesh vertices are generated as 
		// 0: -1, -1, -1		// left		bottom	back
		// 1: -1, -1, +1		// left		bottom	front
		// 2: -1, +1, -1		// left		top		back
		// 3: -1, +1, +1		// left		top		front
		// 4: +1, -1, -1		// right	bottom	back
		// 5: +1, -1, +1		// right	bottom	front
		// 6: +1, +1, -1		// right	top		back
		// 7: +1, +1, +1		// right	top		front

		// TODO: populate the indices 
		// 6 sides to a box, 2 triangles each: 12 triangles
		// 12 triangles, 3 indices each: 36 indices

		// Front triangles of box:
		addTriangle(1, 5, 7);
		addTriangle(7, 3, 1);

		// Right triangles of box
		addTriangle(5, 4, 6);
		addTriangle(6, 7, 5);

		// back triangles of box:
		addTriangle(4, 0, 2);
		addTriangle(2, 6, 4);

		// Left triangles of box:
		addTriangle(0, 1, 3);
		addTriangle(3, 2, 0);

		// Top triangles of box
		addTriangle(3, 7, 6);
		addTriangle(6, 2, 3);

		// Bottom triangles of box
		addTriangle(0, 4, 5);
		addTriangle(5, 1, 0);

		glGenBuffers(1, &IBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
	}
};

struct SimpleShader {
	GLuint vertexShader = 0;
	GLuint fragmentShader = 0;
	GLuint program = 0;

	~SimpleShader() {
		cleanup();
	}

	void cleanup() {
		if (program) {
			if (fragmentShader) {
				glDetachShader(program, fragmentShader);
				glDeleteShader(fragmentShader);
			}
			if (vertexShader) {
				glDetachShader(program, vertexShader);
				glDeleteShader(vertexShader);
			}

			glDeleteProgram(program);
		}
	}

	bool compileShader(const char* shaderSrc, GLenum shaderType, GLuint& shader) {
		shader = glCreateShader(shaderType);

		std::string shaderText = StringUtil::replaceAll(shaderSrc, "__VERSION__", Renderer::shaderVersion);

		const char* shaderTextPtr = shaderText.c_str();
		glShaderSource(shader, 1, &shaderTextPtr, nullptr);
		glCompileShader(shader);

		int  success = 0;
		static char infoLog[512];

		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

		if (!success) {
			glGetShaderInfoLog(shader, 512, nullptr, infoLog);
			log("{0} shader compilation failed:\n{1}\n", (shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment"), infoLog);
			return false;
		}

		return true;
	}

	bool linkProgram() {
		GLint success = 0;

		program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success) {
			char infoLog[512];
			glGetProgramInfoLog(program, 512, nullptr, infoLog);
			log("Shader linking failed:\n{0}\n", infoLog);
			return false;
		}

		return true;
	}

	bool init(const char * vertexShaderSrc, const char * fragmentShaderSrc) {

		if (!compileShader(vertexShaderSrc, GL_VERTEX_SHADER, vertexShader)) {
			if (vertexShader != 0) {
				glDeleteShader(vertexShader);
				vertexShader = 0;
			}
			return false;
		}

		if (!compileShader(fragmentShaderSrc, GL_FRAGMENT_SHADER, fragmentShader)) {
			if (fragmentShader != 0) {
				glDeleteShader(fragmentShader);
				fragmentShader = 0;
			}
			return false;
		}

		if (!linkProgram()) {
			if (program != 0) {
				glDeleteProgram(program);
				program = 0;
			}
			return false;
		}

		return true;
	}
};

struct Lighting {
	bool enabled = true;

	// Ambient terms
	float Ia = 0.05f;		// intensity
	vec3 Ka = vec3(1.f);	// color

	// Diffuse terms
	float Id = 1.0f;
	vec3 Kd = vec3(1.f);

	// Specular terms
	float shininess = 32.f;
	vec3 Ks = vec3(1.f);

	vec3 lightDirection = vec3(0, -1, 0);

	bool autoOrbit = false;
	bool autoHue = false;

	vec3 orbitAxis = vec3(0, 1, 0);

	bool point = false;
	vec3 position = vec3(0.f);
	float intensity = 10.f;
	float radius = 2.0f;

	float softness = 0.0f;

	void renderUI(bool useHeader = true) {

		bool showUI = true;
		if (useHeader) {
			showUI = ImGui::CollapsingHeader("Lighting");
		}
		if (showUI) {
			ImGui::InputFloat("Ambient intensity", &Ia);
			ImGui::ColorEdit3("Ambient color", glm::value_ptr(Ka));
			ImGui::InputFloat("Diffuse intensity", &Id);
			ImGui::ColorEdit3("Diffuse color", glm::value_ptr(Kd));
			ImGui::InputFloat("Shininess", &shininess);
			ImGui::ColorEdit3("Specular color", glm::value_ptr(Ks));
			ImGui::InputFloat3("Light direction", glm::value_ptr(lightDirection));
			ImGui::Checkbox("Point light", &point);
			if (point) {
				ImGui::InputFloat3("Position", glm::value_ptr(position));
				ImGui::InputFloat("Intensity", &intensity);
				ImGui::InputFloat("Radius", &radius);
			}
			ImGui::InputFloat("Softness", &softness);

			ImGui::Checkbox("Auto-orbit", &autoOrbit);
			ImGui::InputFloat3("Orbit on axis", glm::value_ptr(orbitAxis));

			ImGui::Checkbox("Auto-hue", &autoHue);
		}
	}
};