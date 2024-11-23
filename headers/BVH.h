#pragma once

#include "globals.h"
#include "Primitives.h"
#include "RayTracing.h"

class BVHNode;

// Bounded volume hierarchy
class BVH {
public:
	RTOBJMesh * mesh = nullptr;

	int trisPerBox = 3000;
	int maxDepth = 4;

	int numNodes = 0;

	s_ptr<BVHNode> root;

	BVH(RTOBJMesh* _mesh) : mesh(_mesh) {}

	void build(const Transform2D & transform, int depth = 4);

	bool hit(const Ray & ray, HitRecord& hr);

	void renderUI();

};

class BVHNode {
public:
	int depth = 0;
	vec3 min;
	vec3 max;
	// Use a raw pointer to the parent - this node doesn't claim ownership of the parent, so
	// this will effectively cause a cascading delete
	BVHNode* parent = nullptr;
	s_ptr_vector<BVHNode> children;

	BVH* bvh = nullptr;

	RaySceneObject object;

	std::vector<int> vertices;
	std::vector<int> triangles;



	BVHNode(BVH* _bvh, const vec3 & _min, const vec3& _max, BVHNode * _parent = nullptr);

	~BVHNode() {
		log("Destroying node {0}\n", (void*)this);
	}

	bool hitTriangles(const Ray& ray, HitRecord& hr);

	bool hit(const Ray& ray, HitRecord& hr);
};
