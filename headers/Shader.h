#pragma	once

#include "globals.h"

#include "StringUtil.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>


BETTER_ENUM(ShaderType, GLenum, VertexShader = GL_VERTEX_SHADER,
	FragmentShader = GL_FRAGMENT_SHADER,
	GeometryShader = GL_GEOMETRY_SHADER,
	TessControlShader = GL_TESS_CONTROL_SHADER,
	TessEvalShader = GL_TESS_EVALUATION_SHADER,
	Compute = GL_COMPUTE_SHADER)


struct ShaderFragOutputBindings {
	GLuint colorNumber;
	const char* name;
};
	
class Shader
{
public:
	static std::string Version;
	static std::string VersionMacro;
	static std::string Path;
	static std::string SkinningPath;

	// An individual OpenGL shader stage. Shader class is responsible for destroying resources
	struct GLshader
	{
		// The shader type
		ShaderType type = ShaderType::VertexShader;
		// Shader text
		std::string text;
		// OpenGL name
		GLuint name = 0;
		// Compile log
		std::string compileLog;

		GLshader() { }

		GLshader(ShaderType _type, const std::string& _text)
			: type(_type), text(_text) { }

		bool compile();
	};

	// Parameter is the uniform/attribute location for the shader program.
	// This is useful when providing your own functions/lambdas for updating
	// the uniform. But it's not so useful if you want to do reflection-like
	// processing and management of uniforms defined in shader source code.
	using UniformFunc = std::function<void(GLint)>;

	// Specifies a uniform or vertex attribute shader variable
	class Param
	{
	private:
	public:
		// Parameter is location
		using Func = std::function<void(GLint)>;
		GLint location = -1;
		std::string name;
		Func update;
		enum Type
		{
			Uniform,
			VertexAttribute
		};
		Type type = Type::Uniform;

		Param() { }
		Param(const std::string& _name, Func _update, Type _type)
			: name(_name), update(_update), type(_type) { }
		virtual ~Param() { }

		bool bind(GLuint program)
		{
			auto locFunc = type == Type::Uniform ? glGetUniformLocation : glGetAttribLocation;

			location = locFunc(program, (const GLchar*)name.c_str());

			refresh();

			return location != -1;
		}

		void refresh() const
		{
			if (!update) return;

			if (location != -1)
			{
				update(location);
			}
		}
	};

	// A shader's binding points
	struct Binding
	{
		using Map = std::map<std::string, Param>;

		Map uniforms;
		Map vertexAttributes;

		void bindTo(GLuint program)
		{
			for (auto& u : uniforms)
			{
				if (!u.second.bind(program))
				{
					log("\tUnable to bind uniform: {0}\n", u.first);
				}
			}


			for (auto& v : vertexAttributes)
			{
				if (!v.second.bind(program))
				{
					log("\tUnable to bind attribute: {0}\n", v.first);
				}
			}
		}

		void clear()
		{
			uniforms.clear();
			vertexAttributes.clear();
		}

		void refresh() const
		{
			refreshUniforms();
			refreshAttributes();
		}

		void refreshUniforms() const
		{
			for (auto& u : uniforms)
			{
				u.second.refresh();
			}
		}

		void refreshAttributes() const
		{
			for (auto& va : vertexAttributes)
			{
				va.second.refresh();
			}
		}

		// Adds a uniform to the binding and returns the text required to utilize it in shader compilation
		std::string addUniform(const std::string type, const std::string name, Param::Func func = Param::Func(),
			const std::string defaultValue = "")
		{
			std::string uniformText = "";

			if (uniforms.find(name) == uniforms.end())
			{
				uniforms[name] = Param(name, func, Param::Type::Uniform);
				uniformText = fmt::format("uniform {0} {1}{2};\n", type, name,
					defaultValue.empty() ? "" : fmt::format(" = {0}", defaultValue));
			}

			return uniformText;
		}

		// Adds uniforms to the binding and returns the text required to utilize them in shader compilation
		std::string addUniforms(const std::string type, const std::string name, int amount, Param::Func func)
		{
			std::string uniformText = "";

			if (uniforms.find(name) == uniforms.end())
			{
				uniforms[name] = Param(name, func, Param::Type::Uniform);
				uniformText = fmt::format("uniform {0} {1}[{2}];\n", type, name, amount);
			}

			return uniformText;
		}

		// Adds an attribute to the binding and returns the text required to utilize it in shader compilation
		std::string addVertexAttribute(const std::string type, const std::string name, Param::Func func = Param::Func(),
			const std::string interpType = "")
		{
			std::string attributeText = "";

			if (vertexAttributes.find(name) == vertexAttributes.end())
			{
				vertexAttributes[name] = Param(name, func, Param::Type::VertexAttribute);

				// Adds space after interpolation qualifier if present
				auto interp = interpType == "" ? interpType : interpType + " ";

				attributeText = fmt::format("{0}in {1} {2};\n", interp, type, name);
			}

			return attributeText;
		}
	};

	// Shader program
	GLuint program = 0;

	// Link log
	std::string linkLog;

	std::map<ShaderType, GLshader> shaders;

	Binding binding;

	// List of transform feedback parameters
	glsl_list xfbParams;

	// Conveniences flags for checking presence
	struct
	{
		bool hasVertexStage = false;
		bool hasTessellationStage = false;
		bool hasGeometryStage = false;
		bool hasFragStage = false;
	} flags;

	// Parses includes, string replaces version macro
	static std::string ProcessShaderText(const std::string& text)
	{
		std::string result = StringUtil::parseIncludes(text, Path);
		result = StringUtil::replaceAll(result, VersionMacro, Version);
		return result;
	}

	static std::string ProcessShaderText(const std::string& text, const std::vector<std::pair<std::string, std::string>>& replaceables) {
		std::string result = ProcessShaderText(text);

		for (auto& r : replaceables) {
			result = StringUtil::replaceAll(result, r.first, r.second);
		}
		return result;
	}

	Shader() { }
	Shader(const std::string& vertexText, const std::string& fragText,
		const std::string& geoText = "", const std::string& tcText = "",
		const std::string& teText = "", glsl_list _xfbParams = {});
	Shader(const std::string& src, ShaderType sType);
	Shader(const std::string& shaderName, glsl_list _xfbParams = {});

	virtual ~Shader()
	{
		destroy();
	}

	// Initializes the shader by creating the program, compiling and attaching source, and linking
	bool init(bool recompile = false, std::vector<ShaderFragOutputBindings> fragBindings = {});

	// Binds the shader given the provided uniform and attribute binding details
	void bind(const Binding& _binding, bool purgeBad = true);

	void start()
	{
		if (program != 0)
		{
			glUseProgram(program);
		}
	}

	void stop()
	{
		glUseProgram(0);
	}

	bool link();

	void destroy(bool clearGLshaders = true);

	static std::string LoadText(const std::string& src);

	static bool WriteText(const std::string& path, const std::string& txt);
};

using ShaderParamFunc = Shader::Param::Func;
using ShaderBinding = Shader::Binding;
