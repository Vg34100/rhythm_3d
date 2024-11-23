#pragma once

#include "Assignment.h"

class Lab11 : public Assignment {
public:

	Lab11() : Assignment("Lab 11", true) { }
	virtual ~Lab11() { }

	virtual void init();
	virtual void update();

	virtual void render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow = false);
	virtual void renderUI();

	virtual ptr_vector<AnimationObject> getObjects();
};