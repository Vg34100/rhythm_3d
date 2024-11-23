#include "Textures.h"
#include "Application.h"
#include "Renderer.h"

namespace TextureRegistry {
    s_ptr<Texture> getTexture(GLuint texID) {
        if (!OpenGLRenderer::readyToRock) return nullptr;

        if (auto ogl = Application::get().getRenderer<OpenGLRenderer>()) {
            auto tex = ogl->textures.find(texID);
            if (tex != ogl->textures.end()) {
                return tex->second;
            }
        }

        return nullptr;
    }

    bool addTexture(s_ptr<Texture> tex) {
        if (!OpenGLRenderer::readyToRock) return false;
        if (auto ogl = Application::get().getRenderer<OpenGLRenderer>()) {
            ogl->textures[tex->id] = tex;
        }

        return true;
    }

    bool removeTexture(GLuint texID) {
        if (!OpenGLRenderer::readyToRock) return false;
        if (auto ogl = Application::get().getRenderer<OpenGLRenderer>()) {
            auto f = ogl->textures.find(texID);
            if (f != ogl->textures.end()) {
                ogl->textures.erase(f);

                return true;
            }
        }

        return false;
    }
}
