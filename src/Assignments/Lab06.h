#pragma once

#include "Assignment.h"

class Lab06 : public Assignment {
public:

	Lab06() : Assignment("Lab 06", true) { }
	virtual ~Lab06() { }

	virtual void init();
	virtual void update();

	virtual void render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow = false) {}
	virtual void renderUI();

	virtual ptr_vector<AnimationObject> getObjects();
};