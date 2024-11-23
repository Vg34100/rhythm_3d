#pragma once

// Disables warning about vec3 and Mat4d being unusable by clients of Camera dll
#pragma warning (disable : 4251)

// Disables warning about decorated name lengths being too long and truncating them
#pragma warning (disable : 4503)

#include "globals.h"

#include <stdlib.h>
#include <functional>
#include <vector>

// Arc-ball camera
class /*DLLSPEC*/ Camera// : public IJsonable, public ISerializable
{
private:
	// Internal initialization function. Defined so I can 
	// call it from various constructors
	void init(ProjectionType projType, float fov, float w, float h, float nearZ, float farZ, float dmin, float dmax,
		vec3 pos, vec3 look, vec3 up);

public:
	// Will reverse z-ordering for OpenGL when multiplied with a projection matrix
	static mat4 reverseZ;

	vec3 viewDir = vec3(0, 0, -1);
	vec3 upDir = vec3(0, 1, 0);

	// Where the camera's located
	vec3 Position = vec3(0, 0, 1);
	// What the camera's pointed at (view direction = Lookat - Position)
	vec3 Lookat = vec3(0);

	// Projection type
	ProjectionType projectionType = ProjectionType::Perspective;

	vec3 currentView = vec3(0, 0, -1);
	vec3 currentUp = vec3(0, 1, 0);

	// The spherical coordinates of our camera
	float horizontalAngle = 0.0f, verticalAngle = 0.0f, distance = 1.0f;

	mat4 totalRotation;

	// Used to create projection matrix
	float FOV = 60.0f;

	// Panning vector length is (distance * panScale)
	float panScale = 0.01f;

	vec2 nearFar = vec2(1e-3f, 1e6f);

	vec2 screenSize;

	vec4 orthoBounds;

	// For clamping camera distance value
	vec2 distanceMinMax = vec2(1e-4f, 1e4f);

	bool useAngleControls = true;

	// Flag to notify when we should update our camera
	bool changed;

	ivec4 viewport;

	mat4 projection, view, viewproj;
	mat4 inv_projection, inv_view, inv_viewproj;

	vec4 frustumPlanes[6];

	Camera();
	// Create an arcball camera based on your standard parameters
	Camera(float FOV, float width, float height, float dmin, float dmax, float nearPlane, float farPlane,
		vec3 position, vec3 lookat, vec3 up);

	// Copy constructor
	Camera(const Camera& camera);

	// Assignment operator
	Camera& operator=(const Camera& camera);
	bool operator==(const Camera& rhs);
	bool operator!=(const Camera& rhs);

	// Pan the camera. Apply the change in mouse position to the Lookat 
	//and Position vectors based on Lookat's window projection
	void pan(float dx, float dy);
	// Change the distance between Lookat and Position while preserving 
	// the view vector
	void zoom(float dz);
	// Change Position while preserving distance from Lookat
	void orbit(float dh, float dv);

	void alignToAxis(const vec3& axis);

	// Updates state after an operation is completed
	void update();

	// Refreshes viewproj, inverses, and frustum
	void quickUpdate();

	void updateFrustum();

	// Change state
	void change(bool v)
	{
		changed = v;
	}

	bool isInsideWindow(vec3 pos);

	// See if a 3D coordinate gets culled by the camera
	bool isCulled(float x, float y, float z);

	vec3 getScreenPoint(const vec3& worldPoint) const;
	vec3 getWorldPoint(const vec3& screenPoint) const;

	vec3 getUp() { return upDir; }

	void init(vec3 up);

	// Linearly interpolate between two cameras
	//static Camera linterp(Camera * start, Camera * end, double t);
	static Camera linterp(float t, const Camera& a, const Camera& b);

	/*virtual json toJSON() const;
	virtual void applyJSON(const json & ss);*/

	// Replacement functions for GLUT camera initialization
	static mat4 perspective(float fov, float aspectRatio, float znear, float zfar);
	static mat4 frustum(float left, float right, float bottom, float top, float znear, float zfar);
	static mat4 frustumVR(float left, float right, float bottom, float top, float znear, float zfar);
	static mat4 orthographic(float left, float right, float bottom, float top, float znear, float zfar);

	static mat4 lookat(vec3 eye, vec3 center, vec3 up);

	// Projects obj using the camera's current model/view/proj matrices and returns a vector in window space
	vec3 project(vec3 obj, bool flipY = true) const;

	vec3 unproject(vec3 win, bool flipY = true) const;

	vec3 getViewVector();

	//Ray getRay(const vec2 & point);

	void copyFrom(const Camera& camera, bool runUpdate = true);

	// Create a keyframe given the frame number. Set keySelected to false to key all (not recommended for mesh)
	//virtual s_ptr<Anim::Keyframe> key(int frameNumber, bool keySelected);

	//// Attempts to remove selected components of the object from the provided keyframe.
	//virtual s_ptr<Anim::Keyframe> deleteKey(s_ptr<Anim::Keyframe> keyframe);

	//// Computes an interpolated keyframe between two values
	//virtual s_ptr<Anim::Keyframe> interpKeys(float t, s_ptr<Anim::Keyframe> a, s_ptr<Anim::Keyframe> b);

	//// Function to call when animation returns to frame 0
	//virtual void animationReset();

	//// Function to call when you need to create a keyframe from a Json value
	//virtual s_ptr_vector<Anim::Keyframe> createFromJson(const json & value);

	void focusOnJoints();

	void focusOn(const vec3& newLookat);

	virtual bool saveToFile(std::ofstream& fout);
	virtual bool loadFromFile(std::ifstream& fin);

	void renderUI();
};