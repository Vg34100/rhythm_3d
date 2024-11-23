#include "AnimationObject.h"
#include "AnimationObjectRenderer.h"
#include "Meshing.h"
#include "Texture.h"

#include "Application.h"
#include "Renderer.h"

#include "UIHelpers.h"

#include "imgui.h"


int AnimationObject::ObjectIndex = 0;

void AnimationObject::setParent(AnimationObject* p, bool adjustTf) {
	if (p && p != this) {
		parent = p;
		for (auto& s : Application::get().getObjects()) {
			if (s == parent) {
				parentIndex = s->index;
				break;
			}
		}
		if (parentIndex == -1) {
			parent = nullptr;
			log("Unable to set parent for shape {0} - parent index not found\n", index);
			return;
		}

		log("Assigning shape {0} as parent of shape {1}\n", parent->index, index);
		if (adjustTf) {
			// To preserve the child's original position before parenting, we need to 
			// find the local transform such that:
			// originalTf = parentTf* localTf
			// parentTf^-1  *originalTf = localTf
			mat4 parentTf = parent->transform;
			mat4 parentInv = glm::inverse(parentTf);
			mat4 newLocalTf = parentInv * transform;
			setFromMatrix(newLocalTf);
		}
	}
	else if (p == nullptr) {
		parent = nullptr;
		parentIndex = -1;
		setFromMatrix(transform);
	}
}

void AnimationObject::update() {
	transformUpdatedThisFrame = false;

	if (glm::length(constantRotation) > 0.f) {
		localRotation += constantRotation;
	}

	if (glm::length(orbitRotation) > 0.f) {
		float dt = Application::get().deltaTime;
		mat4 orbitRot = glm::toMat4(quaternion(glm::radians(orbitRotation * dt)));

		localPosition = orbitRot * vec4(localPosition, 1.f);
	}
}

void AnimationObject::updateMatrix(bool force) {
	if (transformUpdatedThisFrame && !force) return;
	if (overrideUpdate && !force) return;

	if (billboard) {
		auto& cam = Application::get().getRenderer<OpenGLRenderer>()->camera;

		vec3 modelFwd = glm::normalize(cam.Position - position);
		vec3 modelUp = cam.currentUp;
		vec3 modelSide = glm::cross(modelUp, modelFwd);

		mat3 oldRot = transform;
		mat3 newRot;
		if (forwardDirection == +Axis::X) {
			newRot[0] = modelFwd;
			newRot[1] = modelUp;
			newRot[2] = modelSide;
		}
		else if (forwardDirection == +Axis::Y) {
			newRot[0] = modelSide;
			newRot[1] = modelFwd;
			newRot[2] = modelUp;
		}
		else if (forwardDirection == +Axis::Z) {
			newRot[0] = modelUp;
			newRot[1] = modelSide;
			newRot[2] = modelFwd;
		}

		mat4 newRot4 = newRot;

		if (parent) {
			auto dc = glm::decompose(newRot4);
			localRotation = glm::degrees(glm::eulerAngles(dc.rotation));
		}
		else {
			auto dc = glm::decompose(newRot4);
			localRotation = glm::degrees(glm::eulerAngles(dc.rotation));
		}
	}

	mat4 T = glm::translate(localPosition);
	mat4 R = glm::translate(-localPivot) * glm::toMat4(quaternion(glm::radians(localRotation))) * glm::translate(localPivot);
	mat4 S = glm::scale(localScale);
	localTransform = T * R * S;

	if (parent && parent != this) {
		if (!parent->transformUpdatedThisFrame) parent->updateMatrix();

		mat4 cr = mat4(1.f);
		if (glm::length(constantRotation) > 0.f) {
			cr = glm::toMat4(quaternion(glm::radians(constantRotation)));
		}

		transform = parent->transform * localTransform;

		auto decomp = glm::decompose(transform);
		position = decomp.position;
		rotation = glm::degrees(glm::eulerAngles(decomp.rotation));
		scale = decomp.scale;
	}
	else {
		transform = localTransform;
		position = localPosition;
		rotation = localRotation;
		scale = localScale;
	}

	transformUpdatedThisFrame = true;
}

bool AnimationObject::renderUI() {
	ImGui::PushID((const void*)this);
	renderEnumDropDown<AnimationObjectType>("Shape type", shapeType);

	bool anyChange = false;

	bool s = selected;
	if (ImGui::Checkbox("Selected", &s)) {
		anyChange = true;
		Application::get().selectObject(index);
	}
	anyChange |= ImGui::InputFloat3("Position", glm::value_ptr(localPosition));
	anyChange |= ImGui::InputFloat3("Rotation", glm::value_ptr(localRotation));
	anyChange |= ImGui::SliderFloat3("Rotation slider", glm::value_ptr(localRotation), 0.0f, 360.f, "", 1.0f);
	anyChange |= ImGui::InputFloat3("Constant rotation", glm::value_ptr(constantRotation));
	anyChange |= ImGui::InputFloat3("Orbit rotation", glm::value_ptr(orbitRotation));
	anyChange |= ImGui::InputFloat3("Scale", glm::value_ptr(localScale));
	anyChange |= ImGui::InputFloat("Size", &size);
	anyChange |= ImGui::InputFloat3("Pivot", glm::value_ptr(localPivot));
	anyChange |= ImGui::ColorEdit4("Color", glm::value_ptr(color));

	ImGui::Text("World position: %.3f, %.3f, %.3f", position.x, position.y, position.z);
	ImGui::Text("World rotation: %.3f, %.3f, %.3f", rotation.x, rotation.y, rotation.z);
	ImGui::Text("World scale: %.3f, %.3f, %.3f", scale.x, scale.y, scale.z);


	if (ImGui::InputInt("Parent index", &parentIndex)) {
		if (parentIndex >= 0 && parentIndex < Application::get().objects.size()) {
			setParent(&Application::get().objects[parentIndex]);
		}
	}


	anyChange |= ImGui::InputInt("Light index", &lightIndex);
	anyChange |= ImGui::Checkbox("Use lighting", &useLighting);
	anyChange |= ImGui::Checkbox("Cull backface", &cullFace);
	anyChange |= ImGui::Checkbox("Billboard to camera", &billboard);
	if (billboard) {
		ImGui::Text("Forward direction");
		ImGui::SameLine();
		renderEnumButton<Axis>(forwardDirection);
	}
	anyChange |= ImGui::Checkbox("Vertex transform", &useVertexTransform);
	anyChange |= ImGui::Checkbox("Fragment transform", &useFragmentTransform);


	if (anyChange) {
		updateMatrix();
	}

	int i = long(texture);

	if (ImGui::InputInt("Texture ID", &i)) {
		texture = reinterpret_cast<void*>(i);
	}

	if (this->meshName != "") {
		auto& sr = AnimationObjectRenderer::get();
		auto& mc = sr.meshCatalog[meshName];
		if (mc.mesh && ImGui::CollapsingHeader(meshName.c_str())) {
			for (auto& mat : mc.mesh->materials) {
				if (mat.diffuseTexture) {
					mat.diffuseTexture->renderUI();
				}
			}
		}
	}

	ImGui::PopID();

	return anyChange;
}

void to_json(json& j, const AnimationObject& s) {
	j = json{ {"index", s.index}, {"shapeType", s.shapeType}, {"color", s.color},
		{"localPosition", s.localPosition}, {"localRotation", s.localRotation}, {"localScale", s.localScale},
		{"parentIndex", s.parentIndex}, {"lightIndex", s.lightIndex}
	};
}

void from_json(const json& j, AnimationObject& s) {
	j.at("index").get_to(s.index);
	j.at("shapeType").get_to(s.shapeType);
	j.at("color").get_to(s.color);
	j.at("localPosition").get_to(s.localPosition);
	j.at("localRotation").get_to(s.localRotation);
	j.at("localScale").get_to(s.localScale);
	j.at("parentIndex").get_to(s.parentIndex);
	j.at("lightIndex").get_to(s.lightIndex);
}