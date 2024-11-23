#pragma once

#include "globals.h"


// Interpolation functions
// Returns the minimum value until t >= 1
inline float stepmin(float min, float max, float t) {
	if (t >= 1.f) return max;
	return min;
}

// Returns the maximum value except for when t <= 0
inline float stepmax(float min, float max, float t) {
	if (t <= 0.f) return min;
	return max;
}

// Returns the linear interpolation between min and max
inline float lerp(float min, float max, float t) {
	return t * max + (1.0f - t) * min;
}

// Returns the smoothed hermite interpolation between min and max
inline float hermite(float min, float max, float t) {
	if (t <= 0.f) return min;
	if (t >= 1.f) return max;

	t = (t - min) / (max - min);

	return t * t * (3.f - 2.f * t);
}

inline float bounceOut(float min, float max, float t)
{
	const float a = 4.0 / 11.0;
	const float b = 8.0 / 11.0;
	const float c = 9.0 / 10.0;

	const float ca = 4356.0 / 361.0;
	const float cb = 35442.0 / 1805.0;
	const float cc = 16061.0 / 1805.0;

	float t2 = t * t;

	return t < a
		? 7.5625 * t2
		: t < b
		? 9.075 * t2 - 9.9 * t + 3.4
		: t < c
		? ca * t2 - cb * t + cc
		: 10.8 * t * t - 20.52 * t + 10.72;
}


// Interpolation function pointer.
// Inputs: minimum, maximum, t in [0..1]
// Output: float computed using minimum, maximum, and t
using InterpolationFunction = float(*)(float, float, float);


// Enum definitions for creating interpolations.
// These make it easier to specify what should be animated using the UI code.

// An enum of the targets for interpolation.
MAKE_ENUM(InterpolationTarget, int, Position, Rotation, Scale, Color)
// An enum specifying which parts of the target to animate. 
MAKE_ENUM(InterpolationChannel, int, X, Y, Z, Alpha)
// An enum of the types of interpolation functions available. 
MAKE_ENUM(InterpolationFunctionType, int, stepmin, stepmax, lerp, hermite, bounceOut, customInterp)

// Forward declaration 
struct AnimationObject;

// Data needed to create interpolation-based animation. Contains all the properties needed
// to apply animations to a shape's property. Using the menu, you can change the property
// being animated, but it can only animate one property at a time. 
// So if you want animations on both X and Y positions, you need two different structs that
// have the same value for the shape pointer, but different values for the channel
struct InterpolationData {
	// The shape to which this animation is applied.
	AnimationObject* shape = nullptr;

	bool enabled = true;
	int startFrame = 0;
	int endFrame = 0;
	// The property float points directly to the target and channel of the shape. 
	// You should be able to assign the interpolated value to this by using
	// *property = <value from interpolation function>;
	float* property = nullptr;
	float minValue = 0.f;
	float maxValue = 1.f;
	// Can be Position, Rotation, Scale, or Color
	InterpolationTarget target = InterpolationTarget::Position;
	// Can be X, Y, Z, or Alpha (for color)
	InterpolationChannel channel = InterpolationChannel::X;
	InterpolationFunctionType interp = InterpolationFunctionType::stepmin;

	// A pointer to the function that should be used for interpolation. You can invoke
	// this by calling it like a function and passing in parameters for min, max, and t:
	// float result = f(minValue, maxValue, t);
	InterpolationFunction f = nullptr;


	// Maintains the property and f pointers so you don't have to!
	void updatePointers();
};

void to_json(json& j, const InterpolationData& s);
void from_json(const json& j, InterpolationData& s);