#pragma once

#include "Assignment.h"

class Lab04 : public Assignment {
public:

	Lab04() : Assignment("Lab 04", true) { }
	virtual ~Lab04() { }

	virtual void init();
	virtual void update();

	virtual void render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow = false);
	virtual void renderUI();


	virtual ptr_vector<AnimationObject> getObjects();

	void saveScene();
	void loadScene();
};