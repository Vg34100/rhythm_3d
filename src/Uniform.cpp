#include "Uniform.h"
#include "Textures.h"

#include "Application.h"
#include "UIHelpers.h"
//#include "Interpolation.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>



#include "imgui.h"

// GL Uniform update overloads

// Scalar uniform updates
template<>
void GLSLUniform<bool>::updateGLUniform() {
    if (location != -1) {
        glUniform1i(location, (GLint)value);
    }
}

template<>
void GLSLUniform<int>::updateGLUniform() {
    if (location != -1) {
        glUniform1i(location, value);
    };
}

template<>
void GLSLUniform<unsigned int>::updateGLUniform() {
    if (location != -1) {
        if (type == +GLSLUniformType::_sampler2D) {

            if (value > 0) {

                GLenum boundSpot = bindingUnit - GL_TEXTURE0;
                glActiveTexture(bindingUnit);
                glBindTexture(GL_TEXTURE_2D, value);
                glUniform1i(location, boundSpot);
                if (s_ptr<Texture> texture = TextureRegistry::getTexture(value)) {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture->wrapS._to_integral());
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture->wrapT._to_integral());
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture->minFilter._to_integral());
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture->magFilter._to_integral());
                }

            }
        }
        else if (type == +GLSLUniformType::_sampler2DShadow) {
            if (value > 0) {
                GLenum boundSpot = bindingUnit - GL_TEXTURE0;
                glActiveTexture(bindingUnit);
                glBindTexture(GL_TEXTURE_2D, value);
                glUniform1i(location, boundSpot);
                if (auto texture = TextureRegistry::getTexture(value)) {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture->wrapS._to_integral());
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture->wrapT._to_integral());
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture->minFilter._to_integral());
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture->magFilter._to_integral());
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_ALWAYS);
                }
            }
        }
        else {
            glUniform1ui(location, value);
        }

    }
}

template<>
void GLSLUniform<float>::updateGLUniform() {
    if (location != -1) {
        glUniform1f(location, value);
    }
}

template<>
void GLSLUniform<double>::updateGLUniform() {
    if (location != -1) {
        glUniform1d(location, value);
    }
}

// Bool vector uniform updates
template<>
void GLSLUniform<bvec2>::updateGLUniform() {
    if (location != -1) {
        glUniform2iv(location, 1, (GLint*)glm::value_ptr(value));
    }
}

template<>
void GLSLUniform<bvec3>::updateGLUniform() {
    if (location != -1) {
        glUniform3iv(location, 1, (GLint*)glm::value_ptr(value));
    }
}

template<>
void GLSLUniform<bvec4>::updateGLUniform() {
    if (location != -1) {
        glUniform4iv(location, 1, (GLint*)glm::value_ptr(value));
    }
}

// Int vector uniform updates
template<>
void GLSLUniform<ivec2>::updateGLUniform() {
    if (location != -1) {
        glUniform2iv(location, 1, (GLint*)glm::value_ptr(value));
    }
}

template<>
void GLSLUniform<ivec3>::updateGLUniform() {
    if (location != -1) {
        glUniform3iv(location, 1, (GLint*)glm::value_ptr(value));
    }
}

template<>
void GLSLUniform<ivec4>::updateGLUniform() {
    if (location != -1) {
        glUniform4iv(location, 1, (GLint*)glm::value_ptr(value));
    }
}

// Unsigned int vector uniform updates
template<>
void GLSLUniform<uvec2>::updateGLUniform() {
    if (location != -1) {
        glUniform2uiv(location, 1, (GLuint*)glm::value_ptr(value));
    }
}

template<>
void GLSLUniform<uvec3>::updateGLUniform() {
    if (location != -1) {
        glUniform3uiv(location, 1, (GLuint*)glm::value_ptr(value));
    }
}

template<>
void GLSLUniform<uvec4>::updateGLUniform() {
    if (location != -1) {
        glUniform4uiv(location, 1, (GLuint*)glm::value_ptr(value));
    }
}

// Float vector uniform updates
template<>
void GLSLUniform<vec2>::updateGLUniform() {
    if (location != -1) {
        glUniform2fv(location, 1, glm::value_ptr(value));
    }
}

template<>
void GLSLUniform<vec3>::updateGLUniform() {
    if (location != -1) {
        glUniform3fv(location, 1, glm::value_ptr(value));
    }
}

template<>
void GLSLUniform<vec4>::updateGLUniform() {
    if (location != -1) {
        glUniform4fv(location, 1, glm::value_ptr(value));
    }
}

// Double vector uniform updates
template<>
void GLSLUniform<dvec2>::updateGLUniform() {
    if (location != -1) {
        glUniform2dv(location, 1, glm::value_ptr(value));
    }
}

template<>
void GLSLUniform<dvec3>::updateGLUniform() {
    if (location != -1) {
        glUniform3dv(location, 1, glm::value_ptr(value));
    }
}

template<>
void GLSLUniform<dvec4>::updateGLUniform() {
    if (location != -1) {
        glUniform4dv(location, 1, glm::value_ptr(value));
    }
}

// Matrix uniform updates
template<>
void GLSLUniform<mat2>::updateGLUniform() {
    if (location != -1) {
        glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

template<>
void GLSLUniform<mat3>::updateGLUniform() {
    if (location != -1) {
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

template<>
void GLSLUniform<mat4>::updateGLUniform() {
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}









void GLSLUniformBase::renderUI() {
    ImGui::InputInt("Location", &location);

    renderEnumDropDown<GLSLUniformType>("Data type", type);
}

template<typename T>
void renderUI_AddAnimationCurve(GLSLUniform<T>& uniform) {
    /*static std::map<GLSLUniform<T>*, bool> displayMap;
    if (displayMap.find(&uniform) == displayMap.end()) {
        displayMap[&uniform] = false;
    }

    bool& display = displayMap[&uniform];

    if (ImGui::Button("Animate")) {
        display = true;
    }

    if (!display) return;

    ImGui::Begin("Animate", &display);
    static int animStart = 0;
    static int animEnd = 50;
    static float animMin = 0.f;
    static float animMax = 1.f;
    static InterpolationTarget animTarget = InterpolationTarget::Position;
    static InterpolationChannel animChannel = InterpolationChannel::X;
    static std::string uniformToAnimate = "";

    ImGui::PushID((const void*)&animStart);

    ImGui::Text("Animation curve for %s", uniform.variableName.c_str());
    ImGui::Separator();
    ImGui::InputInt("Starts on frame", &animStart);
    ImGui::InputInt("Ends on frame", &animEnd);
    ImGui::InputFloat("Property min", &animMin);
    ImGui::InputFloat("Property max", &animMax);
    renderEnumDropDown<InterpolationTarget>("Interp target", animTarget);
    renderEnumDropDown<InterpolationChannel>("Interp channel", animChannel);
    ImGui::PopID();

    if (ImGui::Button("Add")) {
        InterpolationData id = { nullptr, true, animStart, animEnd, reinterpret_cast<float*>(&uniform.value), animMin, animMax, animTarget, animChannel, InterpolationFunctionType::stepmin };
        auto& ac = Application::get().animationCurves;
        ac.push_back(InterpolationCurveSegment(id));
    }



    ImGui::End();*/
}

// renderUI overloads

template<>
void GLSLUniform<bool>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::Checkbox(variableName.c_str(), &value);

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<int>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::InputInt(variableName.c_str(), &value);

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<unsigned int>::renderUI() {
    ImGui::PushID((const void*)this);

    int vi = value;
    if (ImGui::InputInt(variableName.c_str(), &vi)) {
        value = vi;
    }

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<float>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::InputFloat(variableName.c_str(), &value);

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<double>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::InputDouble(variableName.c_str(), &value);

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

// bool vector imguis
template<>
void GLSLUniform<bvec2>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::Checkbox(variableName.c_str(), &value[0]);
    ImGui::SameLine();
    ImGui::Checkbox("", &value[1]);

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<bvec3>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::Checkbox(variableName.c_str(), &value[0]);
    ImGui::SameLine();
    ImGui::Checkbox("", &value[1]);
    ImGui::SameLine();
    ImGui::Checkbox("", &value[2]);

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<bvec4>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::Checkbox(variableName.c_str(), &value[0]);
    ImGui::SameLine();
    ImGui::Checkbox("", &value[1]);
    ImGui::SameLine();
    ImGui::Checkbox("", &value[2]);
    ImGui::SameLine();
    ImGui::Checkbox("", &value[3]);

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

// int vector imguis
template<>
void GLSLUniform<ivec2>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::InputInt2(variableName.c_str(), glm::value_ptr(value));

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<ivec3>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::InputInt3(variableName.c_str(), glm::value_ptr(value));


    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<ivec4>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::InputInt4(variableName.c_str(), glm::value_ptr(value));

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

// unsigned int vector imguis
template<>
void GLSLUniform<uvec2>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::InputInt2(variableName.c_str(), (int*)glm::value_ptr(value));

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<uvec3>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::InputInt3(variableName.c_str(), (int*)glm::value_ptr(value));

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<uvec4>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::InputInt4(variableName.c_str(), (int*)glm::value_ptr(value));

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

// Float vector imguis
template<>
void GLSLUniform<vec2>::renderUI() {

    struct SliderConfig {
        bool enabled = false;
        vec2 minMaxX = vec2(0.f, 1.f);
        vec2 minMaxY = vec2(0.f, 1.f);

    };
    static std::map<GLSLUniform<vec2>*, SliderConfig> sliderConfig;
    ImGui::PushID((const void*)this);

    auto& config = sliderConfig[this];

    ImGui::Checkbox("Slider?", &config.enabled);

    if (config.enabled) {
        ImGui::SliderFloat((variableName + " X").c_str(), &value.x, config.minMaxX.x, config.minMaxX.y, nullptr);
        ImGui::InputFloat2("MinMaxX", glm::value_ptr(config.minMaxX));
        ImGui::SliderFloat((variableName + " Y").c_str(), &value.y, config.minMaxY.x, config.minMaxY.y, nullptr);
        ImGui::InputFloat2("MinMaxY", glm::value_ptr(config.minMaxY));
    }
    else {
        ImGui::InputFloat2(variableName.c_str(), glm::value_ptr(value));
    }

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<vec3>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::InputFloat3(variableName.c_str(), glm::value_ptr(value));

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<vec4>::renderUI() {
    ImGui::PushID((const void*)this);

    if (StringUtil::contains(StringUtil::lower(variableName), "color")) {
        ImGui::ColorEdit4(variableName.c_str(), glm::value_ptr(value));
    }
    else {
        ImGui::InputFloat4(variableName.c_str(), glm::value_ptr(value));
    }


    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

// Double vector imguis
template<>
void GLSLUniform<dvec2>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::InputFloat2(variableName.c_str(), (float*)glm::value_ptr(value));

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<dvec3>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::InputFloat3(variableName.c_str(), (float*)glm::value_ptr(value));

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<dvec4>::renderUI() {
    ImGui::PushID((const void*)this);

    ImGui::InputFloat4(variableName.c_str(), (float*)glm::value_ptr(value));

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

// Matrix imguis
template<>
void GLSLUniform<mat2>::renderUI() {
    ImGui::PushID((const void*)this);

    imguiInputMatrix(variableName.c_str(), value);

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<mat3>::renderUI() {
    ImGui::PushID((const void*)this);

    imguiInputMatrix(variableName.c_str(), value);

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

template<>
void GLSLUniform<mat4>::renderUI() {
    ImGui::PushID((const void*)this);

    imguiInputMatrix(variableName.c_str(), value);

    renderUI_AddAnimationCurve(*this);

    ImGui::PopID();
}

s_ptr<GLSLUniformBase> GLSLUniformBase::createUniform(const std::string& datatype, const std::string& varname, const std::string& defaultValue) {
    GLSLUniformType ut = GLSLUniformType::_from_string_nocase(("_" + datatype).c_str());

    switch (ut) {
        // ALL THE SCALAR LADIES
    case GLSLUniformType::_bool: {
        bool dv = bool();
        if (not defaultValue.empty()) std::istringstream(defaultValue) >> dv;
        return std::make_shared<GLSLUniform<bool>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_int: {
        int dv = int();
        if (not defaultValue.empty()) std::istringstream(defaultValue) >> dv;
        return std::make_shared<GLSLUniform<int>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_uint: {
        unsigned int dv = uint32_t();
        if (not defaultValue.empty()) std::istringstream(defaultValue) >> dv;
        return std::make_shared<GLSLUniform<unsigned int>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_float: {
        float dv = float();
        if (not defaultValue.empty()) std::istringstream(defaultValue) >> dv;
        return std::make_shared<GLSLUniform<float>>(ut, varname, false, dv);
    }

    case GLSLUniformType::_double: {
        double dv = double();
        if (not defaultValue.empty()) std::istringstream(defaultValue) >> dv;
        return std::make_shared<GLSLUniform<double>>(ut, varname, false, dv);
    }

                                 // Bool vectors
    case GLSLUniformType::_bvec2: {
        bvec2 dv = bvec2();
        if (not defaultValue.empty()) dv = glm::from_string<2, bool, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<bvec2>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_bvec3: {
        bvec3 dv = bvec3();
        if (not defaultValue.empty()) dv = glm::from_string<3, bool, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<bvec3>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_bvec4: {
        bvec4 dv = bvec4();
        if (not defaultValue.empty()) dv = glm::from_string<4, bool, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<bvec4>>(ut, varname, false, dv);
    }

                                // Int vectors
    case GLSLUniformType::_ivec2: {
        ivec2 dv = ivec2();
        if (not defaultValue.empty()) dv = glm::from_string<2, int, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<ivec2>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_ivec3: {
        ivec3 dv = ivec3();
        if (not defaultValue.empty()) dv = glm::from_string<3, int, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<ivec3>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_ivec4: {
        ivec4 dv = ivec4();
        if (not defaultValue.empty()) dv = glm::from_string<4, int, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<ivec4>>(ut, varname, false, dv);
    }

                                // Unsigned int vectors
    case GLSLUniformType::_uvec2: {
        uvec2 dv = uvec2();
        if (not defaultValue.empty()) dv = glm::from_string<2, unsigned int, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<uvec2>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_uvec3: {
        uvec3 dv = uvec3();
        if (not defaultValue.empty()) dv = glm::from_string<3, unsigned int, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<uvec3>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_uvec4: {
        uvec4 dv = uvec4();
        if (not defaultValue.empty()) dv = glm::from_string<4, unsigned int, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<uvec4>>(ut, varname, false, dv);
    }
                                // Float vectors
    case GLSLUniformType::_vec2: {
        vec2 dv = vec2();
        if (not defaultValue.empty()) dv = glm::from_string<2, float, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<vec2>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_vec3: {
        vec3 dv = vec3();
        if (not defaultValue.empty()) dv = glm::from_string<3, float, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<vec3>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_vec4: {
        vec4 dv = vec4();
        if (not defaultValue.empty()) dv = glm::from_string<4, float, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<vec4>>(ut, varname, false, dv);
    }
                               // Double vectors
    case GLSLUniformType::_dvec2: {
        dvec2 dv = dvec2();
        if (not defaultValue.empty()) dv = glm::from_string<2, double, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<dvec2>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_dvec3: {
        dvec3 dv = dvec3();
        if (not defaultValue.empty()) dv = glm::from_string<3, double, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<dvec3>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_dvec4: {
        dvec4 dv = dvec4();
        if (not defaultValue.empty()) dv = glm::from_string<4, double, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<dvec4>>(ut, varname, false, dv);
    }
                                // Matrices
    case GLSLUniformType::_mat2: {
        mat2 dv = mat2();
        if (not defaultValue.empty()) dv = glm::from_string<2, 2, float, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<mat2>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_mat3: {
        mat3 dv = mat3();
        if (not defaultValue.empty()) dv = glm::from_string<3, 3, float, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<mat3>>(ut, varname, false, dv);
    }
    case GLSLUniformType::_mat4: {
        mat4 dv = mat4();
        if (not defaultValue.empty()) dv = glm::from_string<4, 4, float, glm::defaultp>(defaultValue);
        return std::make_shared<GLSLUniform<mat4>>(ut, varname, false, dv);
    }

                               // Samplers
    case GLSLUniformType::_sampler2D:
    case GLSLUniformType::_sampler2DShadow: {
        return std::make_shared<GLSLUniform<GLuint>>(ut, varname, false);
    }
    default:
        return nullptr;
    }

}

UniformMap GLSLUniformBase::processUniforms(const std::string& shaderText) {
    auto results = std::map<std::string, s_ptr<GLSLUniformBase>>();

    const std::regex uniformRegex(R"TOOTHMAN(uniform\s+(\S+)\s+(\S+)\s*(?:=\s*(.+))?;)TOOTHMAN",
        std::regex_constants::ECMAScript);

    std::smatch baseMatches;

    auto searchStart = shaderText.cbegin();

    while (std::regex_search(searchStart, shaderText.cend(), baseMatches, uniformRegex)) {
        //for(size_t i = 0; i < baseMatches.size(); i+=3) {
        size_t i = 0;
        std::ssub_match datatypeMatch = baseMatches[i + 1];
        std::ssub_match varnameMatch = baseMatches[i + 2];
        log("Uniform: {0} ({1})\n", varnameMatch.str(), datatypeMatch.str());

        std::string defaultValue = "";

        if (baseMatches.size() > i + 2) {
            log("\t baseMatches size: {0}\n", baseMatches.size());
            for (size_t j = 0; j < baseMatches.size(); j++) {
                log("\t baseMatch {0}: {1}\n", j, baseMatches[j].str());
            }
            log("\tDefault value: {0}\n", baseMatches[i + 3].str());
            defaultValue = baseMatches[i + 3].str();
        }

        results[varnameMatch.str()] = createUniform(datatypeMatch.str(), varnameMatch.str(), defaultValue);
        //}

        searchStart = baseMatches.suffix().first;
    }

    return results;
}