#pragma once

#include "Assignment.h"

class Lab05 : public Assignment {
public:

	Lab05() : Assignment("Lab 05", true) { }
	virtual ~Lab05() { }

	virtual void init();
	virtual void update();

	virtual void render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow = false);
	virtual void renderUI();


	virtual ptr_vector<AnimationObject> getObjects();

	void saveScene();
	void loadScene();
};