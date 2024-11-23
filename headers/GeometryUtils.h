#pragma once
#include "globals.h"

#define GEO_RDP_EPSILON 2.0f


bool inside2DPolygon(float x, float y, const std::vector<vec2>& lasso);


std::vector<vec2> reduce_RDP(std::vector<vec2> points, float epsilon = GEO_RDP_EPSILON, int depth = 0);
std::vector<vec3> reduce_RDP(std::vector<vec3> points, float epsilon = GEO_RDP_EPSILON, int depth = 0);


//void smoothSampler(sampler<>& sample, float epsilon = GEO_RDP_EPSILON, float smoother = 0.5f);


inline float lineSegmentTValue(const vec2& start, const vec2& end, const vec2& p)
{
	vec2 a = p - start;
	vec2 b = p - end;
	vec2 c = end - start;

	return glm::dot(a, c) / glm::length2(c);
}

inline float lineSegmentTValue(const vec3& start, const vec3& end, const vec3& p)
{
	vec3 a = p - start;
	vec3 b = p - end;
	vec3 c = end - start;

	return glm::dot(a, c) / glm::length2(c);
}

// Finds the distance between a point p and a line segment between start and end
inline float lineSegmentDistance(const vec2& start, const vec2& end, const vec2& p)
{
	vec2 a = p - start;
	vec2 b = p - end;
	vec2 c = end - start;

	float al = glm::length(a), bl = glm::length(b), cl = glm::length(c);

	// Edge cases
	if (al == 0.0f) return 0.0f;
	if (bl == 0.0f) return 0.0f;
	if (cl == 0.0f) return al;

	// Find projection of p onto (end - start) vector
	//T t = dot(a, c) / cl2;
	float t = lineSegmentTValue(start, end, p);

	// Outside boundaries
	if (t < 0.0f) return al;
	else if (t > 1.0f) return bl;

	// Projected point
	vec2 pnew = start + (t * c);

	return glm::length(pnew - p);
}

inline float lineSegmentDistance(const vec3& start, const vec3& end, const vec3& p)
{
	vec3 a = p - start;
	vec3 b = p - end;
	vec3 c = end - start;

	float al = glm::length(a), bl = glm::length(b), cl = glm::length(c);

	// Edge cases
	if (al == 0.0f) return 0.0f;
	if (bl == 0.0f) return 0.0f;
	if (cl == 0.0f) return al;

	// Find projection of p onto (end - start) vector
	//T t = dot(a, c) / cl2;
	float t = lineSegmentTValue(start, end, p);

	// Outside boundaries
	if (t < 0.0f) return al;
	else if (t > 1.0f) return bl;

	// Projected point
	vec3 pnew = start + (t * c);

	return glm::length(pnew - p);
}

template<typename T>
std::vector<T> reduce_RDP_position(std::vector<T> points, float epsilon = GEO_RDP_EPSILON, int depth = 0)
{
	if (points.empty()) return{};

	int numPoints = (int)points.size();
	float dmax = 0;
	int imax = 0;

	vec3 start = points[0].position;
	vec3 end = points[numPoints - 1].position;

	// Find max distance between points and line segement between endpoints
	for (int i = 1; i < numPoints - 1; i++)
	{
		float d = lineSegmentDistance(start, end, points[i].position);

		if (d > dmax)
		{
			dmax = d;
			imax = i;
		}
	}

	// If max exceeds epsilon, subdivide (and keep the start, end, and max points!)
	if (dmax > epsilon)
	{
		//printf("reduce_RDP(%d): depth is %f (exceeds %f), subdividing...\n", depth, dmax, epsilon);
		std::vector<T> prevPoints, nextPoints;
		for (int i = 0; i < numPoints; i++)
		{
			if (i <= imax)
			{
				prevPoints.push_back(points[i]);
			}
			if (i >= imax)
			{
				nextPoints.push_back(points[i]);
			}
		}

		std::vector<T> prev = reduce_RDP_position(prevPoints, epsilon, depth + 1);
		std::vector<T> next = reduce_RDP_position(nextPoints, epsilon, depth + 1);

		//printf("reduce_RDP(%d) prev size: %d\n", depth, prev.size());
		//printf("reduce_RDP(%d) next size: %d\n", depth, next.size());

		prev.insert(prev.end(), next.begin() + 1, next.end());

		return prev;
	}

	else
	{
		return std::vector<T> {points.front(), points.back()};
	}
}

namespace glm
{
	template<typename T>
	inline T projectOnto(const T& a, const T& b)
	{
		// magnitude of vector
		auto mag = glm::dot(a, b) / glm::length(b);

		// Then apply it to b
		return mag * b;
	}

	// Returns the point on the line from (start to end) that is closest to point p
	template <typename T>
	inline float parameterizedProjection(const T& start, const T& end, const T& p, bool _clamp = false)
	{
		T a = p - start;
		T b = p - end;
		T c = end - start;

		auto al = glm::length(a), bl = glm::length(b), cl = glm::length(c), cl2 = glm::length2(c);

		// Edge cases
		// p == start
		if (al == 0) return 0.0f;
		// start == end, so either point is the closest on the "line"
		if (cl == 0) return 0.0f;
		// p == end
		if (bl == 0) return 1.0f;


		// Find projection of p onto (end - start) vector
		float t = glm::dot(a, c) / cl2;

		// Outside boundaries
		if (_clamp)
		{
			t = glm::clamp<float>(t, 0, 1);
		}

		return t;
	}

	// Returns the point on the line from (start to end) that is closest to point p
	template <typename T>
	inline T closestOnLine(const T& start, const T& end, const T& p)
	{
		// Find projection of p onto (end - start) vector
		float t = parameterizedProjection<T>(start, end, p, true);

		// Projected point
		return start + (t * (end - start));
	}

	// Gets the length of the vector between p and its closest point on (end - start)
	template<typename T>
	static double lineSegmentDistance(const T& start, const T& end, const T& p)
	{
		return glm::length(closestOnLine<T>(start, end, p) - p);
	}

	// Projects point p onto the line segment between a and b
	template<typename T>
	inline T projection(const T& a, const T& b, const T& p, bool clamp = false)
	{
		if (a == b) return a;

		T d = b - a;

		double t = parameterizedProjection<T>(a, b, p, clamp);

		return a + (t * d);
	}

	// Get the projection of point p onto the vector between start and end. Note that t is not clamped to be in [0, 1]
	template <typename T>
	inline T projection(const T& start, const T& end, const T& p, float& t)
	{
		if (start == end)
		{
			t = 0;
			return start;
		}

		T a = p - start;
		T d = end - start;

		// Find projection of p onto (end - start) vector
		t = glm::dot(a, d) / glm::length2(d);

		// Projected point
		return start + t * d;
	}
}
