#pragma once
#include "Platform.h"
#include "Model.h"
#include "ShaderHelper.h"
#include "Transform.h"
#include <chrono>
#include <glm/glm.hpp>

#ifdef PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif

/*!
@file       Fan.h
@author     nurnadianatasya.b@digipen.edu
@date       12/07/2026

@brief
Part hierarchy (one shared cube, drawn 7x with a different model matrix
via the Transform push/pop stack, tinted through PARTCOLOR):

              [B3]
               |
        [B2]--(hub)--[B0]     blades rotate around the hub's Z axis,
               |               90 deg apart, driven by spinAngle
              [B1]
               |
              pole             pole connects hub to base
               |
           ___base___          base sits on the "table"
*//*__________________________________________________________________________*/
class Fan : public Model {
public:
#ifdef PLATFORM_ANDROID
    explicit Fan(AAssetManager* assetMgr);
#else
    Fan();
#endif
    ~Fan() override;

    void InitModel()          override;
    void Render()             override;
    void Resize(int w, int h) override;

private:
    void drawPart(const glm::vec3& partColor);

#ifdef PLATFORM_ANDROID
    AAssetManager* mgr = nullptr;
#endif

    // ---- Cube geometry / GL objects (one cube, drawn 7x) --------------------
    GLuint programID  = 0;
    GLuint vao        = 0;
    GLuint vbo        = 0;   // positions + per-corner shades, two sub-regions
    GLuint ibo        = 0;
    GLint  uMVP       = -1;
    GLint  uPartColor = -1;

    float aspect = 1.0f;     // set in Resize(), used to rebuild the projection

    // ---- Core state (given in spec) ----------------------------------------
    Transform transform;              // provided matrix stack (model/view/projection)
    float spinAngle = 0.0f;           // current blade angle (degrees)
    bool  fanOn     = true;           // toggled by a tap
    float dragBoost = 0.0f;           // extra speed while dragging, 0 otherwise

    static constexpr float kBaseSpeed    = 1.5f;   // deg/frame when ON
    static constexpr float kMaxBoost     = 20.0f;  // clamp for the drag boost
    static constexpr float kBoostScale   = 8.0f;   // px/ms -> deg/frame
    static constexpr float kTapThreshold = 12.0f;  // px: below this it's a tap

    float lastX, lastY;               // previous touch position
    float movedDistance;              // total movement since touch-down
    std::chrono::steady_clock::time_point lastMoveTime;
};
