#pragma once
#include "Model.h"
#include <vector>

#ifdef PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif

/*!
@file       Renderer.h
@author     nurnadianatasya.b@digipen.edu
@date 30/06/2026

@brief
Singleton Renderer that owns the scene's collection of Model instances. It
is the only class main.cpp / NativeTemplate.cpp talk to; this header never
includes Triangle.h or Square.h - only Renderer.cpp knows about concrete
shapes.
*//*__________________________________________________________________________*/
class Renderer {
public:
    static Renderer& Instance();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

#ifdef PLATFORM_ANDROID
    void setAssetManager(AAssetManager* mgr);
#endif

    bool initializeRenderer();
    void resize(int w, int h);
    void render();

private:
    Renderer() = default;
    ~Renderer();

    void createModels();
    void initializeModels();
    void clearModels();

#ifdef PLATFORM_ANDROID
    AAssetManager* assetMgr = nullptr;
#endif

    std::vector<Model*> models;   // replaces the single Model* model
};