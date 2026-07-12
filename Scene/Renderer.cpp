/*!
@file       Renderer.cpp
@author     nurnadianatasya.b@digipen.edu
@date 30/06/2026

@brief
Renderer singleton implementation. Creates and owns the scene's Fan
instance through the Model interface, initializes it once, and forwards
resize()/render() to every Model in the collection each frame.
*//*__________________________________________________________________________*/

#include "Fan.h"
#include "Renderer.h"

Renderer& Renderer::Instance() {
    static Renderer instance;
    return instance;
}

void Renderer::createModels()
{
    clearModels();
#ifdef PLATFORM_ANDROID
    models.push_back(new Fan(assetMgr));
#else
    models.push_back(new Fan());
#endif
}

void Renderer::initializeModels()
{
    for (Model* model : models)
        model->InitModel();
}

void Renderer::clearModels()
{
    for (Model* model : models)
        delete model;
    models.clear();
}

bool Renderer::initializeRenderer()
{
    createModels();
    initializeModels();
    return true;
}

Renderer::~Renderer() {
    clearModels();
}

void Renderer::resize(int w, int h) {
    for (Model* model : models)
        model->Resize(w, h);
}

void Renderer::render() {
    for (Model* model : models)
        model->Render();
}

#ifdef PLATFORM_ANDROID
void Renderer::setAssetManager(AAssetManager* mgr) { assetMgr = mgr; }
#endif
