#include "Camera.h"

#include "Application.h"
#include "Renderer.h"

#include "imgui.h"
#include "UIHelpers.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define CAMERA_EPS 1e-5

mat4 Camera::reverseZ = mat4(1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, -1.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 0.0f);

Camera::Camera()
{
	screenSize = vec2(800, 600);
	orthoBounds = vec4(0, screenSize.x, 0, screenSize.y);
	init(vec3(0, 1, 0));
}

Camera::Camera(float FOV, float width, float height, float nearPlane, float farPlane,
	float dmin, float dmax, vec3 position, vec3 lookat, vec3 up)
{
	//auto res = Settings::get().rendering.screenResolution.value;
	auto res = Application::get().renderer->resolution;
	screenSize = vec2(res.x, res.y);
	auto smallRes = screenSize * 0.125f;
	orthoBounds = vec4(-smallRes.x, smallRes.x, -smallRes.y, smallRes.y);

	init(ProjectionType::Perspective, FOV, width, height, nearPlane, farPlane, dmin, dmax, position, lookat, up);
}

Camera::Camera(const Camera& camera)
{
	copyFrom(camera);
}

bool Camera::operator==(const Camera& rhs)
{
	return (Position == rhs.Position)
		&& (Lookat == rhs.Lookat)
		&& (projectionType == rhs.projectionType)
		&& (currentView == rhs.currentView)
		&& (currentUp == rhs.currentUp)
		&& (fabs(FOV - rhs.FOV) < 1e-5)
		&& (nearFar == rhs.nearFar)
		&& (screenSize == rhs.screenSize)
		&& (distanceMinMax == rhs.distanceMinMax);
}

bool Camera::operator!=(const Camera& rhs)
{
	return !(*this == rhs);
}

Camera& Camera::operator=(const Camera& camera)
{
	if (this != &camera)
	{
		copyFrom(camera);
	}

	return *this;
}

void Camera::copyFrom(const Camera& camera, bool runUpdate)
{
	viewDir = camera.viewDir;
	upDir = camera.upDir;

	Position = camera.Position;
	Lookat = camera.Lookat;
	projectionType = camera.projectionType;
	/*currentView = camera.currentView;
	currentUp = camera.currentUp;*/
	horizontalAngle = camera.horizontalAngle;
	verticalAngle = camera.verticalAngle;
	distance = camera.distance;
	totalRotation = camera.totalRotation;
	FOV = camera.FOV;
	nearFar = camera.nearFar;
	screenSize = camera.screenSize;

	distanceMinMax = camera.distanceMinMax;

	projection = camera.projection;
	view = camera.view;
	viewproj = camera.viewproj;

	inv_view = camera.inv_view;
	inv_projection = camera.inv_projection;
	inv_viewproj = camera.inv_viewproj;

	viewport = camera.viewport;
	screenSize = camera.screenSize;
	orthoBounds = camera.orthoBounds;
	memcpy(frustumPlanes, camera.frustumPlanes, sizeof(frustumPlanes));

	if (runUpdate)
	{
		change(true);
		update();
	}
	else
	{
		change(false);
	}
}

void Camera::init(vec3 up)
{
	distance = glm::length(Position - Lookat);
	horizontalAngle = 0;
	verticalAngle = 0;

	// Set modelview to lookat. We have position and lookat, just need up
	// up = right x forward
	// forward = (lookat - position)
	// right = forward x reference

	vec3 lookatDir = Lookat - Position;
	lookatDir = glm::normalize(lookatDir);

	vec3 referenceDir = vec3(0, 1, 0);

	if (lookatDir.y == referenceDir.y || lookatDir.y == -referenceDir.y)
	{
		referenceDir = vec3(0, 0, 1);
	}

	vec3 right = glm::cross(lookatDir, referenceDir);
	right = glm::normalize(right);

	vec3 upvec;

	if (glm::length(up) == 0)
	{
		upvec = glm::cross(right, lookatDir);
	}
	else
	{
		upvec = up;
	}

	upvec = glm::normalize(upvec);

	viewDir = lookatDir;
	upDir = upvec;

	change(true);
	update();
}

void Camera::init(ProjectionType projType, float fov, float w, float h, float nearZ, float farZ, float dmin, float dmax,
	vec3 pos, vec3 look, vec3 up = vec3(0))
{
	Position = pos;
	Lookat = look;
	upDir = up;
	FOV = fov;
	screenSize = vec2(w, h);
	nearFar = vec2(nearZ, farZ);
	projectionType = projType;
	distanceMinMax = vec2(dmin, dmax);

	init(up);
}


void Camera::update()
{
	if (!changed) return;

	//while (horizontalAngle < 0)
	//{
	//	horizontalAngle += (float)glm::two_pi<float>();
	//}
	//while (horizontalAngle > (float)glm::two_pi<float>())
	//{
	//	horizontalAngle -= (float)glm::two_pi<float>();
	//}

	//while (verticalAngle < 0)
	//{
	//	verticalAngle += (float)glm::two_pi<float>();
	//}
	//while (verticalAngle > (float)glm::two_pi<float>())
	//{
	//	verticalAngle -= (float)glm::two_pi<float>();
	//}

	distance = glm::clamp(distance, distanceMinMax.x + FLT_EPSILON, distanceMinMax.y - FLT_EPSILON);

	// Recompute FOV
	// FOV = FOV * newHeight / oldHeight
	//if (auto win = Aestus::getWindow())
	//if (auto renderer = Aestus::get().renderers.active)
	if (auto renderer = Application::get().renderer)
	{
		auto winSize = renderer->resolution;
		if (winSize.x > 0 || winSize.y > 0)
		{
			FOV = FOV * (float)winSize.y / screenSize.y;
			screenSize = vec2(winSize.x, winSize.y);
		}
	}

	//glViewport(0, 0, (GLsizei)screenSize.x, (GLsizei)screenSize.y);

	glGetIntegerv(GL_VIEWPORT, glm::value_ptr(viewport));

	if (projectionType == +ProjectionType::Perspective)
	{
		projection = perspective(FOV, screenSize.x / screenSize.y, nearFar.x, nearFar.y);
	}
	else if (projectionType == +ProjectionType::Orthographic)
	{
		float aspectRatio = screenSize.x / screenSize.y;

		float r_fov = glm::radians(FOV);
		float r_vfov = r_fov / aspectRatio;

		float d = glm::length(Lookat - Position);

		// Distances from the center to the left and right vertical planes
		float x = tan(r_fov * 0.5f) * d;
		float y = tan(r_vfov * 0.5f) * d;

		float left = Lookat.x - x;
		float right = Lookat.x + x;

		float top = Lookat.y + y;
		float bot = Lookat.y - y;

		//projection = orthographic((float)left, (float)right, (float)bot, (float)top, (float)nearPlane, (float)farPlane);
		projection = orthographic(-x, x, -y, y, nearFar.y, nearFar.x);
		//projection = glm::ortho(-x, x, -y, y, nearFar.y, nearFar.x);

		//projection = glm::ortho(0.f, screenSize.x, 0.f, screenSize.y, nearFar.x, nearFar.y);
		//projection = glm::ortho(orthoBounds.x, orthoBounds.y, orthoBounds.z, orthoBounds.w);
		//projection = glm::ortho(-x, x, -y, y);
	}

	if (useAngleControls)
	{
		// Rotate point position using horizontal and vertical angles
		mat4 verticalRotation = glm::rotate(verticalAngle, vec3(1, 0, 0));
		mat4 horizontalRotation = glm::rotate(horizontalAngle, vec3(0, 1, 0));

		totalRotation = horizontalRotation * verticalRotation;

		vec4 cv = vec4(viewDir.x, viewDir.y, viewDir.z, 0);
		vec4 cu = vec4(upDir.x, upDir.y, upDir.z, 0);

		currentView = totalRotation * cv;
		currentUp = totalRotation * cu;

		Position = Lookat + (vec3(currentView.x, currentView.y, currentView.z) * -distance);
		view = lookat(Position, Lookat, currentUp);
	}
	else
	{
		view = glm::lookAt(Position, Lookat, vec3(0, 1, 0));
	}


	quickUpdate();
}

void Camera::quickUpdate()
{
	viewproj = projection * view;

	inv_projection = glm::inverse(projection);
	inv_view = glm::inverse(view);
	inv_viewproj = glm::inverse(viewproj);

	updateFrustum();
}

void Camera::updateFrustum()
{
	// Compute frustum planes
	vec4 row0 = glm::row(viewproj, 0);
	vec4 row1 = glm::row(viewproj, 1);
	vec4 row2 = glm::row(viewproj, 2);
	vec4 row3 = glm::row(viewproj, 3);

	vec4 left = row3 + row0;
	vec4 right = row3 - row0;
	vec4 bottom = row3 + row1;
	vec4 top = row3 - row1;
	vec4 near = row3 + row2;
	vec4 far = row3 - row2;

	frustumPlanes[0] = left;
	frustumPlanes[1] = right;
	frustumPlanes[2] = bottom;
	frustumPlanes[3] = top;
	frustumPlanes[4] = near;
	frustumPlanes[5] = far;
}

void Camera::orbit(float dh, float dv)
{
	if (fabs(dh) < CAMERA_EPS && fabs(dv) < CAMERA_EPS) return;

	horizontalAngle += dh;
	verticalAngle += dv;

	change(true);
	update();
}

void Camera::pan(float dx, float dy)
{
	if (fabs(dx) < CAMERA_EPS && fabs(dy) < CAMERA_EPS) return;

	vec3 unitDir = totalRotation * vec4(dx, dy, 0, 1.0f);

	vec3 camDir = glm::normalize(unitDir) * panScale * distance;

	Lookat += camDir;
	Position += camDir;

	change(true);
	update();
}

void Camera::zoom(float dz)
{

	// determine the parameterized position
	float dist = distance - distanceMinMax.x;

	// Determine the log of it
	float logd = glm::clamp((float)(log10(dist)), 0.1f, dist);
	//float t = dist / (distanceMinMax.y - distanceMinMax.x);

	// cubic in
	//t = clamp<float>(t * t * t, 0.0f, 1.0f);

	float s = glm::sign(dz);

	// Calculate zoom amount proportional to mouse input and current zoom distance
	float zoomAmount = (s * distanceMinMax.x) + (dz * logd);
	distance += zoomAmount;

	change(true);
	update();
}

void Camera::alignToAxis(const vec3& axis)
{
	horizontalAngle = 0;
	verticalAngle = 0;

	change(true);
	update();

	if (axis[0] != 0)
	{
		if (axis[0] > 0)
		{
			horizontalAngle = (float)-glm::half_pi<float>();
		}
		else
		{
			horizontalAngle = (float)glm::half_pi<float>();
		}
	}

	if (axis[1] != 0)
	{
		if (axis[1] > 0)
		{
			verticalAngle = (float)-glm::half_pi<float>();
		}
		else
		{
			verticalAngle = (float)glm::half_pi<float>();
		}
	}

	if (axis[2] != 0)
	{
		if (axis[2] > 0)
		{
			horizontalAngle = (float)glm::pi<float>();
		}
		else
		{
			horizontalAngle = 0;
		}
	}

	change(true);
	update();
}

bool Camera::isInsideWindow(vec3 pos)
{
	return !isCulled(pos.x, pos.y, pos.z);
}

bool Camera::isCulled(float x, float y, float z)
{
	bool clipped = false;
	// Iterate over 6 clip planes

	vec4 vp(x, y, z, 1.0f);
	for (int i = 0; i < 6; i++)
	{
		//float planeDist = planeEquations[i][0] * x + planeEquations[i][1] * y + planeEquations[i][2] * z + planeEquations[i][3];

		float planeDist = dot(frustumPlanes[i], vp);

		if (planeDist < 0)
		{
			clipped = true;
			break;
		}
	}

	return clipped;
}

vec3 Camera::getScreenPoint(const vec3& worldPoint) const
{
	vec3 sp = project(worldPoint);

	return vec3(sp.x, sp.y, viewport[3] - sp.z);
}

vec3 Camera::getWorldPoint(const vec3& screenPoint) const
{
	vec3 wp = unproject(screenPoint);

	return wp;
}

Camera Camera::linterp(float t, const Camera& a, const Camera& b)
{
	Camera c;

	if (t <= 0) return a;
	if (t >= 1) return b;

	c.Position = glm::mix(a.Position, b.Position, t);
	c.Lookat = glm::mix(a.Lookat, b.Lookat, t);
	c.currentView = glm::mix(a.currentView, b.currentView, t);
	c.currentUp = glm::mix(a.currentUp, b.currentUp, t);
	c.horizontalAngle = glm::mix(a.horizontalAngle, b.horizontalAngle, t);
	c.verticalAngle = glm::mix(a.verticalAngle, b.verticalAngle, t);
	c.distance = glm::mix(a.distance, b.distance, t);
	//c.totalRotation = glm::mix(a.totalRotation, b.totalRotation, t);
	c.FOV = glm::mix(a.FOV, b.FOV, t);
	c.nearFar = glm::mix(a.nearFar, b.nearFar, t);
	c.screenSize = glm::mix(a.screenSize, b.screenSize, t);

	//c.projection = glm::mix(a.projection, b.projection, t);
	//c.view = glm::mix(a.view, b.view, t);
	//c.inv_projection = glm::mix(a.inv_projection, b.inv_projection, t);
	//c.inv_view = glm::mix(a.inv_view, b.inv_view, t);

	for (int i = 0; i < 6; i++)
	{
		c.frustumPlanes[i] = glm::mix(a.frustumPlanes[i], b.frustumPlanes[i], t);
	}

	c.distanceMinMax = glm::mix(a.distanceMinMax, b.distanceMinMax, t);

	c.change(true);
	c.update();

	return c;
}

//json Camera::toJSON() const
//{
//	json c;
//
//	c["viewDir"] = viewDir.toString();
//	c["upDir"] = upDir.toString();
//
//	c["Position"] = Position.toString();
//	// What the camera's pointed at (view direction = Lookat - Position)
//	c["Lookat"] = Lookat.toString();
//
//	// Projection type
//	c["projectionType"] = projectionType._to_integral();
//
//	c["currentView"] = currentView.toString();
//	c["currentUp"] = currentUp.toString();
//
//	// The spherical coordinates of our camera
//	c["horizontalAngle"] = horizontalAngle;
//	c["verticalAngle"] = verticalAngle;
//	c["distance"] = distance;
//
//	// Used to create projection matrix
//	c["FOV"] = FOV;
//	c["nearFar"] = nearFar;
//
//	c["screenSize"] = screenSize;
//
//	c["distanceMinMax"] = distanceMinMax;
//
//	return c;
//}
//
//void Camera::applyJSON(const json & c)
//{
//	viewDir.applyJSON(c["viewDir"]);
//	upDir.applyJSON(c["upDir"]);
//
//	Position.applyJSON(c["Position"]);
//	Lookat.applyJSON(c["Lookat"]);
//
//	// Projection type
//	projectionType = ProjectionType::_from_integral(c["projectionType"]);
//
//	currentView.applyJSON(c["currentView"]);
//	currentUp.applyJSON(c["currentUp"]);
//
//	horizontalAngle = c["horizontalAngle"];
//	verticalAngle = c["verticalAngle"];
//	distance = c["distance"];
//
//	// Used to create projection matrix
//	FOV = c["FOV"];
//	nearFar.applyJSON(c["nearFar"]);
//
//	screenSize.applyJSON(c["screenSize"]);
//
//
//	change(true);
//}

// Gives a perspective projection matrix with reversed-z notation and sets far plane to infinity.
// Result is +z points towards you, so visible points have z < 0.
// Taken from here: https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/
mat4 Camera::perspective(float fov, float aspectRatio, float znear, float zfar)
{
	return glm::perspective(glm::radians(fov), aspectRatio, znear, zfar);

	float f = 1.0f / glm::tan(glm::radians(fov * 0.5f));
	return mat4(f / aspectRatio, 0, 0, 0,
		0, f, 0, 0,
		0, 0, 0, -1,
		0, 0, znear, 0);
}

mat4 Camera::frustum(float left, float right, float bottom, float top, float znear, float zfar)
{
	mat4 f = mat4(0.0f);

	float temp = 2.0f * znear;
	float temp2 = right - left;
	float temp3 = top - bottom;
	float temp4 = zfar - znear;

	f[0][0] = temp / temp2;
	f[0][2] = (right + left) / temp2;
	f[1][1] = temp / temp3;
	f[1][2] = (top + bottom) / temp3;
	f[2][2] = -(zfar + znear) / temp4;
	f[2][3] = (-temp * zfar) / temp4;
	f[3][2] = -1;

	return f;
}

// Taken from https://github.com/ValveSoftware/openvr/wiki/IVRSystem::GetProjectionRaw
mat4 Camera::frustumVR(float left, float right, float bottom, float top, float znear, float zfar)
{
	float idx = 1.0f / (right - left);
	float idy = 1.0f / (bottom - top);
	float idz = 1.0f / (zfar - znear);
	float sx = right + left;
	float sy = bottom + top;

	mat4 p = mat4(0.0f);
	p[0][0] = 2 * idx;
	p[0][2] = sx * idx;
	p[1][1] = 2 * idy;
	p[1][2] = sy * idy;
	p[2][2] = -zfar * idz;
	p[2][3] = -zfar * znear * idz;
	p[3][2] = -1.0f;

	return p;
}

mat4 Camera::orthographic(float left, float right, float bottom, float top, float znear, float zfar)
{
	static mat4 rz = glm::identity<mat4>();
	static bool rzi = false;
	if (!rzi)
	{
		//rz[2][2] = -1.0f;
		rzi = true;
	}

	// Using GLM's orthographic function, then adjusting for reverse-z
	auto glmOrtho = glm::ortho(left, right, bottom, top, zfar, znear);
	// Reverse z here

	return rz * glmOrtho;


	mat4 mat = mat4(1.0f);

	mat[0][0] = float(2.0) / (right - left);
	mat[1][1] = float(2.0) / (top - bottom);
	mat[2][2] = -float(2.0) / (zfar - znear);

	vec3 tr(-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(zfar + znear) / (zfar - znear));

	mat = mat * glm::translate(tr);

	return rz * mat;
}

mat4 Camera::lookat(vec3 eye, vec3 center, vec3 up)
{
	return glm::lookAt(eye, center, up);

	vec3 forward = glm::normalize(center - eye);
	up = glm::normalize(up);

	vec3 side = glm::normalize(glm::cross(forward, up));
	vec3 u = glm::cross(side, forward);

	mat4 mat = mat4(1.0f);

	mat[0][0] = side[0];
	mat[0][1] = side[1];
	mat[0][2] = side[2];

	mat[1][0] = u[0];
	mat[1][1] = u[1];
	mat[2][1] = u[2];

	mat[3][0] = -forward[0];
	mat[3][1] = -forward[1];
	mat[3][2] = -forward[2];

	vec3 pos = vec3(-glm::dot(side, eye), -glm::dot(u, eye), glm::dot(forward, eye));

	mat = mat * glm::translate(pos);

	return mat;
}

vec3 Camera::project(vec3 obj, bool flipY) const
{
	vec4 obj4(obj, 1.0);

	vec4 objPrime = viewproj * obj4;

	objPrime /= objPrime.w;
	objPrime = (objPrime * 0.5f) + 0.5f;


	objPrime.x = objPrime.x * float(viewport[2] + viewport[0]);
	objPrime.y = objPrime.y * float(viewport[3] + viewport[1]);

	if (flipY)
	{
		objPrime.y = viewport[3] - objPrime.y;
	}

	return vec3(objPrime);
}

vec3 Camera::unproject(vec3 win, bool flipY) const
{
	if (flipY)
	{
		win.y = viewport[3] - win.y;
	}

	vec4 tmp;

	tmp.x = ((win.x - float(viewport[0])) / (float(viewport[2])) * 2.0) - 1.0;
	tmp.y = ((win.y - float(viewport[1])) / (float(viewport[3])) * 2.0) - 1.0;
	tmp.z = 2.0 * win.z - 1.0;
	tmp.w = 1.0;

	vec4 obj = glm::inverse(projection * view) * tmp;
	//vec4 obj2 = (view * projection).inverse(false) * tmp;
	//vec4 obj3 = inv_viewproj * tmp;

	vec3 result1 = obj;
	if (obj.w != 0.0f && obj.w == obj.w)
	{
		result1 *= 1.0f / obj.w;
	}

	//vec3 result2 = vec3(obj2.xyz) * (float(1.0) / obj2.w);
	//vec3 result3 = obj3.xyz * (1.0f / obj3.w);

	return result1;
	//return result2;
	//return result3;
}

vec3 Camera::getViewVector()
{
	return glm::normalize(Lookat - Position);
}

//Ray Camera::getRay(const vec2 & point)
//{
//	// Convert point to NDC
//	vec4 ndc;
//
//	float x = point.x;
//	float y = viewport[3] - point.y;
//
//	ndc.x = ((x - float(viewport[0])) / (float(viewport[2])) * 2.0f) - 1.0f;
//	ndc.y = ((y - float(viewport[1])) / (float(viewport[3])) * 2.0f) - 1.0f;
//	ndc.z = 1.0f;
//	ndc.w = 1.0f;
//
//	// Convert to clip, eye, and world space
//	vec4 ray_clip(ndc.x, ndc.y, -1.0, 1.0);
//	vec4 ray_eye = glm::inverse(projection) * ray_clip;
//	ray_eye = vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);
//	vec3 ray_world = glm::normalize(vec3(glm::inverse(view)* ray_eye));
//
//	return Ray(Position, ray_world);
//}

// Create a keyframe given the frame number. Set keySelected to false to key all (not recommended for mesh)
//s_ptr<Anim::Keyframe> Camera::key(int frameNumber, bool keySelected)
//{
//	sts::log("Keying camera...\n");
//	auto camDeform = s_ptr<GenericDeform<Camera>>(new GenericDeform<Camera>(this, *this, *this));
//	return s_ptr<Anim::Keyframe>(new Anim::Keyframe(frameNumber, camDeform));
//}
//
//// Attempts to remove selected components of the object from the provided keyframe.
//s_ptr<Anim::Keyframe> Camera::deleteKey(s_ptr<Anim::Keyframe> keyframe)
//{
//	return nullptr;
//}
//
//// Computes an interpolated keyframe between two values
//s_ptr<Anim::Keyframe> Camera::interpKeys(float t, s_ptr<Anim::Keyframe> a, s_ptr<Anim::Keyframe> b)
//{
//	if (a && b)
//	{
//		auto camA = std::dynamic_pointer_cast<GenericDeform<Camera>>(a->data);
//		auto camB = std::dynamic_pointer_cast<GenericDeform<Camera>>(b->data);
//
//		if (camA && camB)
//		{
//			int frameNumber = (int)std::round(float_linterp(t, a->frame, b->frame));
//			auto camAB = Camera::linterp(t, camA->before, camB->after);
//			auto interpDeform = std::make_shared<GenericDeform<Camera>>(nullptr, camAB, camAB);
//			return s_ptr<Anim::Keyframe>(new Anim::Keyframe(frameNumber, interpDeform));
//		}
//	}
//
//	return nullptr;
//}
//
//// Function to call when animation returns to frame 0
//void Camera::animationReset()
//{
//
//}
//
//// Function to call when you need to create a keyframe from a Json value
//s_ptr_vector<Anim::Keyframe> Camera::createFromJson(const json & value)
//{
//	s_ptr_vector<Anim::Keyframe> results;
//	for (auto & kf : value)
//	{
//		int frame = kf["frameNumber"];
//
//		Camera cam;
//		cam.applyJSON(kf["data"]);
//
//		auto camDeform = s_ptr<GenericDeform<Camera>>(new GenericDeform<Camera>(nullptr, cam, cam));
//
//		auto keyframe = s_ptr<Anim::Keyframe>(new Anim::Keyframe(frame, camDeform));
//		results.push_back(keyframe);
//	}
//
//	return results;
//}

void Camera::focusOnJoints()
{
	vec3 lookat = vec3(0);
	int count = 0;
	/*for (auto & m : Aestus::getMeshes())
	{
		for (auto & joint : m->skeleton->joints)
		{
			if (joint->details->selected)
			{
				lookat += joint->world.position;
				count++;
			}
		}
	}*/

	if (count > 0)
	{
		lookat *= 1.0f / (float)count;

		focusOn(lookat);
	}
}

void Camera::focusOn(const vec3& newLookat)
{
	/*Aestus::getCamera()->Lookat = newLookat;
	Aestus::getCamera()->change(true);*/
}

bool Camera::saveToFile(std::ofstream& fout)
{
	//if (!IO::writeContents(fout, FileContents::Camera)) return false;

	/*viewDir.saveToFile(fout);
	upDir.saveToFile(fout);
	Position.saveToFile(fout);
	Lookat.saveToFile(fout);
	fout.write(reinterpret_cast<const char*>(&projectionType), sizeof(ProjectionType));
	currentView.saveToFile(fout);
	currentUp.saveToFile(fout);
	fout.write(reinterpret_cast<const char*>(&horizontalAngle), sizeof(horizontalAngle));
	fout.write(reinterpret_cast<const char*>(&verticalAngle), sizeof(verticalAngle));
	fout.write(reinterpret_cast<const char*>(&distance), sizeof(distance));
	totalRotation.saveToFile(fout);
	fout.write(reinterpret_cast<const char*>(&FOV), sizeof(FOV));
	nearFar.saveToFile(fout);
	screenSize.saveToFile(fout);
	distanceMinMax.saveToFile(fout);*/

	return true;
}

bool Camera::loadFromFile(std::ifstream& fin)
{
	//auto fc = IO::readContents(fin);
	//if (fc != +FileContents::Camera) return false;

	/*if (!viewDir.loadFromFile(fin)) return false;
	if (!upDir.loadFromFile(fin)) return false;
	if (!Position.loadFromFile(fin)) return false;
	if (!Lookat.loadFromFile(fin)) return false;
	fin.read((char*)&projectionType, sizeof(projectionType));
	if (!currentView.loadFromFile(fin)) return false;
	if (!currentUp.loadFromFile(fin)) return false;
	fin.read((char*)&horizontalAngle, sizeof(horizontalAngle));
	fin.read((char*)&verticalAngle, sizeof(verticalAngle));
	fin.read((char*)&distance, sizeof(distance));
	if (!totalRotation.loadFromFile(fin)) return false;
	fin.read((char*)&FOV, sizeof(FOV));
	if (!nearFar.loadFromFile(fin)) return false;
	if (!screenSize.loadFromFile(fin)) return false;
	if (!distanceMinMax.loadFromFile(fin)) return false;*/

	return true;
}

void Camera::renderUI()
{
	if (ImGui::CollapsingHeader("Camera"))
	{
		ImGui::Indent(16.0f);

		ImGui::PushID((const void*)this);

		changed = false;

		changed |= renderEnumButton<ProjectionType>(projectionType);
		if (projectionType == +ProjectionType::Perspective)
		{
			changed |= ImGui::InputFloat3("Position", glm::value_ptr(Position));
			changed |= ImGui::InputFloat3("Lookat", glm::value_ptr(Lookat));
			changed |= ImGui::InputFloat("FOV", &FOV);
		}
		else if (projectionType == +ProjectionType::Orthographic)
		{
			changed |= ImGui::InputFloat4("Ortho bounds", glm::value_ptr(orthoBounds));
		}

		changed |= ImGui::InputFloat2("NearFar", glm::value_ptr(nearFar));
		changed |= ImGui::InputFloat2("Distance min/max", glm::value_ptr(distanceMinMax));

		changed |= ImGui::Checkbox("Use angle controls", &useAngleControls);
		if (useAngleControls)
		{
			changed |= ImGui::SliderFloat("H. angle", &horizontalAngle, 0.0f, glm::two_pi<float>());
			changed |= ImGui::SliderFloat("V. angle", &verticalAngle, 0.0f, glm::two_pi<float>());
			changed |= ImGui::SliderFloat("Distance", &distance, distanceMinMax.x, distanceMinMax.y);
		}

		if (changed)
		{
			update();
		}

		ImGui::TextWrapped("View matrix:\n %s", glm::to_string<mat4>(view).c_str());
		ImGui::TextWrapped("Proj matrix:\n %s", glm::to_string<mat4>(projection).c_str());

		ImGui::PopID();

		ImGui::Unindent(16.0f);
	}
}
