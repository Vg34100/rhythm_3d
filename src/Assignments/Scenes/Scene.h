#pragma once
#include <globals.h>
#include <AnimationObject.h>

template<typename T>
using ptr_vector = std::vector<T*>;

class Scene {
    public:
        virtual ~Scene() = default;
        virtual void init() = 0;
        virtual void update(double now, float dt) = 0;
        virtual void render(const mat4& projection, const mat4& view, bool isShadow) = 0;
        virtual void renderUI() = 0;
        virtual ptr_vector<AnimationObject> getObjects() = 0;
};