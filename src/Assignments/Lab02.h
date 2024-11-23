#pragma once

#include "Assignment.h"

//#include "GPU.h"
//#include "Shader.h"

class Lab02 : public Assignment {
public:

	Lab02() : Assignment("Lab 02", true) { }
	virtual ~Lab02() { }

	virtual void init();
	virtual void update();

	virtual void render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow = false);
	virtual void renderUI();

	

	virtual ptr_vector<AnimationObject> getObjects();
};
