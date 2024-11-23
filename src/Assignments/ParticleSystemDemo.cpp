// CMPS 4480 Particle system demo
// Name:  

#include "ParticleSystemDemo.h"
#include "Application.h"
#include "AnimationObjectRenderer.h"
#include "GLTFImporter.h"
#include "InputOutput.h"
#include "Input.h"
#include "Lighting.h"
#include "LineRenderer.h"
#include "Prompts.h"
#include "Renderer.h"
#include "QuadRenderer.h"
#include "Textures.h"
#include "UIHelpers.h"

#include <cassert>

#include <glm/gtc/random.hpp>

#include "implot.h"


namespace cmps_4480_particle_system {

	AnimationObject ground;
	AnimationObject quad;
	vec4 groundPlane;

	float physicsDt = 1.0f / 60.0f;

	std::vector<AnimationObject> objects;

	vec4 uvOffset = vec4(0, 0, 1, 1);

	std::vector<ParticleSystem> particleSystems;
}

using namespace cmps_4480_particle_system;

void ParticleSystemDemo::init() {

	ground = AnimationObject(AnimationObjectType::quad, vec3(0.f, -1.0f, 0.f), vec3(0.f, 0.f, 90.f), vec3(250), nullptr, vec4(vec3(0.5f), 1.f));
	ground.cullFace = true;

	vec3 p = ground.localPosition;
	vec3 n = vec3(0, 1, 0);

	groundPlane = vec4(n, -glm::dot(n, p));

	fireTexture = s_ptr<Texture>(new Texture(IO::getAssetRoot() + "/fireSheet.png"));

	quad = AnimationObject(
		AnimationObjectType::quad, vec3(0, 4, -4), vec3(0, 270, 0), vec3(4), (void*)fireTexture->id, vec4(1.f));

	particleSystems.push_back(ParticleSystem());

	initialized = true;
}

#define REDUCE_REUSE_RECYCLE
void updateParticleSystem(ParticleSystem& ps, float dt) {

	if (!ps.renderData) {
		std::vector<ParticleRenderData> prd(ps.maxParticles, { mat4(1.f), vec4(1.f), vec4(0.f) });
		ps.renderData = spVectorBuffer<ParticleRenderData>(new VectorBuffer<ParticleRenderData>(GL_ARRAY_BUFFER, prd, GL_DYNAMIC_DRAW));
	}
	else if (ps.renderData->data.size() < ps.particles.size()) {
		std::vector<ParticleRenderData> prd(ps.particles.size(), { mat4(1.f), vec4(1.f), vec4(0.f) });
		ps.renderData = spVectorBuffer<ParticleRenderData>(new VectorBuffer<ParticleRenderData>(GL_ARRAY_BUFFER, prd, GL_DYNAMIC_DRAW));
	}

	auto& cam = Application::get().getRenderer<OpenGLRenderer>()->camera;
	vec3 reverseLookDir = glm::normalize(cam.Position - cam.Lookat);

	//while (it != ps.particles.end()) {
#pragma omp parallel for
	for (int i = 0; i < ps.particles.size(); i++) {
		auto& p = ps.particles[i];


		auto aliveFor = getTime() - p.createdAt;
		if (!p.alive) {
			ps.renderData->data[i].data.x = p.alive ? 1 : 0;
		}
		else if (aliveFor > p.lifeSpan) {
			p.alive = false;
			ps.renderData->data[i].data.x = p.alive ? 1 : 0;
			ps.numAlive--;
		}

		else {

			float lt = aliveFor / p.lifeSpan;
			p.size = ps.particleSize * vec3(glm::mix(ps.sizeChange.x, ps.sizeChange.y, lt));

			// Update physics
			auto& physics = p.physics;

			if (ps.useGravity) {
				physics.linearVelocity += dt * vec3(0.f, -9.8f, 0.f);
			}

			if (ps.useOtherForce) {
				physics.linearVelocity += dt * ps.otherForce;
			}

			physics.position += physics.linearVelocity * dt;
			physics.rotation += physics.angularVelocity * dt;

			if (ps.billboard) {

				vec3 z = reverseLookDir;
				vec3 y = cam.currentUp;
				vec3 x = glm::cross(y, z);

				mat3 r;
				r[0] = x;
				r[1] = y;
				r[2] = z;
				quaternion rfix = quaternion(r);
				physics.rotation = glm::degrees(glm::eulerAngles(rfix));
			}


			mat4 quadPos = glm::translate(physics.position);
			mat4 quadRot = glm::toMat4(quaternion(glm::radians(physics.rotation)));
			mat4 quadScale = glm::scale(p.size);

			ps.renderData->data[i].transform = quadPos * quadRot * quadScale;
			ps.renderData->data[i].color = p.color * ps.mainColor;
			ps.renderData->data[i].data.x = p.alive ? 1 : 0;
		}
	}

	ps.renderData->update(0, ps.renderData->data.size());

	// See if we need to create any new particles
	int numSpawned = 0;
	int maxSpawnPerFrame = 100;
#ifndef REDUCE_REUSE_RECYCLE
	while (ps.numAlive < ps.maxParticles && numSpawned < maxSpawnPerFrame) {

		ps.particles.push_back(Particle());
		ps.numAlive++;
		numSpawned++;

		auto& p = ps.particles.back();

		p.createdOn = getTime();
		p.size = ps.particleSize * ps.sizeChange.x;
		p.physics->position = ps.position;
		p.alive = true;


		mat4 forwardDir = glm::toMat4(quaternion(glm::radians(ps.rotation)));

		mat4 randDir = glm::toMat4(quaternion(glm::radians(vec3(
			glm::linearRand<float>(-ps.maxAngle, ps.maxAngle),
			0.0f,
			glm::linearRand<float>(-ps.maxAngle, ps.maxAngle)))));

		vec3 forwardDirection = (forwardDir * randDir) * vec4(1.0f, 0.0f, 0.0f, 1.0f);

		p.physics->linearVelocity = glm::normalize(forwardDirection) * glm::linearRand<float>(ps.speedRange.x, ps.speedRange.y);
		p.physics->angularVelocity = vec3(
			glm::linearRand<float>(-ps.maxParticleAngle, ps.maxParticleAngle),
			glm::linearRand<float>(-ps.maxParticleAngle, ps.maxParticleAngle),
			glm::linearRand<float>(-ps.maxParticleAngle, ps.maxParticleAngle)
		);

		p.lifeSpan = glm::linearRand<float>(ps.lifespanRange.x, ps.lifespanRange.y);

		p.color = vec4(
			glm::linearRand<float>(0.0f, 1.0f),
			glm::linearRand<float>(0.0f, 1.0f),
			glm::linearRand<float>(0.0f, 1.0f),
			1.0f);
	}
#else
	if (ps.maxParticles != ps.particles.size()) {
		if (ps.lastAliveIndex > ps.maxParticles) {
			ps.lastAliveIndex = ps.maxParticles - 1;
		}
		ps.particles.resize(ps.maxParticles);
		if (ps.numAlive > ps.maxParticles) {
			ps.numAlive = ps.maxParticles;
		}

	}

	unsigned int numChecked = 0;
	while (ps.numAlive < ps.maxParticles && numSpawned < maxSpawnPerFrame && numChecked < ps.maxParticles) {
		bool foundAParticle = false;

		while (!foundAParticle && numChecked < ps.maxParticles) {
			numChecked++;
			auto& p2 = ps.particles[ps.lastAliveIndex];
			if (p2.alive == false) {
				foundAParticle = true;
				break;
			}
			else {
				ps.lastAliveIndex = (ps.lastAliveIndex + 1) % ps.maxParticles;
			}
		}

		if (foundAParticle) {
			numSpawned++;
			auto& p = ps.particles[ps.lastAliveIndex];
			p.createdAt = getTime();
			p.size = ps.particleSize * ps.sizeChange.x;
			p.physics.position = ps.position;
			p.alive = true;


			mat4 forwardDir = glm::toMat4(quaternion(glm::radians(ps.rotation)));

			// Option 1 - pick a random angle for X, Y, and Z. Can create a pyramid-style shape.

//#define RANDDIR1
#define RANDDIR2

#ifdef RANDDIR1
			mat4 randDir = glm::toMat4(quaternion(glm::radians(vec3(
				glm::linearRand<float>(-ps.maxAngle, ps.maxAngle),
				glm::linearRand<float>(-ps.maxAngle, ps.maxAngle),
				glm::linearRand<float>(-ps.maxAngle, ps.maxAngle)))));

			vec3 forwardDirection = (forwardDir * randDir) * vec4(1.0f, 0.0f, 0.0f, 1.0f);
#elif defined RANDDIR2
			// Deviation from the forward direction
			float swingAngle = glm::radians(glm::linearRand<float>(-ps.maxAngle, ps.maxAngle));
			// Rotation around the forwad direction
			float twistAngle = glm::linearRand<float>(-glm::pi<float>(), glm::pi<float>());

			vec3 sideAxis = forwardDir * vec4(0.f, 1.f, 0.f, 0.f);




			vec3 fwd = forwardDir * vec4(1.0f, 0.0f, 0.0f, 1.0f);

			vec3 swingAxis = glm::normalize(glm::cross(fwd, sideAxis));

			vec3 forwardDirection = glm::rotate(fwd, swingAngle, swingAxis);
			forwardDirection = glm::rotate(forwardDirection, twistAngle, fwd);
#endif

			// Option 2 - use a random angle away from the forward direction axis
			p.physics.position = ps.position;
			p.physics.rotation = vec3(0.f);
			p.physics.linearVelocity = glm::normalize(forwardDirection) * glm::linearRand<float>(ps.speedRange.x, ps.speedRange.y);
			p.physics.angularVelocity = vec3(
				glm::linearRand<float>(-ps.maxParticleAngle, ps.maxParticleAngle),
				glm::linearRand<float>(-ps.maxParticleAngle, ps.maxParticleAngle),
				glm::linearRand<float>(-ps.maxParticleAngle, ps.maxParticleAngle)
			);

			p.lifeSpan = glm::linearRand<float>(ps.lifespanRange.x, ps.lifespanRange.y);

			if (ps.randomizeColor) {
				p.color = vec4(
					glm::linearRand<float>(0.0f, 1.0f),
					glm::linearRand<float>(0.0f, 1.0f),
					glm::linearRand<float>(0.0f, 1.0f),
					1.0f);
			}
			else {
				p.color = ps.mainColor;
			}

			ps.numAlive++;
		}
	}
#endif
}

void ParticleSystemDemo::update() {
	if (!initialized) init();

	double nowish = getTime();

	float dt = physicsDt;

	for (auto& ps : particleSystems) {
		updateParticleSystem(ps, dt);
	}
}

void ParticleSystemDemo::render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow) {

	auto& jr = AnimationObjectRenderer::get();
	// Render ground
	jr.beginBatchRender(ground.shapeType, false, vec4(1.f), isShadow);

	if (!isShadow) {
		GPU::Lighting::get().bind(jr.currentMesh->shader);
		if (ground.cullFace)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
		}
		else {
			glDisable(GL_CULL_FACE);
		}
	}

	if (isShadow) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}

	ground.updateMatrix(true);
	jr.renderBatchWithOwnColor(ground, isShadow);

	quad.updateMatrix(true);

	static int currentFrame = 0;
	float xOffset = (1.0f / 25.0f) * currentFrame;

	glm::sphericalRand(1.0f);

	currentFrame = (currentFrame + 1) % 25;

	uvOffset.x = xOffset;
	uvOffset.z = (1.0f / 25.0f);

	if (!isShadow) {
		glUniform4fv(glGetUniformLocation(jr.currentMesh->shader->program, "uvOffset"),
			1, glm::value_ptr(uvOffset));
	}
	
	
	jr.renderBatchWithOwnColor(quad.transform, quad.color, fireTexture->id, isShadow);
	
	jr.endBatchRender(isShadow);

	if (objects.size() > 0) {
		jr.beginBatchRender(objects[0].shapeType, false, vec4(1.f), isShadow);

		if (!isShadow) {
			GPU::Lighting::get().bind(jr.currentMesh->shader);
			if (objects[0].cullFace)
			{
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
			}
			else {
				glDisable(GL_CULL_FACE);
			}
		}

		if (isShadow) {
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
		}

		for (auto& o : objects) {
			o.updateMatrix(true);
			jr.renderBatchWithOwnColor(o, isShadow);
		}

		jr.endBatchRender(isShadow);
	}

	auto& iqr = InstanceQuadRenderer::get();
	for (auto& ps : particleSystems) {
		if (ps.renderData == nullptr) continue;
		iqr.beginBatch(&ps, true, ps.textureID);
		iqr.render(vec4(1), uvOffset);
		iqr.endBatch();
	}
}

// Renders the UI controls for the lab.
void ParticleSystemDemo::renderUI() {

	if (ImGui::CollapsingHeader("Ground")) {
		if (ground.renderUI()) {
			// Recompute ground plane

			quaternion q = quaternion(radians(ground.rotation));
			vec3 forward = { 1, 0, 0 };
			vec3 newNormal = q * forward;

			groundPlane = vec4(newNormal, -glm::dot(newNormal, ground.localPosition));
		}
	}

	if (ImGui::CollapsingHeader("Target")) {
		objects[0].renderUI();
	}

	if (ImGui::CollapsingHeader("Objects")) {

		static int findex = 1;
		ImGui::InputInt("Object index", &findex);
		if (findex >= 0 && findex < objects.size()) {
			ImGui::Text("Object %d", findex);

			objects[findex].renderUI();
		}
	}
	
	ImGui::InputFloat("Timestep (physics)", &physicsDt);

	ImGui::InputFloat4("UV offset", glm::value_ptr(uvOffset));

	int psCount = 0;
	for (auto& ps : particleSystems) {
		std::string psLabel = fmt::format("Particle system {0}", psCount++);
		if (ImGui::CollapsingHeader(psLabel.c_str())) {
			ps.renderUI();
		}
	}

	if (ImGui::CollapsingHeader("Fire texture")) {
		fireTexture->renderUI();
	}

}

ptr_vector<AnimationObject> ParticleSystemDemo::getObjects() {
	ptr_vector<AnimationObject> object_ptrs;
	for (auto& o : objects) {
		object_ptrs.push_back(&o);
	}
	return object_ptrs;
}