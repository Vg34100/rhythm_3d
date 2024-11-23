#pragma once

#define GPU_LIGHT_BINDING_SPOT 3
#define GPU_LIGHT_MAX_COUNT 3

struct Ray
{
	vec3 origin;
	vec3 direction;
	ivec2 screenspace;
	vec2 uv;
};

struct HitRecord
{
	vec3 point;
	vec3 normal;
	vec3 color;
	vec3 reflection;
	float t;
	int shapeIndex;
	bool front;
	bool hit;
	vec2 barycentric;
};

struct Shape
{
	// For sphere: xyz is center, w is radius. For plane: xyz is point, 
	vec4 coords;
	vec4 color;
	// x: 0 is sphere, 1 is plane, that's all I gots for now baby
	// y: enabled 1 or 0
	vec4 data;
};

struct Light
{
	// w is whether this light is a directional (0) or spot (1)
	vec4 position;
	vec4 direction;
	vec4 diffuse;
	vec4 specular;
	// x is constant, y is linear, z is quadratic
	vec4 attenuation;
	// x is cutoff, y is exponent
	vec4 spot;
};

struct SceneLight
{
	vec4 mainDiffuse;
	vec4 mainSpecular;
	vec4 scene_ambient;
};

struct Material
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

struct CameraParams
{
	vec4 position;
	vec4 lookat;
	// normalized lookat - position
	vec4 lookdir;
	vec4 up;
	vec4 focusat;
	

	float hfov;
	float focalLength;
	float fStop;
	float diameter;			// focal length of lens / aperture number

	float radius;
	float n;
	float ulen, vlen;

	vec4 u;
	vec4 v;

	// x is horizontal aspect ratio, y is vertical
	vec2 aspectRatio;
	uvec2 resolution;

	mat4 mvp;
	mat4 inv_mvp;
};

struct RuntimeParams
{
	CameraParams camera;
	vec4 clearColor;
	vec4 ambientColor;
	int shapeCount;
	int lightCount;
	int maxRayBounces;
	int renderOnDemand;
	int renderedFrameCount;
	int maxFrames;
	int accumulate;
	float accumulateBlend;
	int clearAccumulate;
	float time;
};