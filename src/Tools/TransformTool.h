#pragma once

#include "Tool.h"

MAKE_ENUM(TransformToolMode, int, Off, Translate, Rotate, Scale);

struct AnimationObject;

class TransformTool : public Tool {
public:
	TransformToolMode mode = TransformToolMode::Off;

	vec3 transformWeights = vec3(1.f, 2.f, 3.f);

	vec2 mousePos, lastMousePos, selectMousePos;

	std::map<AnimationObject*, vec3> selectedOffsets;

	vec2 scroll, lastScroll;

	TransformTool() : Tool("TransformTool", defaultEvents()) {}

	virtual ~TransformTool() {}

	virtual EventActions defaultEvents();

	virtual void activate();

	//virtual void deactivate() { Tool::deactivate(); mode = TransformToolMode::Off; }

	virtual void update(std::vector<InputEvent>& events);


	virtual void renderUI();
};
