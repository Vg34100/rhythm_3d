#include "AnimationObjectRenderer.h"

#include "Textures.h"

#include "Application.h"
#include "Camera.h"
#include "Renderer.h"
#include "Lighting.h"

#include "Framebuffer.h"
#include "StringUtil.h"
#include "InputOutput.h"

#include "imgui.h"


void initJointPyramid(AnimationObjectRenderData& renderData) {
	// Here are the vertex positions needed for a simple 4-sided pyramid joint. The base is on YZ and the tip is on +1 x. 
	// Yes, +x is forward. So rotation will need to occur for proper alignment to make it point down a direction. 

	vec3 vertexPositions[] = {
		vec3(-0.f, 0.f, 1.f),		// 0
		vec3(-0.f, 1.f, 0.f),		// 1
		vec3(-0.f, 0.f, -1.f),		// 2
		vec3(-0.f, -1.f, 0.f),		// 3
		vec3(1.f, 0.f, 0.f),		// 4 (top of pyramid)
	};

	vec3 faceNormals[] = {
		vec3(-1.f, 0.f, 0.f), // base of pyramid
		glm::normalize(glm::cross(vertexPositions[4] - vertexPositions[0], vertexPositions[1] - vertexPositions[0])),	// front
		glm::normalize(glm::cross(vertexPositions[4] - vertexPositions[1], vertexPositions[2] - vertexPositions[1])),	// top
		glm::normalize(glm::cross(vertexPositions[4] - vertexPositions[2], vertexPositions[3] - vertexPositions[2])),	// back
		glm::normalize(glm::cross(vertexPositions[4] - vertexPositions[3], vertexPositions[0] - vertexPositions[3]))	// bottom
	};

	vec2 uvs[] = {
		vec2(0.f),
		vec2(1.f, 0.f),
		vec2(1.f),
		vec2(0.f, 1.f)
	};

	vec4 color = vec4(1.f);

	std::vector<PlainOldVertex> vertices;

	// First face: bottom half of base
	PlainOldVertex v0 = { vertexPositions[0], faceNormals[0], uvs[0] }; vertices.push_back(v0);
	PlainOldVertex v1 = { vertexPositions[1], faceNormals[0], uvs[1] }; vertices.push_back(v1);
	PlainOldVertex v2 = { vertexPositions[2], faceNormals[0], uvs[2] }; vertices.push_back(v2);

	// Second face: top half of base
	vertices.push_back(v2);
	PlainOldVertex v3 = { vertexPositions[3], faceNormals[0], uvs[3] }; vertices.push_back(v3);
	vertices.push_back(v0);

	// Third face: front 
	vertices.push_back({ vertexPositions[0], faceNormals[1], uvs[0] });
	vertices.push_back({ vertexPositions[4], faceNormals[1], uvs[1] });
	vertices.push_back({ vertexPositions[1], faceNormals[1], uvs[2] });

	// Fourth face: top
	vertices.push_back({ vertexPositions[1], faceNormals[2], uvs[0] });
	vertices.push_back({ vertexPositions[4], faceNormals[2], uvs[1] });
	vertices.push_back({ vertexPositions[2], faceNormals[2], uvs[2] });

	// Fifth face: back
	vertices.push_back({ vertexPositions[2], faceNormals[3], uvs[0] });
	vertices.push_back({ vertexPositions[4], faceNormals[3], uvs[1] });
	vertices.push_back({ vertexPositions[3], faceNormals[3], uvs[2] });

	// Sixth face: bottom
	vertices.push_back({ vertexPositions[3], faceNormals[0], uvs[0] });
	vertices.push_back({ vertexPositions[4], faceNormals[0], uvs[1] });
	vertices.push_back({ vertexPositions[0], faceNormals[0], uvs[2] });

	// Silly index buffer?
	std::vector<GLuint> indices;
	for (GLuint i = 0; i < vertices.size(); i++) {
		indices.push_back(i);
	}

	renderData.indexVBO = s_ptr<VectorBuffer<GLuint>>(new VectorBuffer<GLuint>(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW));
	renderData.vertexVBO = s_ptr<VectorBuffer<PlainOldVertex>>(new VectorBuffer<PlainOldVertex>(GL_ARRAY_BUFFER, vertices, GL_STATIC_DRAW));
}

void initQuad(AnimationObjectRenderData& renderData) {
	// Creates a quad made of two triangles with a normal facing the +X axis. 
	// The quad is centered at (0, 0, 0) and has a width and height of 1. 
	// Here are the vertex positions needed for a simple 4-sided pyramid joint. The base is on YZ and the tip is on +1 x. 
	// Yes, +x is forward. So rotation will need to occur for proper alignment to make it point down a direction. 

	vec3 vertexPositions[] = {
		vec3(0.f, -0.5f, 0.5f),		// 0
		vec3(0.f, -0.5f, -0.5f),	// 1
		vec3(0.f, 0.5f, -0.5f),		// 2
		vec3(0.f, 0.5f, 0.5f),		// 3
	};

	vec3 faceNormals[] = {
		vec3(1.f, 0.f, 0.f)
	};

	vec2 uvs[] = {
		vec2(0.f),
		vec2(1.f, 0.f),
		vec2(1.f),
		vec2(0.f, 1.f)
	};



	vec4 color = vec4(1.f);

	std::vector<PlainOldVertex> vertices;

	// First face: bottom left to bottom right to top right
	PlainOldVertex v0 = { vertexPositions[0], faceNormals[0], uvs[0] }; vertices.push_back(v0);
	PlainOldVertex v1 = { vertexPositions[1], faceNormals[0], uvs[1] }; vertices.push_back(v1);
	PlainOldVertex v2 = { vertexPositions[2], faceNormals[0], uvs[2] }; vertices.push_back(v2);

	// Second face: top right to top left to bottom left
	vertices.push_back(v2);
	PlainOldVertex v3 = { vertexPositions[3], faceNormals[0], uvs[3] }; vertices.push_back(v3);
	vertices.push_back(v0);

	// Silly index buffer?
	std::vector<GLuint> indices;
	for (GLuint i = 0; i < vertices.size(); i++) {
		indices.push_back(i);
	}

	renderData.indexVBO = s_ptr<VectorBuffer<GLuint>>(new VectorBuffer<GLuint>(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW));
	renderData.vertexVBO = s_ptr<VectorBuffer<PlainOldVertex>>(new VectorBuffer<PlainOldVertex>(GL_ARRAY_BUFFER, vertices, GL_STATIC_DRAW));
}

void initBox(AnimationObjectRenderData& renderData) {
	// Creates a quad made of two triangles with a normal facing the +X axis. 
	// The quad is centered at (0, 0, 0) and has a width and height of 1. 
	// Here are the vertex positions needed for a simple 4-sided pyramid joint. The base is on YZ and the tip is on +1 x. 
	// Yes, +x is forward. So rotation will need to occur for proper alignment to make it point down a direction. 

	vec3 vertexPositions[] = {
		vec3(0.5f, -0.5f, 0.5f),		// 0: right bottom front
		vec3(0.5f, -0.5f, -0.5f),		// 1: right bottom back
		vec3(0.5f, 0.5f, -0.5f),		// 2: right top back
		vec3(0.5f, 0.5f, 0.5f),			// 3: right top front
		vec3(-0.5f, -0.5f, 0.5f),		// 4: left bottom front
		vec3(-0.5f, -0.5f, -0.5f),		// 5: left bottom back
		vec3(-0.5f, 0.5f, -0.5f),		// 6: left top back
		vec3(-0.5f, 0.5f, 0.5f),		// 7: left top front
	};

	vec3 faceNormals[] = {
		vec3(1.f, 0.f, 0.f),
		vec3(0.f, 1.f, 0.f),
		vec3(0.f, 0.f, 1.f),
	};

	vec2 uvs[] = {
		vec2(0.f),
		vec2(1.f, 0.f),
		vec2(1.f),
		vec2(0.f, 1.f)
	};



	vec4 color = vec4(1.f);

	std::vector<PlainOldVertex> vertices;

	// Right side: 
		// First face: bottom left to bottom right to top right
	{
		PlainOldVertex v0 = { vertexPositions[0], faceNormals[0], uvs[0] }; vertices.push_back(v0);
		PlainOldVertex v1 = { vertexPositions[1], faceNormals[0], uvs[1] }; vertices.push_back(v1);
		PlainOldVertex v2 = { vertexPositions[2], faceNormals[0], uvs[2] }; vertices.push_back(v2);
	}
	// Second face: top right to top left to bottom left
	{
		PlainOldVertex v2 = { vertexPositions[2], faceNormals[0], uvs[2] }; vertices.push_back(v2);
		PlainOldVertex v3 = { vertexPositions[3], faceNormals[0], uvs[3] }; vertices.push_back(v3);
		PlainOldVertex v0 = { vertexPositions[0], faceNormals[0], uvs[0] }; vertices.push_back(v0);
	}

	// 0: right bottom front
	// 1: right bottom back
	// 2: right top back
	// 3: right top front
	// 4: left bottom front
	// 5: left bottom back
	// 6: left top back
	// 7: left top front	

	// Top side:
		// First face: 7 3 2
	{
		PlainOldVertex v0 = { vertexPositions[7], faceNormals[1], uvs[0] }; vertices.push_back(v0);
		PlainOldVertex v1 = { vertexPositions[3], faceNormals[1], uvs[1] }; vertices.push_back(v1);
		PlainOldVertex v2 = { vertexPositions[2], faceNormals[1], uvs[2] }; vertices.push_back(v2);
	}
	// Second face: 2 6 7
	{
		PlainOldVertex v2 = { vertexPositions[2], faceNormals[1], uvs[2] }; vertices.push_back(v2);
		PlainOldVertex v3 = { vertexPositions[6], faceNormals[1], uvs[3] }; vertices.push_back(v3);
		PlainOldVertex v0 = { vertexPositions[7], faceNormals[1], uvs[0] }; vertices.push_back(v0);
	}

	// Left side:
		// First face: 5 4 7
	{
		PlainOldVertex v0 = { vertexPositions[5], -faceNormals[0], uvs[0] }; vertices.push_back(v0);
		PlainOldVertex v1 = { vertexPositions[4], -faceNormals[0], uvs[1] }; vertices.push_back(v1);
		PlainOldVertex v2 = { vertexPositions[7], -faceNormals[0], uvs[2] }; vertices.push_back(v2);
	}
	// Second face: 7 6 5
	{
		PlainOldVertex v2 = { vertexPositions[7], -faceNormals[0], uvs[2] }; vertices.push_back(v2);
		PlainOldVertex v3 = { vertexPositions[6], -faceNormals[0], uvs[3] }; vertices.push_back(v3);
		PlainOldVertex v0 = { vertexPositions[5], -faceNormals[0], uvs[0] }; vertices.push_back(v0);
	}

	// Bottom side:
		// First face: 5 1 0
	{
		PlainOldVertex v0 = { vertexPositions[5], -faceNormals[1], uvs[0] }; vertices.push_back(v0);
		PlainOldVertex v1 = { vertexPositions[1], -faceNormals[1], uvs[1] }; vertices.push_back(v1);
		PlainOldVertex v2 = { vertexPositions[0], -faceNormals[1], uvs[2] }; vertices.push_back(v2);
	}
	// Second face: 0 4 5
	{
		PlainOldVertex v2 = { vertexPositions[0], -faceNormals[1], uvs[2] }; vertices.push_back(v2);
		PlainOldVertex v3 = { vertexPositions[4], -faceNormals[1], uvs[3] }; vertices.push_back(v3);
		PlainOldVertex v0 = { vertexPositions[5], -faceNormals[1], uvs[0] }; vertices.push_back(v0);
	}

	// Front side
		// First face: 4 0 3
	{
		PlainOldVertex v0 = { vertexPositions[4], faceNormals[2], uvs[0] }; vertices.push_back(v0);
		PlainOldVertex v1 = { vertexPositions[0], faceNormals[2], uvs[1] }; vertices.push_back(v1);
		PlainOldVertex v2 = { vertexPositions[3], faceNormals[2], uvs[2] }; vertices.push_back(v2);
	}
	// Second face: 3 7 4
	{
		PlainOldVertex v2 = { vertexPositions[3], faceNormals[2], uvs[2] }; vertices.push_back(v2);
		PlainOldVertex v3 = { vertexPositions[7], faceNormals[2], uvs[3] }; vertices.push_back(v3);
		PlainOldVertex v0 = { vertexPositions[4], faceNormals[2], uvs[0] }; vertices.push_back(v0);
	}

	// Back side
		// First face: 1 5 6
	{
		PlainOldVertex v0 = { vertexPositions[1], -faceNormals[2], uvs[0] }; vertices.push_back(v0);
		PlainOldVertex v1 = { vertexPositions[5], -faceNormals[2], uvs[1] }; vertices.push_back(v1);
		PlainOldVertex v2 = { vertexPositions[6], -faceNormals[2], uvs[2] }; vertices.push_back(v2);
	}
	// Second face: 6 2 1
	{
		PlainOldVertex v2 = { vertexPositions[6], -faceNormals[2], uvs[2] }; vertices.push_back(v2);
		PlainOldVertex v3 = { vertexPositions[2], -faceNormals[2], uvs[3] }; vertices.push_back(v3);
		PlainOldVertex v0 = { vertexPositions[1], -faceNormals[2], uvs[0] }; vertices.push_back(v0);
	}
	// Silly index buffer?
	std::vector<GLuint> indices;
	for (GLuint i = 0; i < vertices.size(); i++) {
		indices.push_back(i);
	}

	renderData.indexVBO = s_ptr<VectorBuffer<GLuint>>(new VectorBuffer<GLuint>(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW));
	renderData.vertexVBO = s_ptr<VectorBuffer<PlainOldVertex>>(new VectorBuffer<PlainOldVertex>(GL_ARRAY_BUFFER, vertices, GL_STATIC_DRAW));
}

void initSphere(AnimationObjectRenderData& renderData) {
	const float X = -.525731112119133606f;
	const float Z = -.850650808352039932f;
	const float N = 0.f;

	vec3 vertexPositions[] = {
		{-X,N,Z}, {X,N,Z}, {-X,N,-Z}, {X,N,-Z},
		{N,Z,X}, {N,Z,-X}, {N,-Z,X}, {N,-Z,-X},
		{Z,X,N}, {-Z,X, N}, {Z,-X,N}, {-Z,-X, N}
	};

	vec4 color = vec4(1.f);

	// Silly index buffer?
	std::vector<uvec3> triangleList = {
	  {0,4,1},{0,9,4},{9,5,4},{4,5,8},{4,8,1},
	  {8,10,1},{8,3,10},{5,3,8},{5,2,3},{2,7,3},
	  {7,10,3},{7,6,10},{7,11,6},{11,0,6},{0,1,6},
	  {6,1,10},{9,0,11},{9,11,2},{9,2,5},{7,2,11}
	};

	std::vector<GLuint> indices;
	std::vector<PlainOldVertex> vertices;

	bool useFaceVertex = false;

	auto subdivideTriangle = [useFaceVertex](const vec3& v1, const vec3& v2, const vec3& v3) {

		std::vector<PlainOldVertex> subVertices;

		auto v12 = glm::normalize((v1 + v2) * 0.5f);
		auto v23 = glm::normalize((v2 + v3) * 0.5f);
		auto v13 = glm::normalize((v1 + v3) * 0.5f);

		// First sub: v1, v12, v13
		vec3 f1Normal = glm::normalize(glm::cross(v12 - v1, v13 - v1));
		subVertices.push_back({ v1, useFaceVertex ? f1Normal : v1 });
		subVertices.push_back({ v12, useFaceVertex ? f1Normal : v12 });
		subVertices.push_back({ v13, useFaceVertex ? f1Normal : v13 });

		// Second sub: v12, v2, v23
		vec3 f2Normal = glm::normalize(glm::cross(v2 - v12, v23 - v2));
		subVertices.push_back({ v12, useFaceVertex ? f1Normal : v12 });
		subVertices.push_back({ v2, useFaceVertex ? f1Normal : v2 });
		subVertices.push_back({ v23, useFaceVertex ? f1Normal : v23 });

		// Third sub: v12, v23, v13
		vec3 f3Normal = glm::normalize(glm::cross(v23 - v12, v13 - v12));
		subVertices.push_back({ v12, useFaceVertex ? f1Normal : v12 });
		subVertices.push_back({ v23, useFaceVertex ? f1Normal : v23 });
		subVertices.push_back({ v13, useFaceVertex ? f1Normal : v13 });

		// Fourth sub: v13, v23, v3
		vec3 f4Normal = glm::normalize(glm::cross(v23 - v13, v3 - v13));
		subVertices.push_back({ v13, useFaceVertex ? f1Normal : v13 });
		subVertices.push_back({ v23, useFaceVertex ? f1Normal : v23 });
		subVertices.push_back({ v3, useFaceVertex ? f1Normal : v3 });

		return subVertices;
		};

	for (auto& t : triangleList) {
		const auto& v1 = vertexPositions[t.x];
		const auto& v2 = vertexPositions[t.y];
		const auto& v3 = vertexPositions[t.z];

		vec3 faceNormal = (glm::normalize(glm::cross(v2 - v1, v3 - v1)));

		vertices.push_back({ v1, faceNormal });
		vertices.push_back({ v2, faceNormal });
		vertices.push_back({ v3, faceNormal });
	}

	int subdivideLevels = 2;

	for (size_t i = 0; i < subdivideLevels; i++) {
		std::vector<PlainOldVertex> newVertices;

		for (size_t j = 0; j < vertices.size(); j += 3) {
			auto s = subdivideTriangle(vertices[j].position, vertices[j + 1].position, vertices[j + 2].position);

			newVertices.insert(newVertices.end(), s.begin(), s.end());
		}

		vertices = newVertices;
	}

	GLuint index = 0;
	for (const auto& v : vertices) {
		indices.push_back(index++);
	}

	renderData.indexVBO = s_ptr<VectorBuffer<GLuint>>(new VectorBuffer<GLuint>(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW));
	renderData.vertexVBO = s_ptr<VectorBuffer<PlainOldVertex>>(new VectorBuffer<PlainOldVertex>(GL_ARRAY_BUFFER, vertices, GL_STATIC_DRAW));
}

void initTri(AnimationObjectRenderData& renderData) {

	vec3 vertexPositions[] = {
		vec3(-0.5f, -0.5f, 0.f),		// 0
		vec3(0.5f, -0.5f, 0.f),
		vec3(0.f, 0.5f, 0.0f)
	};

	vec3 faceNormals[] = {
		vec3(0.f, 0.f, 1.f)
	};

	vec2 uvs[] = {
		vec2(0.f),
		vec2(1.f, 0.f),
		vec2(0.5f, 1.f)
	};



	vec4 color = vec4(1.f);

	std::vector<PlainOldVertex> vertices;

	// First face: bottom left to bottom right to top right
	PlainOldVertex v0 = { vertexPositions[0], faceNormals[0], uvs[0] }; vertices.push_back(v0);
	PlainOldVertex v1 = { vertexPositions[1], faceNormals[0], uvs[1] }; vertices.push_back(v1);
	PlainOldVertex v2 = { vertexPositions[2], faceNormals[0], uvs[2] }; vertices.push_back(v2);

	// Silly index buffer?
	std::vector<GLuint> indices;
	for (GLuint i = 0; i < vertices.size(); i++) {
		indices.push_back(i);
	}

	renderData.indexVBO = s_ptr<VectorBuffer<GLuint>>(new VectorBuffer<GLuint>(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW));
	renderData.vertexVBO = s_ptr<VectorBuffer<PlainOldVertex>>(new VectorBuffer<PlainOldVertex>(GL_ARRAY_BUFFER, vertices, GL_STATIC_DRAW));
}

void initModel(AnimationObjectRenderData& renderData, std::string filename) {

	if (StringUtil::lower(filename).find(".obj") != std::string::npos) {
		renderData.mesh = LoadOBJModel(filename, true);

		renderData.indexVBO = s_ptr<VectorBuffer<GLuint>>(new VectorBuffer<GLuint>(GL_ELEMENT_ARRAY_BUFFER, renderData.mesh->indices, GL_STATIC_DRAW));
		renderData.vertexVBO = s_ptr<VectorBuffer<PlainOldVertex>>(new VectorBuffer<PlainOldVertex>(GL_ARRAY_BUFFER, renderData.mesh->vertex_data, GL_STATIC_DRAW));
	}
}

AnimationObjectRenderData::AnimationObjectRenderData(AnimationObjectType st) {
	this->shapeType = st;
	this->objectName = shapeType._to_string();

	glGenVertexArrays(1, &vao);
	glGenVertexArrays(1, &shadowVAO);

	switch (shapeType) {
	case AnimationObjectType::joint:
		initJointPyramid(*this);
		break;
	case AnimationObjectType::sphere:
		initSphere(*this);
		break;
	case AnimationObjectType::quad:
		initQuad(*this);
		break;
	case AnimationObjectType::box:
		initBox(*this);
		break;
	case AnimationObjectType::tri:
		initTri(*this);
		break;
	case AnimationObjectType::model:
		log("Initializing a model requires a filename!\n");
		[[fallthrough]];
	default:
		initSphere(*this);
		break;
	}

	initShader();
}

AnimationObjectRenderData::AnimationObjectRenderData(std::string filename) {
	this->shapeType = AnimationObjectType::model;

	auto splitName = StringUtil::split(filename, '/');

	if (splitName.empty()) {
		splitName = StringUtil::split(filename, '\\');
	}

	if (!splitName.empty()) {
		this->objectName = splitName.back();
	}
	else {
		this->objectName = filename;
	}


	glGenVertexArrays(1, &vao);
	glGenVertexArrays(1, &shadowVAO);

	initModel(*this, filename);

	initShader();
}


bool AnimationObjectRenderData::initShader() {
	auto vertText = Shader::LoadText("objects/staticmesh.vert");
	auto fragText = Shader::LoadText("objects/staticmesh.frag");

	shader = s_ptr<Shader>(new Shader(vertText, fragText));

	std::vector<ShaderFragOutputBindings> fragBindings = {
		{0, "frag_color"},
		{1, "frag_prim"}
	};

	if (!shader->init(false, fragBindings))
	{
		log("Unable to load {0} shader!\n", objectName);
		return false;
	}

	ShaderBinding binding;

	binding.addVertexAttribute("vec3", "position", [this](GLint loc) {
		vertexVBO->bind();
		glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, sizeof(PlainOldVertex), (const GLvoid*)offsetof(PlainOldVertex, position));
		glEnableVertexAttribArray(loc);
		});

	binding.addVertexAttribute("vec3", "normal", [this](GLint loc) {
		vertexVBO->bind();
		glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, sizeof(PlainOldVertex), (const GLvoid*)offsetof(PlainOldVertex, normal));
		glEnableVertexAttribArray(loc);
		});

	binding.addVertexAttribute("vec2", "uv", [this](GLint loc) {
		vertexVBO->bind();
		glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, sizeof(PlainOldVertex), (const GLvoid*)offsetof(PlainOldVertex, uv));
		glEnableVertexAttribArray(loc);
		});
	binding.addVertexAttribute("vec4", "color", [this](GLint loc) {
		vertexVBO->bind();
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(PlainOldVertex), (const GLvoid*)offsetof(PlainOldVertex, color));
		glEnableVertexAttribArray(loc);
		});

	binding.addUniform("vec3", "viewDirection", [this](GLint loc) {
		auto& cam = Application::get().getRenderer<OpenGLRenderer>()->camera;
		auto axis = glm::normalize(cam.Lookat - cam.Position);
		glUniform3fv(loc, 1, glm::value_ptr(axis));
		});
	glBindVertexArray(vao);
	shader->bind(binding);
	indexVBO->bind();
	glBindVertexArray(0);

	// Shadow shader
	auto shadowVertText = Shader::LoadText("objects/staticshadow.vert");
	auto shadowFragText = Shader::LoadText("objects/staticshadow.frag");

	shadowShader = s_ptr<Shader>(new Shader(shadowVertText, shadowFragText));

	if (!shadowShader->init())
	{
		log("Unable to load {0} shadow shader!\n", objectName);
		return false;
	}

	ShaderBinding shadowBinding;

	shadowBinding.addVertexAttribute("vec3", "position", [this](GLint loc) {
		vertexVBO->bind();
		glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, sizeof(PlainOldVertex), (const GLvoid*)offsetof(PlainOldVertex, position));
		glEnableVertexAttribArray(loc);
		});

	glBindVertexArray(shadowVAO);
	shadowShader->bind(shadowBinding);
	indexVBO->bind();
	glBindVertexArray(0);

	return true;
}

AnimationObjectRenderer& AnimationObjectRenderer::get()
{
	static AnimationObjectRenderer instance;
	return instance;
}

bool AnimationObjectRenderer::init()
{
	for (const auto m : AnimationObjectType::_values()) {
		shapeCatalog[m] = AnimationObjectRenderData(m);
	}

	return true;
}


void AnimationObjectRenderer::beginBatchRender(AnimationObjectType shapeType, bool overrideColor, const vec4& batchColor, bool isShadow)
{
	if (!initialized)
	{
		initialized = init();
		if (!initialized) return;
	}

	auto meshToRender = shapeCatalog.find(shapeType);
	if (meshToRender == shapeCatalog.end()) {
		shapeCatalog[shapeType] = AnimationObjectRenderData(shapeType);
	}

	currentMesh = &shapeCatalog[shapeType];

	//mesh->beginRender(ShadingMode::Shaded, true);

	//glPushAttrib(GL_ENABLE_BIT);
	if (!isShadow) {
		glBindVertexArray(currentMesh->vao);
		//glEnable(GL_DEPTH_TEST);
		//glDepthFunc(GL_GREATER);
		currentMesh->shader->start();

		GLint nmLoc = glGetUniformLocation(currentMesh->shader->program, "normalMatrix");
		mat4 normalMatrix = glm::transpose(glm::inverse(Application::get().renderer->camera.view));
		glUniformMatrix3fv(nmLoc, 1, GL_FALSE, (const GLfloat*)glm::value_ptr(normalMatrix));

		GLint gcLoc = glGetUniformLocation(currentMesh->shader->program, "globalColor");
		if (overrideColor) {
			glUniform4fv(gcLoc, 1, glm::value_ptr(batchColor));
		}
		else {
			glUniform4fv(gcLoc, 1, glm::value_ptr(globalColor));
		}

		GPU::Lighting::get().bind(currentMesh->shader);
	}
	else {
		glBindVertexArray(currentMesh->shadowVAO);
		currentMesh->shadowShader->start();
	}
}

void AnimationObjectRenderer::beginBatchRender(const AnimationObject& shape, bool overrideColor, const vec4& batchColor, bool isShadow) {
	if (!initialized)
	{
		initialized = init();
		if (!initialized) return;
	}

	auto meshToRender = meshCatalog.find(shape.meshName);
	if (meshToRender == meshCatalog.end()) {
		meshCatalog[shape.meshName] = AnimationObjectRenderData(shape.meshName);

		// Check for materials file

		auto mtlFile = StringUtil::replaceAll(shape.meshName, ".obj", ".mtl");
		if (IO::pathExists(mtlFile)) {
			meshCatalog[shape.meshName].mesh->materials = MaterialData::LoadFromFile(mtlFile);
			if (!meshCatalog[shape.meshName].mesh->materials.empty()) {
				AnimationObject* s = (AnimationObject*)&shape;
				if (meshCatalog[shape.meshName].mesh->materials.front().diffuseTexture) {
					s->texture = reinterpret_cast<void*>(meshCatalog[shape.meshName].mesh->materials.front().diffuseTexture->id);
				}

			}
		}
	}

	currentMesh = &meshCatalog[shape.meshName];

	//mesh->beginRender(ShadingMode::Shaded, true);

	//glPushAttrib(GL_ENABLE_BIT);
	if (!isShadow) {
		glBindVertexArray(currentMesh->vao);
		//glEnable(GL_DEPTH_TEST);
		//glDepthFunc(GL_GREATER);
		currentMesh->shader->start();

		GLint nmLoc = glGetUniformLocation(currentMesh->shader->program, "normalMatrix");
		mat4 normalMatrix = glm::transpose(glm::inverse(Application::get().renderer->camera.view));
		glUniformMatrix3fv(nmLoc, 1, GL_FALSE, (const GLfloat*)glm::value_ptr(normalMatrix));

		GLint gcLoc = glGetUniformLocation(currentMesh->shader->program, "globalColor");
		if (overrideColor) {
			glUniform4fv(gcLoc, 1, glm::value_ptr(batchColor));
		}
		else {
			glUniform4fv(gcLoc, 1, glm::value_ptr(globalColor));
		}

		GPU::Lighting::get().bind(currentMesh->shader);
	}
	else {
		glBindVertexArray(currentMesh->shadowVAO);
		currentMesh->shadowShader->start();
	}
}

void AnimationObjectRenderer::endBatchRender(bool isShadow)
{
	if (!isShadow) {
		currentMesh->shader->stop();
	}
	else {
		currentMesh->shadowShader->stop();
	}

	glBindVertexArray(0);

	//glPopAttrib();
}

void AnimationObjectRenderer::render(AnimationObjectType shapeType, const mat4& mat, bool singular, bool isShadow)
{
	if (singular)
	{
		beginBatchRender(shapeType, false, vec4(1.f), isShadow);
	}

	renderBatch(mat, isShadow);

	if (singular)
	{
		endBatchRender(isShadow);
	}

}

void AnimationObjectRenderer::renderLight(const mat4& mat) {
	beginBatchRender(AnimationObjectType::joint, false, vec4(1.f), false);

	GLint lLoc = glGetUniformLocation(currentMesh->shader->program, "isLight");
	glUniform1i(lLoc, 1);


	auto ogl = std::dynamic_pointer_cast<OpenGLRenderer>(Application::get().renderer);
	if (ogl && ogl->dummyInput) {
		glEnable(GL_DEPTH_TEST);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ogl->dummyInput->textures[0]->id);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, ogl->dummyInput->textures[1]->id);
		GLint stLoc = glGetUniformLocation(currentMesh->shader->program, "shadowTexture");
		glUniform1i(stLoc, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_ALWAYS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	}

	renderBatch(mat, false);

	endBatchRender(false);
}

void AnimationObjectRenderer::renderBatch(const mat4& mat, bool isShadow) {
	static mat4 lightmvp;

	auto shaderToUse = isShadow ? currentMesh->shadowShader : currentMesh->shader;

	if (isShadow) {
		lightmvp = Application::get().renderer->camera.viewproj;
	}
	else {
		GLint lightmvpLoc = glGetUniformLocation(shaderToUse->program, "lightmvp");
		glUniformMatrix4fv(lightmvpLoc, 1, GL_FALSE, glm::value_ptr(lightmvp));
	}

	GLint mLoc = glGetUniformLocation(shaderToUse->program, "model");
	if (mLoc != -1) {
		glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mat));
	}

	GLint mvpLoc = glGetUniformLocation(shaderToUse->program, "mvp");
	if (mvpLoc != -1) {
		mat4 mvp = Application::get().renderer->camera.viewproj * mat;
		glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
	}

	glDrawElements(GL_TRIANGLES, currentMesh->indexVBO->data.size(), GL_UNSIGNED_INT, 0);
}

void AnimationObjectRenderer::renderBatchWithOwnColor(const mat4& mat, const vec4& color, const GLuint texture, bool isShadow) {

	if (!isShadow) {
		GLint gcLoc = glGetUniformLocation(currentMesh->shader->program, "globalColor");
		glUniform4fv(gcLoc, 1, glm::value_ptr(color));

		glActiveTexture(GL_TEXTURE0);
		GLint utLoc = glGetUniformLocation(currentMesh->shader->program, "useTexture");
		if (texture != 0) {
			glUniform1i(utLoc, 1);
			glBindTexture(GL_TEXTURE_2D, texture);
		}
		else {
			glUniform1i(utLoc, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		GLint smLoc = glGetUniformLocation(currentMesh->shader->program, "useShadow");
		glActiveTexture(GL_TEXTURE1);
		auto ogl = std::dynamic_pointer_cast<OpenGLRenderer>(Application::get().renderer);
		if (ogl && ogl->shadowMap) {
			glUniform1i(smLoc, 1);
			GLint stLoc = glGetUniformLocation(currentMesh->shader->program, "shadowTexture");
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

			GLint sbLoc = glGetUniformLocation(currentMesh->shader->program, "shadowBias");
			glUniform2fv(sbLoc, 1, glm::value_ptr(shadowBias));

			auto& l = GPU::Lighting::get();
			GLint lpLoc = glGetUniformLocation(currentMesh->shader->program, "lightPosition");
			GLint ldLoc = glGetUniformLocation(currentMesh->shader->program, "lightDirection");

			glUniform3fv(lpLoc, 1, glm::value_ptr(l.position));
			auto lightDir = glm::normalize(l.center - l.position);
			glUniform3fv(ldLoc, 1, glm::value_ptr(lightDir));


		}
		else {
			glUniform1i(smLoc, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	renderBatch(mat, isShadow);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void AnimationObjectRenderer::renderBatchWithOwnColor(const AnimationObject& shape, bool isShadow, bool useShapeColor) {

	if (!isShadow) {
		GLint lLoc = glGetUniformLocation(currentMesh->shader->program, "isLight");
		glUniform1i(lLoc, (shape.lightIndex != -1));

		if (useShapeColor) {
			GLint gcLoc = glGetUniformLocation(currentMesh->shader->program, "globalColor");
			glUniform4fv(gcLoc, 1, glm::value_ptr(shape.color));
		}

		GLint midLoc = glGetUniformLocation(currentMesh->shader->program, "meshID");
		if (midLoc != -1) {
			glUniform1i(midLoc, shape.index);
		}

		GLint sLoc = glGetUniformLocation(currentMesh->shader->program, "isSelected");
		if (sLoc != -1) {
			glUniform1i(sLoc, (int)shape.selected);
		}

		GLint uLoc = glGetUniformLocation(currentMesh->shader->program, "useLighting");
		if (uLoc != -1) {
			glUniform1i(uLoc, (int)shape.useLighting);
		}


		bool foundATexture = true;

		glActiveTexture(GL_TEXTURE0);
		GLint utLoc = glGetUniformLocation(currentMesh->shader->program, "useTexture");
		GLuint tex = static_cast<GLuint>(reinterpret_cast<intptr_t>(shape.texture));
		if (auto texptr = TextureRegistry::getTexture(tex)) {
			glUniform1i(utLoc, 1);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texptr->magFilter._to_integral());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texptr->minFilter._to_integral());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texptr->wrapS._to_integral());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texptr->wrapT._to_integral());
		}
		else if (shape.meshName != "") {
			auto& shapeMesh = meshCatalog[shape.meshName];
			if (!shapeMesh.mesh->materials.empty()) {
				if (auto texptr = shapeMesh.mesh->materials.front().diffuseTexture) {
					glUniform1i(utLoc, 1);
					glBindTexture(GL_TEXTURE_2D, texptr->id);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texptr->magFilter._to_integral());
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texptr->minFilter._to_integral());
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texptr->wrapS._to_integral());
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texptr->wrapT._to_integral());
				}
				else {
					foundATexture = false;
				}
			}
			else {
				foundATexture = false;
			}
		}
		else {
			foundATexture = false;
		}

		if (!foundATexture) {
			glUniform1i(utLoc, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		GLint smLoc = glGetUniformLocation(currentMesh->shader->program, "useShadow");
		glActiveTexture(GL_TEXTURE1);
		auto ogl = std::dynamic_pointer_cast<OpenGLRenderer>(Application::get().renderer);
		if (ogl && ogl->shadowMap) {
			glUniform1i(smLoc, 1);
			GLint stLoc = glGetUniformLocation(currentMesh->shader->program, "shadowTexture");
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

			GLint sbLoc = glGetUniformLocation(currentMesh->shader->program, "shadowBias");
			glUniform2fv(sbLoc, 1, glm::value_ptr(shadowBias));

			auto& l = GPU::Lighting::get();
			GLint lpLoc = glGetUniformLocation(currentMesh->shader->program, "lightPosition");
			GLint ldLoc = glGetUniformLocation(currentMesh->shader->program, "lightDirection");

			glUniform3fv(lpLoc, 1, glm::value_ptr(l.position));
			auto lightDir = glm::normalize(l.center - l.position);
			glUniform3fv(ldLoc, 1, glm::value_ptr(lightDir));
		}
		else {
			glUniform1i(smLoc, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	auto shaderToUse = isShadow ? currentMesh->shadowShader : currentMesh->shader;

	GLint vtLoc = glGetUniformLocation(shaderToUse->program, "useVertexTransform");
	glUniform1i(vtLoc, shape.useVertexTransform ? 1 : 0);

	GLint ftLoc = glGetUniformLocation(shaderToUse->program, "useFragmentTransform");
	glUniform1i(ftLoc, shape.useFragmentTransform ? 1 : 0);

	GLint itLoc = glGetUniformLocation(shaderToUse->program, "iTime");
	glUniform1f(itLoc, (float)Application::get().timeSinceStart);

	GLint sizLoc = glGetUniformLocation(shaderToUse->program, "size");
	if (sizLoc != -1) {
		glUniform1f(sizLoc, shape.size);
	}


	if (!shape.cullFace) {
		glDisable(GL_CULL_FACE);
	}
	renderBatch(shape.transform, isShadow);

	if (!shape.cullFace) {
		glEnable(GL_CULL_FACE);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

void AnimationObjectRenderer::renderUI() {
	if (ImGui::CollapsingHeader("StaticMeshRenderer")) {
		ImGui::ColorEdit4("Global color", glm::value_ptr(globalColor));
		ImGui::InputFloat2("Shadow bias", glm::value_ptr(shadowBias));
	}
}