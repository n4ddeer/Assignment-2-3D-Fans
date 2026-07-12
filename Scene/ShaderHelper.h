#pragma once
#include "Platform.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#ifdef PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif

/*!
@file       ShaderHelper.h
@author     nurnadianatasya.b@digipen.edu
@date 30/06/2026

@brief
Shared shader compile/link helpers used by every Model. Supports both the
"stringify" method (inline C-string source, via buildProgram) and the
file/asset-based method (buildProgramFromFile on Desktop/Web,
buildProgramFromAssets on Android), both funneling into the same
compileShader/linkProgram core.
*//*__________________________________________________________________________*/
class ShaderHelper {
public:
    static GLuint compileShader(GLenum type, const char* src);
    static GLuint linkProgram(GLuint vert, GLuint frag);
    static GLuint buildProgram(const char* vertSrc, const char* fragSrc);

#ifdef PLATFORM_ANDROID
    static std::string loadAsset(AAssetManager* mgr, const char* path);
    static GLuint buildProgramFromAssets(AAssetManager* mgr, const char* vertPath, const char* fragPath);
#else
    static std::string loadFile(const char* filename);
    static GLuint buildProgramFromFile(const char* vertFile, const char* fragFile);
#endif
};

inline GLuint ShaderHelper::compileShader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    if (!shader) { LOGE("glCreateShader failed (type=0x%x)", type); return 0; }
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::string log(static_cast<size_t>(len), '\0');
        glGetShaderInfoLog(shader, len, nullptr, &log[0]);
        LOGE("Shader compile error:\n%s", log.c_str());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

inline GLuint ShaderHelper::linkProgram(GLuint vert, GLuint frag)
{
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> buf(len + 1);
        glGetProgramInfoLog(prog, len, nullptr, buf.data());
        LOGE("Program link error:\n%s", buf.data());
        glDeleteProgram(prog);
        prog = 0;
    }

    glDetachShader(prog, vert);
    glDetachShader(prog, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

inline GLuint ShaderHelper::buildProgram(const char* vertSrc, const char* fragSrc)
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc);
    if (!vs) return 0;

    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!fs) { glDeleteShader(vs); return 0; }

    return linkProgram(vs, fs);
}

#ifndef PLATFORM_ANDROID

inline std::string ShaderHelper::loadFile(const char* filename)
{
    std::vector<std::string> candidates = {
        std::string("assets/shader/") + filename,
        std::string(filename),
        std::string("shader/") + filename,
        std::string("assets/") + filename
    };

    for (const auto& path : candidates) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    }

    LOGE("ShaderHelper::loadFile - could not find '%s' in any candidate path", filename);
    return std::string(); // empty string signals failure
}

inline GLuint ShaderHelper::buildProgramFromFile(const char* vertFile, const char* fragFile)
{
    std::string vertSrc = loadFile(vertFile);
    std::string fragSrc = loadFile(fragFile);

    if (vertSrc.empty() || fragSrc.empty()) {
        LOGE("ShaderHelper::buildProgramFromFile - failed to load shader source");
        return 0;
    }

    return buildProgram(vertSrc.c_str(), fragSrc.c_str());
}

#endif

#ifdef PLATFORM_ANDROID

inline std::string ShaderHelper::loadAsset(AAssetManager* mgr, const char* path)
{
    if (!mgr) {
        LOGE("ShaderHelper::loadAsset - null AAssetManager");
        return std::string();
    }

    AAsset* asset = AAssetManager_open(mgr, path, AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("ShaderHelper::loadAsset - could not open asset '%s'", path);
        return std::string();
    }

    size_t length = AAsset_getLength(asset);
    std::string contents(length, '\0');
    AAsset_read(asset, &contents[0], length);
    AAsset_close(asset);

    return contents;
}

inline GLuint ShaderHelper::buildProgramFromAssets(AAssetManager* mgr, const char* vertPath, const char* fragPath)
{
    std::string vertSrc = loadAsset(mgr, vertPath);
    std::string fragSrc = loadAsset(mgr, fragPath);

    if (vertSrc.empty() || fragSrc.empty()) {
        LOGE("ShaderHelper::buildProgramFromAssets - failed to load shader source");
        return 0;
    }

    return buildProgram(vertSrc.c_str(), fragSrc.c_str());
}

#endif