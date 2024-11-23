#pragma once

#include "globals.h"

MAKE_ENUM(GLSLUniformType, int,
	_bool, _int, _uint, _float, _double, // scalar types
	_bvec2, _bvec3, _bvec4,			// vector bools
	_ivec2, _ivec3, _ivec4,			// vector ints
	_uvec2, _uvec3, _uvec4,			// vector unsigneds
	_vec2, _vec3, _vec4,			// vector floats
	_dvec2, _dvec3, _dvec4,			// vector doubles
	_mat2, _mat3, _mat4,			// matrix floats (2x2, _3x3, _4x4),
	_sampler1D, _sampler2D, _sampler3D, _samplerCube,
	_sampler1DShadow, _sampler2DShadow, _samplerCubeShadow
);

struct GLSLUniformBase;

using UniformMap = std::map<std::string, s_ptr<GLSLUniformBase>>;

// Uniform data that needs to be tracked:
// variable name. How it's found in the shader by doing glGetUniformLocation during binding.
// data type. Affects glUniform* calls, imgui calls, and data storage. Specialized for each value.
// value. Dominated by data type
// Desires: have a list of uniforms be stored in one collection even with different types,
// automatically use the right calls for 
struct GLSLUniformBase
{
	GLSLUniformType type = GLSLUniformType::_float;
	// The location as reported by calling glGetUniformLocation
	int location = -1;
	std::string variableName = "";
	bool autoUpdated = false;
	// For samplers, where the binding unit is set for this variable
	unsigned int bindingUnit = 0;

	GLSLUniformBase(GLSLUniformType t, const std::string& varName, bool autoSet = false)
		: type(t), variableName(varName), autoUpdated(autoSet) {}

	virtual ~GLSLUniformBase() { }

	virtual void updateGLUniform() { }

	virtual void renderUI();

	static s_ptr<GLSLUniformBase> createUniform(const std::string& datatype, const std::string& varname, const std::string& defaultValue = "");

	static UniformMap processUniforms(const std::string& shaderText);
};

template<typename T>
struct GLSLUniform : GLSLUniformBase
{
	T value;

	GLSLUniform(GLSLUniformType t, const std::string& varName, bool autoSet = false, const T& initial = T())
		: GLSLUniformBase(t, varName, autoSet), value(initial)
	{}

	virtual ~GLSLUniform() { }

	virtual void updateGLUniform();

	virtual void renderUI();
};