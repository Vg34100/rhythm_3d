#pragma once

#include "Assignment.h"

class Lab09 : public Assignment {
public:

	Lab09() : Assignment("Lab 09", true) { }
	virtual ~Lab09() { }

	virtual void init();
	virtual void update();

	virtual void render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow = false);
	virtual void renderUI();

	virtual ptr_vector<AnimationObject> getObjects();
};