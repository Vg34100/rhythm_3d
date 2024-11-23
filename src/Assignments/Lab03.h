#pragma once

#include "Assignment.h"

//#include "GPU.h"
//#include "Shader.h"

class Lab03 : public Assignment {
public:

	Lab03() : Assignment("Lab 03", true) { }
	virtual ~Lab03() { }

	virtual void init();
	virtual void update();

	virtual void render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow = false);
	virtual void renderUI();


	virtual ptr_vector<AnimationObject> getObjects();
};
