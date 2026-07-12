#pragma once

/*!
@file       Model.h
@author     nurnadianatasya.b@digipen.edu
@date       13/07/2026

@brief
Abstract lifecycle every renderable shape must implement. Renderer owns a
collection of these and never knows the concrete type behind any of them -
that is the whole point of the interface.
*//*__________________________________________________________________________*/
class Model {
public:
    Model() {}
    virtual ~Model() {}

    virtual void InitModel()    = 0;   // called once, before the first frame
    virtual void Render()       = 0;   // called once per frame
    virtual void Resize(int w, int h) {}   // called whenever the surface/window resizes

    // Touch/mouse pipeline (MotionEvent -> queueEvent -> JNI -> Renderer, or
    // desktop/web mouse mapping in main.cpp). Default no-ops so Models that
    // don't care about input need not override any of them.
    virtual void TouchEventDown(float x, float y)    {}
    virtual void TouchEventMove(float x, float y)    {}
    virtual void TouchEventRelease(float x, float y) {}
};