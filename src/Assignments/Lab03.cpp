// CMPS 4480 Lab 03
// Name: 

#include "Lab03.h"
#include "Application.h"
#include "AnimationObjectRenderer.h"
#include "InputOutput.h"
#include "Lighting.h"
#include "UIHelpers.h"

// Variables for lab 03
namespace cmps_4480_lab_03 {
	
	// Table object parts
	AnimationObject table;
	AnimationObject tableTop;
	AnimationObject tableLegs[4];
	vec4 tableColor = vec4(0.75f, 0.6f, 0.2f, 1.f);

	// Centerpiece to go on top of the table.
	AnimationObject centerpiece;
	// Any components you add to the centerpiece should go inside the children collection and be parented to the centerpiece.
	std::vector<AnimationObject> children;
	AnimationObjectType childrenShapeType = AnimationObjectType::box;
	vec3 childrenScale(0.25f);
	vec4 childrenColor = vec4(vec3(0.5f), 1.f);
	float delay = 0.5f;

	// Variables for time progression
	int FPS = 24;					// frames per second
	double SPF = 1.0 / 24;			// seconds per frame (1/FPS)
	double lastFrameChange = 0;		// timestamp of the last frame change
	double fixedSecondsElapsed = 0;


	// Where the motion data file will be saved. This should be the root
	// of the project directory.
	std::string labDataFilepath() {
		return IO::getAssetRoot() + "/Lab03.dat";
	}
}

using namespace cmps_4480_lab_03;

void Lab03::init() {
	table = AnimationObject(AnimationObjectType::quad);
	tableTop = AnimationObject(AnimationObjectType::box, vec3(0, 0, 0), vec3(0), vec3(3, 0.25f, 3), nullptr, tableColor);

	// TODO 1: fix the legs! Try moving them around in the application first and observing the property values in the UI.
	// Then when you're happy with their configuration, take note of those values and change their properties here.
	// Note that all of the tableLeg objects are parented to the tableTop object, so they will inherit the tableTop's
	// transform!
	// The order of parameters for the AnimationObject constructor is:
	/*
	*	0: AnimationObjectType. Leave this on box for now
	*	1: position: a vec3 indicating the object's position *relative to its parent!*
	*	2: rotation: a vec3 indicating the angles of rotation (in degrees) around the X, Y, and Z axes *relative to its parent!*
	*	3: scale: a vec3 indicating the size of the object in each dimension *relative to its parent!*
	*	4: texture: leave this as a null pointer for now
	*	5: color: a vec4 indicating the red, green, blue, and alpha values for the object's color. All channels should be between 0 and 1 for color.
	*/

	// Once you've got the table looking the way you like, use this space to explain what changes you had to make:
	/*
	* 
	* 
	* 
	*/
	vec3 legSize = vec3(0.15f, 4.5f, 0.15f);
	float tableLegOffset = 1.425;
	float tableLegY = -2.25;
	tableLegs[0] = AnimationObject(AnimationObjectType::box, vec3(-tableLegOffset, tableLegY, -tableLegOffset), vec3(0), legSize, nullptr, tableColor);
	tableLegs[1] = AnimationObject(AnimationObjectType::box, vec3(tableLegOffset, tableLegY, -tableLegOffset), vec3(0), legSize, nullptr, tableColor);
	tableLegs[2] = AnimationObject(AnimationObjectType::box, vec3(tableLegOffset, tableLegY, tableLegOffset), vec3(0), legSize, nullptr, tableColor);
	tableLegs[3] = AnimationObject(AnimationObjectType::box, vec3(-tableLegOffset, tableLegY, tableLegOffset), vec3(0), legSize, nullptr, tableColor);


	// TODO 2: customize your centerpiece! This object is also parented to the tableTop.
	centerpiece = AnimationObject(AnimationObjectType::box, vec3(0, 1, 0), vec3(0, 45, 0), vec3(0.5f, 1, 0.5f), nullptr, vec4(0.2f, 0.4f, 0.6f, 1.f));

	// TODO 3: initialize some objects in the "children" vector to further customize your centerpiece. These will be parented
	// to the centerpiece object, which is itself parented to the tableTop object. For these objects, the first argument should 
	// always be the variable childrenShapeType.
	
	// One object has already been added for you as an example. You can get rid of this line and replace it with your own objects. 
	children.push_back(AnimationObject(childrenShapeType, vec3(0, 1, 0), vec3(45, 90, 0), vec3(0.5f, 0.5f, 0.5f), nullptr, vec4(1)));

	initialized = true;
}

// TODO 5: explore different orders of multiplication for constructing an object's model
// matrix.
void updateShape(AnimationObject& object, const AnimationObject * parent = nullptr) {
	mat4 modelMatrix = glm::identity<mat4>();

	mat4 T = glm::translate(object.localPosition);
	mat4 R = glm::toMat4(quaternion(glm::radians(object.localRotation)));
	mat4 S = glm::scale(object.localScale);

	modelMatrix = T * R * S;

	object.localTransform = modelMatrix;

	mat4 parentMatrix = glm::identity<mat4>();
	if (parent) {
		parentMatrix = parent->transform;
	}

	object.transform = parentMatrix * modelMatrix;
}

void Lab03::update() {
	if (!initialized) init();

	double nowish = getTime();

	double dt = nowish - lastFrameChange;

	if (dt >= SPF) {

		updateShape(table);

		// Updates the table top and legs
		updateShape(tableTop, &table);

		for (int i = 0; i < 4; i++) {
			updateShape(tableLegs[i], &table);
		}

		// Updates the centerpiece.
		updateShape(centerpiece, &table);

		for (auto& child : children) {

			// TODO 4: animate some properties of the centerpiece's children. Here are a few options that work well:
			//		a) set x, y, or z of localPosition to glm::sin(nowish). This creates periodic motion
			//		b) set x, y, or z of localScale to glm::sin(nowish). This creates growing/shrinking motion
			//		c) set x, y, or z of localRotation to the nowish time value. This creates a semi-constant rotation around that axis.
			//		d) add a small random value to the color.

			// Make any changes to the object BEFORE calling updateShape on it.

			updateShape(child, &centerpiece);
		}

		fixedSecondsElapsed += SPF;

		lastFrameChange = nowish;
	}
}

// Render the lab. This is called twice from the main renderer: once to make the shadow map, and again
// to render the scene using the shadow data.
void Lab03::render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow) {
	auto& jr = AnimationObjectRenderer::get();

	if (!isShadow) {
		GPU::Lighting::get().bind(jr.currentMesh->shader);
	}
	else {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}

	// Render the table and its legs
	jr.beginBatchRender(tableTop.shapeType, false, vec4(1.f), isShadow);
	jr.renderBatchWithOwnColor(tableTop, isShadow);
	for (int i = 0; i < 4; i++) {
		jr.renderBatchWithOwnColor(tableLegs[i], isShadow);
	}
	jr.endBatchRender(isShadow);

	// Render the centerpiece and its children
	jr.beginBatchRender(centerpiece.shapeType, false, vec4(1.f), isShadow);

	jr.renderBatchWithOwnColor(centerpiece, isShadow);

	if (!children.empty()) {

		if (childrenShapeType != centerpiece.shapeType) {
			jr.endBatchRender(isShadow);
			jr.beginBatchRender(childrenShapeType, true, childrenColor, isShadow);
		}
		for (auto& c : children) {
			jr.renderBatchWithOwnColor(c, isShadow, true);
		}
	} 

	jr.endBatchRender(isShadow);
}

// Renders the UI controls for the lab.
void Lab03::renderUI() {

	renderEnumDropDown<AnimationObjectType>("Shape of children", childrenShapeType);

	int fps = FPS;
	if (ImGui::InputInt("FPS", &fps)) {
		if (fps > 0) {
			FPS = fps;
			SPF = 1.0 / (double)FPS;
		}
	}
	ImGui::SameLine();

	ImGui::Text("SPF: %.2f", SPF);

	if (ImGui::CollapsingHeader("Table")) {
		IMDENT;
		if (ImGui::CollapsingHeader("Table anchor")) {
			table.renderUI();
		}
		if (ImGui::CollapsingHeader("TableTop")) {
			tableTop.renderUI();
		}
		for (int i = 0; i < 4; i++) {
			if (ImGui::CollapsingHeader(fmt::format("Leg {0}", i).c_str())) {
				tableLegs[i].renderUI();
			}
		}
		if (ImGui::CollapsingHeader("Centerpiece")) {
			if (ImGui::CollapsingHeader("Children")) {
				IMDENT;
				int c = 0;
				for (auto& child : children) {
					if (ImGui::CollapsingHeader(fmt::format("Child {0}", c).c_str())) {
						IMDENT;
						child.renderUI();
						IMDONT;
					}
					c++;
				}
				IMDONT;
			}
			centerpiece.renderUI();
		}
		IMDONT;
	}
	
}

ptr_vector<AnimationObject> Lab03::getObjects() {
	ptr_vector<AnimationObject> objects;

	objects.push_back(&tableTop);
	for (int i = 0; i < 4; i++) {
		objects.push_back(tableLegs + i);
	}
	objects.push_back(&centerpiece);
	for (auto& child : children) {
		objects.push_back(&child);
	}
	

	return objects;
}