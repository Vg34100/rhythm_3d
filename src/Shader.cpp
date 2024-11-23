#include "Shader.h"
#include "InputOutput.h"

#include <algorithm>
#include <iostream>
#include <streambuf>

//#define PRINT_SHADER_DEBUG

std::string Shader::Version = "#version 130";
std::string Shader::VersionMacro = "__VERSION__";
std::string Shader::Path = IO::assetPath("shaders/");
//std::string Shader::Path = "./shaders";

bool Shader::GLshader::compile()
{
	if (text == "") return false;

	const char* shaderTextC = text.c_str();
	int shaderLength = (int)text.length();

	name = glCreateShader(type);

	glShaderSource(name, 1, &shaderTextC, &shaderLength);

	glCompileShader(name);

	int didCompile;
	glGetShaderiv(name, GL_COMPILE_STATUS, &didCompile);

	int logLength = 0;
	glGetShaderiv(name, GL_INFO_LOG_LENGTH, &logLength);
	std::vector<GLchar> shaderLog((size_t)logLength + 1);

	if (logLength > 0)
	{
		glGetShaderInfoLog(name, logLength, &logLength, shaderLog.data());
	}

	if (logLength > 0)
	{
#ifdef PRINT_SHADER_DEBUG
		// Print shader with line numbers
		auto lines = util::splitString(shaderText, '\n');

		int index = 0;
		for (auto& line : lines)
		{
			index++;
			log("{0}\t{1}\n", index, line);
		}
#endif

		compileLog = std::string(shaderLog.begin(), shaderLog.end());

		auto logLines = StringUtil::split(compileLog, '\n');
		auto shaderLines = StringUtil::split(text, '\n', false);

		for (auto& ll : logLines)
		{
			// Parse out the erroneous line number
			auto lpstart = ll.find_first_of('(');
			auto rpstart = ll.find_first_of(')');

			if (lpstart == std::string::npos || rpstart == std::string::npos) {
				log("{0}\n", ll);
				continue;
			}
			auto lparen = ll.begin() + lpstart + 1;
			auto rparen = ll.begin() + rpstart;

			if (lparen == ll.end() || rparen == ll.end())
			{
				continue;
			}

			auto sub = std::string(lparen, rparen);

			int lineNo = -1;

			try
			{
				lineNo = std::stoi(sub);
			}
			catch (std::invalid_argument& e)
			{
				log("Error: {0}\n", e.what());
			}

			if (lineNo > 0 && lineNo <= shaderLines.size())
			{
				auto sl = StringUtil::replaceAll(
					StringUtil::replaceAll(shaderLines[(size_t)lineNo - 1], "}", "}}", false),
					"{", "{{", false);
				log("{0}:\t{1}\n", lineNo, sl);
			}

			log("{0}\n", StringUtil::replaceAll(StringUtil::replaceAll(ll, "{", "{{", false),
				"}", "}}", false));
		}
	}

	if (didCompile == GL_FALSE)
	{
		return false;
	}

	return true;
}

Shader::Shader(const std::string& vertexText, const std::string& fragText,
	const std::string& geoText, const std::string& tcText, const std::string& teText, glsl_list _xfbParams)
	: xfbParams(_xfbParams)
{
	if (!vertexText.empty())
	{
		shaders[ShaderType::VertexShader] = GLshader(ShaderType::VertexShader, ProcessShaderText(vertexText));
		flags.hasVertexStage = true;
	}

	if (!fragText.empty())
	{
		shaders[ShaderType::FragmentShader] = GLshader(ShaderType::FragmentShader, ProcessShaderText(fragText));
		flags.hasFragStage = true;
	}

	if (!geoText.empty())
	{
		shaders[ShaderType::GeometryShader] = GLshader(ShaderType::GeometryShader, ProcessShaderText(geoText));
		flags.hasGeometryStage = true;
	}

	if (!tcText.empty())
	{
		shaders[ShaderType::TessControlShader] = GLshader(ShaderType::TessControlShader, ProcessShaderText(tcText));
		flags.hasTessellationStage = true;
	}

	if (!teText.empty())
	{
		shaders[ShaderType::TessEvalShader] = GLshader(ShaderType::TessEvalShader, ProcessShaderText(teText));
	}
}

Shader::Shader(const std::string& shaderName, glsl_list _xfbParams)
	: xfbParams(_xfbParams)
{
	size_t index;
	std::string path;
	std::string text;

	path = IO::assetPath("shaders/" + shaderName + ".vert");
	if (IO::pathExists(path)) {
		text = IO::readText(path);
		index = text.find_first_not_of(" \t\n\r");
		if (index != std::string::npos) {
			shaders[ShaderType::VertexShader] = GLshader(ShaderType::VertexShader, ProcessShaderText(text));
			flags.hasVertexStage = true;
		}
	}

	path = IO::assetPath("shaders/" + shaderName + ".frag");
	if (IO::pathExists(path)) {
		text = IO::readText(path);
		index = text.find_first_not_of(" \t\n\r");
		if (index != std::string::npos) {
			shaders[ShaderType::FragmentShader] = GLshader(ShaderType::FragmentShader, ProcessShaderText(text));
			flags.hasFragStage = true;
		}
	}

	path = IO::assetPath("shaders/" + shaderName + ".geom");
	if (IO::pathExists(path)) {
		text = IO::readText(path);
		index = text.find_first_not_of(" \t\n\r");
		if (index != std::string::npos) {
			shaders[ShaderType::GeometryShader] = GLshader(ShaderType::GeometryShader, ProcessShaderText(text));
			flags.hasGeometryStage = true;
		}
	}

	path = IO::assetPath("shaders/" + shaderName + ".tessCtrl");
	if (IO::pathExists(path)) {
		text = IO::readText(path);
		index = text.find_first_not_of(" \t\n\r");
		if (index != std::string::npos) {
			shaders[ShaderType::TessControlShader] = GLshader(ShaderType::TessControlShader, ProcessShaderText(text));
			flags.hasTessellationStage = true;
		}
	}

	path = IO::assetPath("shaders/" + shaderName + ".tessEval");
	if (IO::pathExists(path)) {
		text = IO::readText(path);
		index = text.find_first_not_of(" \t\n\r");
		if (index != std::string::npos) {
			shaders[ShaderType::TessEvalShader] = GLshader(ShaderType::TessEvalShader, ProcessShaderText(text));
		}
	}
}

Shader::Shader(const std::string& src, ShaderType sType)
{
	if (!src.empty())
	{
		shaders[sType] = GLshader(sType, ProcessShaderText(src));
	}
}

bool Shader::init(bool recompile, std::vector<ShaderFragOutputBindings> fragBindings)
{
	if (program)
	{
		// If we're recompiling, we don't want to erase the GLshaders, just their GL state.
		destroy(!recompile);
	}

	program = glCreateProgram();

	for (auto& shader : shaders)
	{
		if (shader.second.compile())
		{
			glAttachShader(program, shader.second.name);
		}
		else
		{
			log("Error: {0} {1} failed to compile\n", shader.first._to_string(), shader.second.name);
			return false;
		}
	}

	if (!xfbParams.empty())
	{
		const GLchar** feedbackParams = new const GLchar * [xfbParams.size()];

		for (size_t i = 0; i < xfbParams.size(); i++)
		{
			feedbackParams[i] = xfbParams[i].second;
		}

		glTransformFeedbackVaryings(program, (GLsizei)xfbParams.size(), feedbackParams, GL_INTERLEAVED_ATTRIBS);
	}

	// Call bindings before linking
	for (auto& fb : fragBindings) {
		glBindFragDataLocation(program, fb.colorNumber, fb.name);
	}

	// Make sure we can link!
	if (!link())
	{
		log("Unable to link shader!\n");
		return false;
	}

	return true;
}

void Shader::bind(const Binding& _binding, bool purgeBad)
{
	binding = _binding;

	string_vector badUniforms, badAttributes;

	start();
	for (auto& u : binding.uniforms)
	{
		if (!u.second.bind(program))
		{
			log("\tUnable to bind uniform: {0}\n", u.first);
			badUniforms.push_back(u.first);
		}
	}
	stop();

	if (purgeBad)
	{
		for (auto& bu : badUniforms)
		{
			binding.uniforms.erase(bu);
		}
	}

	for (auto& v : binding.vertexAttributes)
	{
		if (!v.second.bind(program))
		{
			log("\tUnable to bind attribute: {0}\n", v.first);
			badAttributes.push_back(v.first);
		}
	}

	if (purgeBad)
	{
		for (auto& ba : badAttributes)
		{
			binding.vertexAttributes.erase(ba);
		}
	}
}

bool Shader::link()
{
	// Make sure we can link!
	glLinkProgram(program);
	GLint link_ok = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &link_ok);

	if (!link_ok)
	{
		int logLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		std::vector<GLchar> shaderLog((size_t)logLength + 1);
		glGetProgramInfoLog(program, logLength, &logLength, shaderLog.data());

		linkLog = std::string(shaderLog.begin(), shaderLog.end());

		log("Shader did not link. Log:\n{0}\n", linkLog);

		return false;
	}

	return true;
}

void Shader::destroy(bool clearGLshaders)
{
	stop();

	for (auto& s : shaders)
	{
		auto shaderName = s.second.name;
		if (shaderName != 0)
		{
			glDetachShader(program, shaderName);
			glDeleteShader(shaderName);
		}
	}

	if (clearGLshaders)
	{
		shaders.clear();
	}

	binding.clear();
	linkLog.clear();

	glDeleteProgram(program);
	program = 0;
}

std::string Shader::LoadText(const std::string& src)
{
	auto srcFwd = StringUtil::replaceAll(src, "\\", "/");
	//srcFwd = StringUtil::replaceAll(src, "/./", "/");
	srcFwd = StringUtil::replaceAll(srcFwd, Shader::Path, "");

	log("Reading text from file {0}\n", Shader::Path + srcFwd);
	return IO::readText(Shader::Path + srcFwd);
}

bool Shader::WriteText(const std::string& path, const std::string& txt)
{
	return IO::writeText(Shader::Path + path, txt);
}
