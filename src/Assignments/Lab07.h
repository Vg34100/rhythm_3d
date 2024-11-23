#pragma once

#include "Assignment.h"

class Lab07 : public Assignment {
public:

	Lab07() : Assignment("Lab 07", true) { }
	virtual ~Lab07() { }

	virtual void init();
	virtual void update();

	virtual void render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow = false);
	virtual void renderUI();

	virtual ptr_vector<AnimationObject> getObjects();
};