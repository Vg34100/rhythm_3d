#pragma once

#include "Assignment.h"

class FinalProject : public Assignment {
public:

	FinalProject() : Assignment("Final Project", true) { }
	virtual ~FinalProject() { }

	virtual void init();
	virtual void update();

	virtual void render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow = false);
	virtual void renderUI();

	virtual ptr_vector<AnimationObject> getObjects();
};