#pragma once

#include "Texture.h"

namespace TextureRegistry {
	s_ptr<Texture> getTexture(GLuint texID);
	bool addTexture(s_ptr<Texture> tex);
	bool removeTexture(GLuint texID);
}