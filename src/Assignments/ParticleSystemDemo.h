#pragma once

#include "Assignment.h"
#include "Buffer.h"

struct ParticlePhysics {
	vec3 position = vec3(0);
	vec3 linearVelocity = vec3(0);
	vec3 angularVelocity = vec3(0);
	vec3 acceleration = vec3(0);
	vec3 rotation = vec3(0);
	vec3 force = vec3(0);
	float mass = 1.0f;
	float dampening = 1.0f;

	//bool kinematic = false;

	//void renderUI() {
	//	ImGui::PushID((const void*)this);
	//	ImGui::Checkbox("Kinematic?", &kinematic);
	//	ImGui::InputFloat3("Force", glm::value_ptr(force));
	//	ImGui::InputFloat3("Acceleration", glm::value_ptr(acceleration));
	//	ImGui::InputFloat3("Velocity", glm::value_ptr(velocity));
	//	ImGui::InputFloat3("Position", glm::value_ptr(position));
	//	ImGui::InputFloat("Mass", &mass);
	//	ImGui::InputFloat("Dampening", &dampening);
	//	ImGui::PopID();
	//}

};

struct Particle {
	double lifeSpan = 1.0f;
	double createdAt = 0.;
	ParticlePhysics physics;
	vec3 size = vec3(0.1f);
	vec4 color = vec4(1.0f);
	bool alive = false;
};

struct ParticleRenderData {
	mat4 transform;
	vec4 color;
	vec4 data;
};

struct ParticleSystem {
	vec3 position = vec3(0.f);
	vec3 rotation = vec3(0.f, 0.0f, 90.0f);
	unsigned int maxParticles = 100;
	unsigned int numAlive = 0;
	unsigned int lastAliveIndex = 0;
	vec2 lifespanRange = vec2(1.0, 5.0);
	vec2 speedRange = vec2(1.0f, 5.0f);
	vec3 particleSize = vec3(0.1f);
	vec2 sizeChange = vec2(1.0f, 1.0f);
	float maxAngle = 45.0f;
	float maxParticleAngle = 20.0f;
	vec4 mainColor = vec4(1.f);
	bool randomizeColor = true;
	bool billboard = false;
	bool useGravity = false;
	bool useOtherForce = false;
	vec3 otherForce = vec3(0.f);

	unsigned int textureID = 0;

	// CPU-side particles
	std::vector<Particle> particles;

	// GPU-side particles
	spVectorBuffer<ParticleRenderData> renderData;

	void renderUI() {

		ImGui::PushID((const void*)this);

		ImGui::Text("Num particles: %lu, num alive %u", particles.size(), numAlive);
		int mp = maxParticles;
		if (ImGui::InputInt("Max particles", &mp)) {
			if (mp > 0) {
				maxParticles = mp;
			}
		}
		ImGui::InputFloat3("Position", glm::value_ptr(position));
		ImGui::InputFloat3("Rotation", glm::value_ptr(rotation));
		ImGui::InputFloat3("Size", glm::value_ptr(particleSize));
		ImGui::InputFloat2("Size change", glm::value_ptr(sizeChange));


		ImGui::InputFloat2("Lifespan range", glm::value_ptr(lifespanRange));
		ImGui::InputFloat("Max angle", &maxAngle);
		ImGui::InputFloat("Max particle angle", &maxParticleAngle);
		ImGui::InputFloat2("Speed range", glm::value_ptr(speedRange));

		int tid = textureID;
		if (ImGui::InputInt("Texture ID", &tid)) {
			textureID = tid;
		}

		ImGui::Checkbox("Billboard particles", &billboard);
		ImGui::Checkbox("Use gravity", &useGravity);
		ImGui::Checkbox("Use other force", &useOtherForce);
		if (useOtherForce) {
			ImGui::InputFloat3("Other force", glm::value_ptr(otherForce));
		}
		ImGui::Checkbox("Randomize color", &randomizeColor);
		ImGui::ColorEdit4("Main color", glm::value_ptr(mainColor));

		ImGui::PopID();
	}

};


class ParticleSystemDemo : public Assignment {
public:

	ParticleSystemDemo() : Assignment("Particle system", true) { }
	virtual ~ParticleSystemDemo() { }

	virtual void init();
	virtual void update();

	virtual void render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow = false);
	virtual void renderUI();

	s_ptr<Texture> fireTexture;

	virtual ptr_vector<AnimationObject> getObjects();
};
