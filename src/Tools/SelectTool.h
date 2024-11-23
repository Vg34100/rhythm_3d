#pragma once

#include "Tool.h"

MAKE_ENUM(SelectToolMode, int, Mesh, Face, Vertex, Skeleton);

class SelectTool : public Tool {
	public: 
		SelectToolMode mode = SelectToolMode::Mesh;

		vec4 currentColorData = vec4(0.f);
		ivec4 currentSelectData = vec4(0.f);
		ivec2 mousePos;

		bool selecting = false, lastFrameSelecting = false;

		int textureToReadFrom = 1;

		SelectTool() : Tool("SelectTool", defaultEvents()) {}

		virtual ~SelectTool() {}

		virtual EventActions defaultEvents();

		virtual void update(std::vector<InputEvent>& events);

		virtual void renderUI();
};
