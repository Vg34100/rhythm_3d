// CMPS 4480 Procedural demos
// Name: 

#include "ProceduralDemos.h"
#include "Application.h"
#include "AnimationObjectRenderer.h"
#include "GLTFImporter.h"
#include "InputOutput.h"
#include "Input.h"
#include "Lighting.h"
#include "LineRenderer.h"
#include "Prompts.h"
#include "UIHelpers.h"
#include "Textures.h"

#include <cassert>

#include <glm/gtc/random.hpp>

#include "implot.h"


namespace cmps_4480_procedural_demos {

	// Variables for time and frame progression
	int numFrames = 120;
	int currentFrame = 0;
	int FPS = 24;					// frames per second
	double SPF = 1.0 / 24;			// seconds per frame (1/FPS)
	double lastFrameChange = 0;		// timestamp of the last frame change
	double fixedSecondsElapsed = 0;

	bool isFollowing = false;

	float maxSpeed = 6.0f;

	AnimationObject ground;
	vec4 groundPlane;

	std::vector<AnimationObject> objects;

	struct PhysicsObject {
		vec3 position = vec3(0);
		vec3 velocity = vec3(0);
		vec3 acceleration = vec3(0);
		vec3 force = vec3(0);

		float mass = 1.0f;
		float dampening = 1.0f;

		bool kinematic = false;

		void renderUI() {
			ImGui::PushID((const void*)this);
			ImGui::Checkbox("Kinematic?", &kinematic);
			ImGui::InputFloat3("Force", glm::value_ptr(force));
			ImGui::InputFloat3("Acceleration", glm::value_ptr(acceleration));
			ImGui::InputFloat3("Velocity", glm::value_ptr(velocity));
			ImGui::InputFloat3("Position", glm::value_ptr(position));
			ImGui::InputFloat("Mass", &mass);
			ImGui::InputFloat("Dampening", &dampening);
			ImGui::PopID();
		}

	};

	struct SpringForce {
		PhysicsObject* target = nullptr;
		PhysicsObject* anchor = nullptr;
		vec3 anchorPos = vec3(0);
		vec3 defaultDisplacement = vec3(0);
		float defaultDistance = 0.0f;
		float stiffness = 1.0f;

		void renderUI() {
			ImGui::PushID((const void*)this);
			if (anchor) {
				ImGui::InputFloat3("Anchor position", glm::value_ptr(anchor->position));
			}
			else {
				ImGui::InputFloat3("Anchor position", glm::value_ptr(anchorPos));
			}
			
			ImGui::InputFloat3("Default displacement", glm::value_ptr(defaultDisplacement));
			ImGui::InputFloat("Default distance", &defaultDistance);
			ImGui::InputFloat("Stiffness", &stiffness);
			ImGui::PopID();
		}
	};

	std::vector<PhysicsObject> physics;
	std::vector<SpringForce> springs;

	float globalStiffness = 1.0f;
	float globalDampening = 1.0f;
	bool showLines = true;
	bool limitY = true;
	bool keepOnGround = true;

	bool gravity = false;
	float gravityForce = -9.8f;

	float physicsDt = 1.0f / 60.0f;

	bool simulating = true;
}

using namespace cmps_4480_procedural_demos;

void initWheel() {

	AnimationObject target = AnimationObject(AnimationObjectType::sphere);
	target.color = vec4(1.0f, 0.f, 1.f, 1.f);
	target.localScale = vec3(0.5f);

	objects.push_back(target);

	int numFollowers = 32;

	//vec3 lastFollowerPos = vec3(-1, 0, 0);

	vec3 lastColor = vec3(
		glm::linearRand<float>(0, 1),
		glm::linearRand<float>(0, 1),
		glm::linearRand<float>(0, 1)
	);

	float angle = glm::two_pi<float>() / numFollowers;
	float radius = 4.0f;



	for (int i = 0; i < numFollowers; i++) {
		float theta = i * angle;
		float x = radius * glm::cos(theta);
		float z = radius * glm::sin(theta);

		float t = (float)(i) / (numFollowers - 1);
		AnimationObject f = AnimationObject(AnimationObjectType::sphere);
		f.color = vec4(glm::mix(vec3(target.color), lastColor, t), 1.0f);
		f.localPosition = vec3(x, 0, z);
		f.localScale = vec3(0.5f);
		objects.push_back(f);
	}

	objects[0].color = vec4(1.0f);

	// Initialize physics objects and create some spring forces along the way
	physics.resize(objects.size(), PhysicsObject());
	physics[0].kinematic = true;

	// Create a series of springs that connect the outer objects to one another
	// e.g. 1 to 2, 2 to 3, ... n - 1 to n, and n to 1. 

	for (int i = 1; i < objects.size(); i++) {
		AnimationObject& o = objects[i];
		PhysicsObject& po = physics[i];
		po.position = o.localPosition;

		SpringForce sf;

		if (i - 1 > 0) {

			sf.target = &po;
			sf.anchor = &physics[i - 1];
			sf.defaultDisplacement = po.position - sf.anchor->position;
			sf.defaultDistance = glm::length(sf.defaultDisplacement);
			sf.stiffness = 1.0f;

			springs.push_back(sf);
		}

		// Create another spring that connects the ith object to the central kinematic object
		// located at index 0.
		sf.target = &po;
		sf.anchor = &physics[0];
		sf.defaultDisplacement = po.position - physics[0].position;
		sf.defaultDistance = glm::length(sf.defaultDisplacement);
		sf.stiffness = 1.0f;
		springs.push_back(sf);
	}



	SpringForce ee;
	ee.target = &physics[physics.size() - 1];
	ee.anchor = &physics[1];
	ee.defaultDisplacement = physics[physics.size() - 1].position - physics[1].position;
	ee.defaultDistance = glm::length(ee.defaultDisplacement);
	ee.stiffness = 1.0f;
	springs.push_back(ee);

	// 32 followers, 32 C 2 possible pairs = 496
	//int numRandom = 100;

	//std::unordered_set<ivec2> randomPairs;

	//for (int i = 0; i < numRandom; i++) {
	//	ivec2 indices = ivec2(0);
	//	while (indices.x == indices.y || randomPairs.find(indices) != randomPairs.end()) {
	//		indices.x = glm::linearRand<int>(1, physics.size() - 1);
	//		indices.y = glm::linearRand<int>(1, physics.size() - 1);
	//		indices.x = glm::min(indices.x, indices.y);
	//		indices.y = glm::max(indices.x, indices.y);
	//	}
	//	randomPairs.insert(indices);
	//	

	//	SpringForce rf;
	//	rf.target = &physics[indices.x];
	//	rf.anchor = &physics[indices.y];
	//	rf.defaultDisplacement = rf.target->position - rf.anchor->position;
	//	rf.defaultDistance = glm::length(rf.defaultDisplacement);
	//	rf.stiffness = 1.0f;
	//	springs.push_back(rf);
	//}

	// Maximum connectivity
	for (int i = 1; i < physics.size(); i++) {
		for (int j = i + 2; j < physics.size(); j++) {
			ivec2 indices = { i, j };

			SpringForce rf;
			rf.target = &physics[indices.x];
			rf.anchor = &physics[indices.y];
			rf.defaultDisplacement = rf.target->position - rf.anchor->position;
			rf.defaultDistance = glm::length(rf.defaultDisplacement);
			rf.stiffness = 1.0f;
			springs.push_back(rf);
		}
	}
}


void initCube() {

	keepOnGround = false;

	vec3 offset = vec3(0, 4, 0);

	AnimationObject target = AnimationObject(AnimationObjectType::sphere);
	target.color = vec4(1.0f, 0.f, 1.f, 1.f);
	target.localScale = vec3(0.5f);
	target.localPosition = offset;

	objects.push_back(target);

	


	vec3 corners[8];

	for (int i = 0; i < 8; i++) {
		// 000 to 111
		int x = (i & 1);
		int y = (i & 2) >> 1;
		int z = (i & 4) >> 2;

		vec3 ci = { x, y, z };

		if (x == 0) x = -1;
		if (y == 0) y = -1;
		if (z == 0) z = -1;

		corners[i] = offset + vec3(x, y, z);

		AnimationObject oi = AnimationObject(AnimationObjectType::sphere, corners[i], vec3(0), vec3(0.5f), nullptr, vec4(ci, 1.0f));

		objects.push_back(oi);
	}

	physics.resize(objects.size(), PhysicsObject());

	for (int i = 0; i < objects.size(); i++) {
		auto& p = physics[i];
		p.position = objects[i].localPosition;
		//p.kinematic = true;
	}

	//physics[0].kinematic = true;

	for (int i = 0; i < objects.size(); i++) {
		for (int j = i + 1; j < objects.size(); j++) {
			SpringForce sf;
			sf.target = &physics[j];
			sf.anchor = &physics[i];
			sf.defaultDisplacement = sf.target->position - sf.anchor->position;
			sf.defaultDistance = glm::length(sf.defaultDisplacement);
			sf.stiffness = 1.0f;
			springs.push_back(sf);

			log("Connecting object {0} to {1}\n", i, j);
		}
	}
	
}

void initCloth(int gridSize) {
	keepOnGround = false;
	limitY = false;

	float space = 5.0f;
	float left = space * gridSize / -2.0f;
	float top = gridSize * space;
	

	for (int x = 0; x < gridSize; x++) {
		float px = left + (x * space);
		for (int y = 0; y < gridSize; y++) {
			float py = top - (y * space);

			vec3 p = { px, py, 0 };

			objects.push_back(AnimationObject(AnimationObjectType::sphere, p));
			physics.push_back(PhysicsObject());
			physics.back().position = p;
		}
	}

	auto getIndex = [gridSize](int x, int y) {
		return (x * gridSize) + y;
	};

	for (int x = 0; x < gridSize; x++) {
		for (int y = 0; y < gridSize; y++) {
			int i = getIndex( x, y);

			auto& p = physics[i];

			// Make springs with its right (x + 1) and bottom (y + 1) neighbor
			int right = getIndex(x + 1, y);
			if (right < physics.size() && x + 1 < gridSize) {
				SpringForce sf;
				sf.target = &p;
				sf.anchor = &physics[right];
				sf.defaultDisplacement = p.position - physics[right].position;
				sf.defaultDistance = glm::length(sf.defaultDisplacement);
				springs.push_back(sf);
			}
			int bottom = getIndex(x, y + 1);
			if (bottom < physics.size() && y + 1 < gridSize) {
				SpringForce sf;
				sf.target = &p;
				sf.anchor = &physics[bottom];
				sf.defaultDisplacement = p.position - physics[bottom].position;
				sf.defaultDistance = glm::length(sf.defaultDisplacement);
				springs.push_back(sf);
			}

		}
	}

	// Make spring forces connecting each corner to the other
	/*std::vector<int> cornerIndices = {
		getIndex(0, 0), getIndex(gridSize - 1, 0), getIndex(gridSize - 1, gridSize - 1), getIndex(0, gridSize - 1)
	};

	for (int i = 0; i < cornerIndices.size(); i++) {
		int ci = cornerIndices[i];
		objects[ci].color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
		auto & pi = physics[ci];
		for (int j = i+1; j < cornerIndices.size(); j++) {
			int cj = cornerIndices[j];
			auto & pj = physics[cj];

			SpringForce sf;
			sf.target = &pi;
			sf.anchor = &pj;
			sf.defaultDisplacement = sf.target->position - sf.anchor->position;
			sf.defaultDistance = glm::length(sf.defaultDisplacement);
			springs.push_back(sf);
		}
	}*/
}


void initSolidCube(int gridSize) {
	keepOnGround = false;
	limitY = true;

	float space = 5.0f;
	float left = space * gridSize / -2.0f;
	float top = gridSize * space;
	float front = space * gridSize / -2.0f;


	for (int x = 0; x < gridSize; x++) {
		float px = left + (x * space);
		for (int y = 0; y < gridSize; y++) {
			float py = top - (y * space);

			for (int z = 0; z < gridSize; z++) {
				float pz = front - (z * space);

				vec3 p = { px, py, pz };

				objects.push_back(AnimationObject(AnimationObjectType::sphere, p));
				physics.push_back(PhysicsObject());
				physics.back().position = p;
				//physics.back().kinematic = true;
			}
		}
	}
}

void initTwoSpheres() {
	keepOnGround = false;
	limitY = true;
	gravity = true;

	vec3 p1 = { -4, 4, 0 };
	vec3 p2 = { 4, 4, 0 };

	objects.push_back(AnimationObject(AnimationObjectType::sphere, p1));
	physics.push_back(PhysicsObject());
	physics.back().position = p1;
	physics.back().velocity = { 1, 0, 0 };

	objects.push_back(AnimationObject(AnimationObjectType::sphere, p2));
	physics.push_back(PhysicsObject());
	physics.back().position = p2;
	physics.back().velocity = { -1, 0, 0 };
}

std::vector<vec3> initialPositions;

void initPablosphere() {
	keepOnGround = false;
	// Golden ratio
	float phi = (1.0f + sqrt(5.0f)) / 2.0f;
	float scale = 2.0f;

	// Dodecahedron
	std::vector<vec3> positions = {
		vec3(1, 1, 1),           vec3(-1, 1, 1),          vec3(1, -1, 1),
		vec3(-1, -1, 1),         vec3(1, 1, -1),          vec3(-1, 1, -1),
		vec3(1, -1, -1),         vec3(-1, -1, -1),
		vec3(0, phi, 1 / phi),     vec3(0, -phi, 1 / phi),    vec3(0, phi, -1 / phi),
		vec3(0, -phi, -1 / phi),   vec3(1 / phi, 0, phi),     vec3(-1 / phi, 0, phi),
		vec3(1 / phi, 0, -phi),    vec3(-1 / phi, 0, -phi),   vec3(phi, 1 / phi, 0),
		vec3(-phi, 1 / phi, 0),    vec3(phi, -1 / phi, 0),    vec3(-phi, -1 / phi, 0)
	};

	for (auto& p : positions) {
		p = (p * scale) + vec3(0, 9, 0);
	}

	// Store initial positions for reset
	initialPositions = positions;

	// Add all vertices as objects
	for (const auto& pos : positions) {
		AnimationObject vertex(AnimationObjectType::sphere);
		vertex.color = vec4(1.0f, 0.5f, 0.0f, 1.0f);
		vertex.localPosition = pos;
		//vertex.localScale = vec3(0.3f);
		objects.push_back(vertex);
	}

	objects[0].color = vec4(1.0f, 0.0f, 0.0f, 1.0f);

	// Initialize physics objects
	physics.resize(objects.size(), PhysicsObject());

	// Setup physics objects
	for (int i = 0; i < objects.size(); i++) {
		AnimationObject& o = objects[i];
		PhysicsObject& po = physics[i];
		po.position = o.localPosition;
		po.mass = 1.0f;
	}

	// Function to add a spring between two vertices
	auto addSpring = [&](int i, int j) {
		SpringForce sf;
		sf.target = &physics[j];
		sf.anchor = &physics[i];
		sf.defaultDisplacement = physics[j].position - physics[i].position;
		sf.defaultDistance = glm::length(sf.defaultDisplacement);
		sf.stiffness = 0.5f;
		springs.push_back(sf);
		};

	// Add springs to connect vertices (simplified connections)
	for (int i = 0; i < positions.size(); i++) {
		for (int j = i + 1; j < positions.size(); j++) {
			// Connect vertices if they're close enough
			float dist = glm::length(positions[i] - positions[j]);
			if (dist < 2.5f) {
				addSpring(i, j);
			}
		}
	}

	// Adjust physics parameters for stability
	globalStiffness = 0.3f;
	physicsDt = 1.0f / 120.0f;
}

void ProceduralDemos::init() {

	ground = AnimationObject(AnimationObjectType::quad, vec3(0.f, 0.0f, 0.f), vec3(0.f, 0.f, 90.f), vec3(250), nullptr, vec4(vec3(0.5f), 1.f));
	ground.cullFace = true;

	vec3 p = ground.localPosition;
	vec3 n = vec3(0, 1, 0);

	groundPlane = vec4(n, -glm::dot(n, p));

	//initWheel();
	//initCube();
	//initCloth(10);
	//initSolidCube(10);
	initTwoSpheres();
	//initPablosphere();
	
	initialized = true;
}

void ProceduralDemos::update() {
	if (!initialized) init();

	double nowish = getTime();

	//float dt = (float)(nowish - lastFrameChange);
	float dt = physicsDt;
	lastFrameChange = nowish;

	/*if (isPlaying) {
		if (dt >= SPF) {
			int framesToAdvance = glm::round(dt / SPF);
			framesToAdvance = glm::clamp<int>(framesToAdvance, 1, 20);
			fixedSecondsElapsed += SPF * framesToAdvance;
			currentFrame += framesToAdvance;

			if (currentFrame > numFrames) {
				currentFrame = 0;
			}

			lastFrameChange = nowish;
		}
	}*/


	float currentTime = (float)(currentFrame * SPF);

	auto & input = Input::get();
	if (input.current.keyStates[GLFW_KEY_K]) {
		for (int i = 0; i < objects.size(); i++) {
			if (objects[i].selected) {
				physics[i].kinematic = !physics[i].kinematic;
			}
		}
	}

	if (!simulating) return;

	// Reset and recompute all forces each frame
	for (auto& po : physics) {

		po.force = vec3(0);

		if (gravity) {
			po.force += vec3(0, gravityForce, 0);
		}
	}

	// Update any selected moved positions
	for (int i = 0; i < physics.size(); i++) {
		auto& o = objects[i];
		auto& p = physics[i];
		if (o.selected) {
			p.position = o.localPosition;
		}
	}

	for (auto& sf : springs) {
		// Get current difference 
		vec3 currentDisplacement = vec3(0);
		if (sf.anchor) {
			currentDisplacement = sf.target->position - sf.anchor->position;
		}
		else {
			currentDisplacement = sf.target->position - sf.anchorPos;
		}
		
		float currentDistance = glm::length(currentDisplacement);

		float x = (currentDistance - sf.defaultDistance);

		// F = -kx

		vec3 springForce = (-sf.stiffness * globalStiffness * x) * glm::normalize(currentDisplacement);
		// Apply spring force to both
		sf.target->force += springForce;

		if (sf.anchor) {
			sf.anchor->force += -springForce;
		}
	}

	

	// Now apply physics
	//for (auto& po : physics) {
	for(int i = 0; i < physics.size(); i++) {

		auto& o = objects[i];
		auto& p = physics[i];

		// Update physics based on forces
		if (!p.kinematic) {
			p.acceleration = p.force * (1.0f / p.mass);
			p.velocity = p.velocity * p.dampening * globalDampening + dt * p.acceleration;
			p.position = p.position + dt * p.velocity;
			// Keep objects on ground
			if (keepOnGround || (limitY && p.position.y < 0)) {
				p.position.y = 0;
			}

			//p.position.z = 0;

			o.localPosition = p.position;
		}
		else {
			// Kinematic objects don't move under forces. 
			if (keepOnGround || (limitY && p.position.y < 0)) {
				o.localPosition.y = 0;
			}
			
			p.position = o.localPosition;
		}
		
		o.updateMatrix(true);
	}

	for (int i = 0; i < physics.size(); i++) {
		auto& oi = objects[i];
		if (oi.shapeType != +AnimationObjectType::sphere) continue;
		auto& pi = physics[i];
		float ri = oi.size;
		float ri2 = ri * ri;

		// Sphere-plane intersection test: need to check sphere radius vs. distance to plane via plane equation
		{
			vec3 planeNormal = vec3(groundPlane);
			float distFromPlane = glm::dot(planeNormal, pi.position) + groundPlane.w;

			if (glm::abs(distFromPlane) < ri) {
				float depth = ri - glm::abs(distFromPlane);

				if (i == 0) {
					log("Distance: {0}. Depth: {1}\n", distFromPlane, depth);
				}

				// Step 1: correct for position
				pi.position += planeNormal * (depth + 1e-4f);

				vec3 newVelocityI = glm::reflect(pi.velocity, planeNormal);
				pi.velocity = newVelocityI;
			}
		}


		// Sphere-sphere intersection test
		for (int j = i+1; j < physics.size(); j++) {
			if (i == j) continue;

			auto& oj = objects[j];

			if (oj.shapeType != +AnimationObjectType::sphere) continue;
			auto& pj = physics[j];
			float rj = oj.size;
			float rj2 = rj * rj;

			// Sphere-sphere intersection test
			vec3 d = pi.position - pj.position;

			float dl2 = glm::length2(d);
			float dl = glm::length(d);

			float r2 = ri2 + rj2;

			float r = ri + rj;

			if (dl <= r) {
				//log("{0} to {1} Bonk!\n", i, j);

				float depth2 = r2 - dl2;
				float depth = glm::sqrt(depth2);
				depth = r - dl;

				vec3 dn = d * (1.0f / depth2);

				vec3 intersectionPoint = pj.position + dn * depth;
				vec3 intersectionNormal = glm::normalize(intersectionPoint - pj.position);

				pi.position += intersectionNormal * depth;
				pj.position += -intersectionNormal * depth;

				float newVIMagnitude = glm::length(pi.velocity) * 0.9f;
				vec3 vi = glm::reflect(glm::normalize(pi.velocity), intersectionNormal) * newVIMagnitude;

				float newVJMagnitude = glm::length(pj.velocity) * 0.9f;
				vec3 vj = glm::reflect(glm::normalize(pj.velocity), -intersectionNormal) * newVJMagnitude;

				pi.velocity = vi;
				pj.velocity = vj;

				oi.localPosition = pi.position;
				oj.localPosition = pj.position;

				oi.updateMatrix(true);
				oj.updateMatrix(true);
			}
		}
	}

}

void ProceduralDemos::render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow) {

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
	jr.endBatchRender(isShadow);

	// Render target
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

	// Render lines between springs


	if (!isShadow && showLines) {
		LineRenderer& lr = LineRenderer::get();

		lr.begin();
		for (auto& spring : springs) {
			float lc = glm::length(spring.target->position - spring.anchor->position);
			float ld = spring.defaultDistance;
			float colorWeight = glm::clamp(glm::abs(lc - ld) / ld, 0.f, 1.f);
			vec4 color = glm::mix(vec4(1.f), vec4(1.0f, 0.0f, 0.0f, 1.0f), colorWeight);
			lr.renderSingle(spring.target->position, spring.anchor->position, color, 1.0f);
		}
		lr.end();
	}	
}

// Renders the UI controls for the lab.
void ProceduralDemos::renderUI() {

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
			ImGui::Separator();
			physics[findex].renderUI();
		}
	}

	ImGui::Checkbox("Simulating physics", &simulating);
	ImGui::InputFloat("Max speed", &maxSpeed);
	ImGui::InputFloat("Global stiffness", &globalStiffness);
	ImGui::InputFloat("Global dampening", &globalDampening);
	ImGui::Checkbox("Show spring lines", &showLines);
	ImGui::Checkbox("Keep on ground", &keepOnGround);
	ImGui::Checkbox("No negative y", &limitY);
	ImGui::InputFloat("Timestep (physics)", &physicsDt);

	ImGui::Checkbox("Gravity", &gravity);
	if (gravity) {
		ImGui::InputFloat("Gravity force", &gravityForce);
	}
}

ptr_vector<AnimationObject> ProceduralDemos::getObjects() {
	ptr_vector<AnimationObject> object_ptrs;
	for (auto& o : objects) {
		object_ptrs.push_back(&o);
	}
	return object_ptrs;
}