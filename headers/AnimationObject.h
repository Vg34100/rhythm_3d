#pragma once

#include "globals.h"

struct AnimationObject {
	static int ObjectIndex;
	int index = 0;
	AnimationObjectType shapeType = AnimationObjectType::sphere;
	mat4 transform = mat4();
	mat4 localTransform = mat4();
	bool transformUpdatedThisFrame = false;
	bool overrideUpdate = false;
	void* texture = nullptr;
	vec4 color = vec4(1.f);
	vec3 position = vec3(0.f);
	vec3 rotation = vec3(0.f); // // Euler angles
	vec3 scale = vec3(1.f);
	vec3 localPosition = vec3(0.f);
	vec3 localRotation = vec3(0.f); // // Euler angles
	vec3 localScale = vec3(1.f);
	vec3 localPivot = vec3(0.f);
	bool selected = false;
	bool useVertexTransform = false;
	bool useFragmentTransform = false;
	bool visible = true;
	bool selectable = true;

	vec3 orbitRotation = vec3(0.);
	vec3 constantRotation = vec3(0.);
	AnimationObject* parent = nullptr;
	int parentIndex = -1;
	int lightIndex = -1;
	bool useLighting = true;
	bool cullFace = true;
	bool billboard = false;
	Axis forwardDirection = Axis::X;

	float size = 1.0f;

	std::string meshName;

	AnimationObject() : index(ObjectIndex++)  {
		//log("Creating object {0} from {1}\n", index, ObjectIndex);
	}

	AnimationObject(AnimationObjectType st, const vec3& pos = vec3(0.f), const vec3& rot = vec3(0.f), const vec3& sca = vec3(1.f),
		void* tex = nullptr, const vec4& col = vec4(1.f), int li = -1)
		: index(ObjectIndex++), shapeType(st), localPosition(pos), localRotation(rot), localScale(sca), texture(tex), color(col), lightIndex(li) {

		update();
	}

	AnimationObject(const std::string& _meshName, const vec3& pos = vec3(0.f), const vec3& rot = vec3(0.f), const vec3& sca = vec3(1.f),
		void* tex = nullptr, const vec4& col = vec4(1.f), int li = -1)
		: meshName(_meshName), index(ObjectIndex++), shapeType(AnimationObjectType::model), localPosition(pos), localRotation(rot), localScale(sca), texture(tex), color(col), lightIndex(li) {

		update();
	}

	AnimationObject(const AnimationObject& rhs) {
		index = rhs.index;
		shapeType = rhs.shapeType;
		transform = rhs.transform;
		localTransform = rhs.localTransform;
		transformUpdatedThisFrame = rhs.transformUpdatedThisFrame;
		overrideUpdate = rhs.overrideUpdate;
		texture = rhs.texture;
		color = rhs.color;
		position = rhs.position;
		rotation = rhs.rotation;
		scale = rhs.scale;
		localPosition = rhs.localPosition;
		localRotation = rhs.localRotation;
		localScale = rhs.localScale;
		localPivot = rhs.localPivot;
		selected = rhs.selected;
		useVertexTransform = rhs.useVertexTransform;
		useFragmentTransform = rhs.useFragmentTransform;
		visible = rhs.visible;
		selectable = rhs.selectable;

		orbitRotation = rhs.orbitRotation;
		constantRotation = rhs.constantRotation;
		parent = rhs.parent;
		parentIndex = rhs.parentIndex;
		lightIndex = rhs.lightIndex;
		useLighting = rhs.useLighting;
		cullFace = rhs.cullFace;
		billboard = rhs.billboard;
		forwardDirection = rhs.forwardDirection;

		meshName = rhs.meshName;
	}

	void setParent(AnimationObject* p, bool adjustTf = true);

	void setFromMatrix(const mat4& m) {
		auto decomp = glm::decompose(m);
		localPosition = decomp.position;
		localRotation = glm::degrees(glm::eulerAngles(decomp.rotation));
		localScale = decomp.scale;
	}

	void update();

	void updateMatrix(bool force = false);

	bool renderUI();
};


void to_json(json& j, const AnimationObject& s);
void from_json(const json& j, AnimationObject& s);