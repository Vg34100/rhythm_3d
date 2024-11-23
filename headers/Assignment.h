#pragma once

#include "globals.h"
#include "imgui.h"

struct AnimationObject;
class Framebuffer;
class Texture;



class Assignment {
	public:
		bool initialized = false;
		bool useOpenGL = false;
		std::string name;

		Assignment() {}
		Assignment(const std::string& _name, bool openGL = false) : name(_name), useOpenGL(openGL) { }
		virtual ~Assignment() { }

		virtual void init() { }

		virtual void update() { }
		
		virtual void render(s_ptr<Texture> screen) { }
		virtual void render(const mat4& projection, const mat4& view, s_ptr<Framebuffer> framebuffer, bool isShadow = false) { }
		virtual void renderUI() { }

		virtual ptr_vector<AnimationObject> getObjects() = 0;
};