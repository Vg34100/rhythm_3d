// CMPS 4480 Lab 02
// Name: 

#include "Lab02.h"
#include "Application.h"
#include "AnimationObjectRenderer.h"
#include "InputOutput.h"
#include "Lighting.h"
#include "UIHelpers.h"

namespace cmps_4480_lab_02 {

	// Variables for lab 02
	// The bouncing ball
	AnimationObject ball;
	// Storage saving/loading position data for the ball on each frame.
	std::vector<vec3> framePositions(1024);
	// Current frame of playback. Value should be in [0, framePositions.size() - 1]
	size_t currentFrame = 0;
	// Boolean states: recording and playing
	bool isRecording = false;
	bool isPlaying = false;

	// Variables for giving the ball a motion trail
	bool showTrail = true;
	std::vector<AnimationObject> trail;
	int trailCount = 20;
	vec3 lastBallPosition = vec3(0);
	float minTrailGap = 0.25f;
	float trailScale = 0.75f;
	vec4 trailColor = vec4(vec3(0), 1);
	bool trailLit = false;

	// Variables for time progression
	int FPS = 24;					// frames per second
	double SPF = 1.0 / 24;			// seconds per frame (1/FPS)
	double lastFrameChange = 0;		// timestamp of the last frame change


	// Initializes the motion trail by making other sphere objects to inherit the ball's position
	void initTrail() {
		// S-S-S-Shadow trail!
		for (int i = 0; i < trailCount; i++) {
			float si = trailScale * (1.0f - ((float)i / (trailCount - 1)));
			AnimationObject ao(AnimationObjectType::sphere, vec3(0), vec3(0), vec3(si), nullptr, trailColor);
			ao.selectable = false;
			trail.push_back(ao);
		}
	}

	// Where the motion data file will be saved. This should be the root
	// of the project directory.
	std::string labDataFilepath() {
		return IO::getAssetRoot() + "/lab02.dat";
	}
}

using namespace cmps_4480_lab_02;

void Lab02::init() {
	ball = AnimationObject(AnimationObjectType::sphere);

	initTrail();

	initialized = true;
}

void Lab02::update() {
	if (!initialized) init();

	int framesToAdvance = 0;

	// Update current frame	
	if (isRecording || isPlaying) {
		auto nowish = getTime();
		double dt = nowish - lastFrameChange;
		if (dt >= SPF) {

			framesToAdvance = glm::round(dt / SPF);
			framesToAdvance = glm::clamp<int>(framesToAdvance, 1, 20);
			size_t nextFrame = currentFrame + framesToAdvance;

			log("Current frame: {0}. Advancing by {1}\n", currentFrame, framesToAdvance);

			// Stop recording when we reach the end of frames
			if (isRecording && nextFrame >= framePositions.size()) {
				log("Done recording!");
				isRecording = false;
			}

			// Advance the frame. For playback, loop it back around to the beginning.
			currentFrame = nextFrame % framePositions.size();
			
			lastFrameChange = nowish;	
		}
	}

	// When recording: save ball's current position to the current frame
	if (isRecording) {
		framePositions[currentFrame] = ball.localPosition;

		// Need to interpolate between these frames and last frames 
		if (framesToAdvance > 1) {
			size_t oldFrame = currentFrame - framesToAdvance;
			if (oldFrame > framePositions.size()) oldFrame = framePositions.size() - framesToAdvance;
			vec3 oldPos = framePositions[oldFrame];
			vec3 newPos = ball.localPosition;

			for (size_t i = 1; i < framesToAdvance; i++) {
				float t = (float)i / framesToAdvance;
				framePositions[oldFrame + i] = glm::mix(oldPos, newPos, t);
			}
		}
	}
	// When playing: load ball's current position from the current frame
	else if (isPlaying && !framePositions.empty()) {
		ball.localPosition = framePositions[currentFrame];
	}

	// Keep the motion trail up to date by copying positions and
	// possibly resizing the collection.
	if (showTrail) {

		if (trailCount > 0 && trail.size() != (size_t)trailCount) {
			initTrail();
			trail.resize(trailCount);
		}

		trail[0].localPosition = lastBallPosition;

		float dist = glm::length(lastBallPosition - ball.localPosition);

		if (dist > minTrailGap) {
			lastBallPosition = ball.localPosition;
		}

		for (size_t i = trail.size() - 1; i > 0; i--) {
			auto& ti = trail[i];
			auto& tj = trail[i - 1];
			ti.localPosition = tj.localPosition;
		}
	}
}

// Render the lab. This is called twice from the main renderer: once to make the shadow map, and again
// to render the scene using the shadow data.
void Lab02::render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow) {
	auto& jr = AnimationObjectRenderer::get();
	jr.beginBatchRender(AnimationObjectType::sphere, false, vec4(1.f), isShadow);

	if (!isShadow) {
		GPU::Lighting::get().bind(jr.currentMesh->shader);
	}

	if (isShadow) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}

	// Render ball
	ball.updateMatrix(true);
	jr.renderBatchWithOwnColor(ball, isShadow);

	// and maybe trail
	if (showTrail) {
		for (auto& t : trail) {
			t.color = trailColor;
			t.useLighting = trailLit;
			t.updateMatrix(true);
			jr.renderBatchWithOwnColor(t, isShadow);
		}
	}
	
	jr.endBatchRender(isShadow);
}

// Renders the UI controls for the lab.
void Lab02::renderUI() {
	if (!isRecording) {
		if (ImGui::Button("Record"))
		{
			isRecording = true;
			lastFrameChange = getTime();
		}
	}
	else if (ImGui::Button("Stop record")) {
		isRecording = false;
	}

	ImGui::SameLine();

	if (!isPlaying) {
		if (ImGui::Button("Play"))
		{
			isPlaying = true;
			lastFrameChange = getTime();
		}
	}
	else if (ImGui::Button("Stop play")) {
		isPlaying = false;
	}

	ImGui::SameLine();

	auto filePath = labDataFilepath();

	if (ImGui::Button("Save")) {

		std::ofstream fout(filePath, std::ios::out | std::ios::binary);

		for (auto& frame : framePositions) {
			fout.write((const char*)&frame, sizeof(float) * 3);
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Load")) {
		std::ifstream fin(filePath, std::ios::in | std::ios::binary);
		if (!fin) {
			log("Error: {0} not found\n", filePath);
		}
		else {
			for (int i = 0; i < framePositions.size(); i++) {
				vec3* location = framePositions.data() + i;
				fin.read((char*)location, sizeof(float) * 3);
			}
		}
	}

	// Resets the motion path. 
	if (ImGui::Button("Reset")) {
		framePositions.assign(framePositions.size(), vec3(0));
		currentFrame = 0;
	}

	// Slider for playback
	int pf = (int)currentFrame;
	if (ImGui::SliderInt("Frame position", &pf, 0, (int)framePositions.size() - 1)) {
		currentFrame = (size_t)pf;
		ball.localPosition = framePositions[currentFrame];
	}

	int fps = FPS;
	if (ImGui::InputInt("FPS", &fps)) {
		if (fps > 0) {
			FPS = fps;
			SPF = 1.0 / (double)FPS;
		}
	}
	ImGui::SameLine();

	ImGui::Text("SPF: %.2f", SPF);

	ImGui::Checkbox("Show trail", &showTrail);

	if (showTrail) {
		IMDENT;
		int ts = trailCount;
		if (ImGui::InputInt("Trail size", &ts)) {
			if (ts > 0) trailCount = ts;
		}
		ImGui::InputFloat("Min gap", &minTrailGap);
		if (ImGui::InputFloat("Trail scale", &trailScale)) {
			if (trailScale >= 0) {
				initTrail();
			}
		}

		ImGui::ColorEdit4("Trail color", glm::value_ptr(trailColor));
		ImGui::Checkbox("Trail lighting", &trailLit);
		IMDONT;
	}

	if (ImGui::CollapsingHeader("Ball")) {
		ball.renderUI();
	}
}

ptr_vector<AnimationObject> Lab02::getObjects() {
	ptr_vector<AnimationObject> objects;

	objects.push_back(&ball);
	for (auto& t : trail) {
		objects.push_back(&t);
	}
	return objects;
}