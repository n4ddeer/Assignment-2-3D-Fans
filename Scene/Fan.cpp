/*!
@file       Fan.cpp
@author     nurnadianatasya.b@digipen.edu
@date       13/07/2026

@brief
Implements the Fan model. The 8-vertex/36-index cube is uploaded once
(positions + per-corner grayscale shade, two sub-regions of one VBO, plus
an IBO) and recorded into one VAO. Render() walks the scene graph on the
Transform push/pop stack every frame: a shared "world" pull-back + tilt,
then base / pole / hub / four blades, each tinted through the PARTCOLOR
uniform before its glDrawElements call. spinAngle advances by kBaseSpeed
(+ dragBoost while dragging) at the top of Render(), driving the blade
rotation.

Gesture handling: TouchEventDown records the starting position and resets
movedDistance; TouchEventMove accumulates movedDistance for tap detection
and measures per-event drag velocity (via std::chrono) to compute
dragBoost, clamped to kMaxBoost; TouchEventRelease toggles fanOn (logging
"Fan ON"/"Fan OFF") if the total movement stayed under kTapThreshold, and
always resets dragBoost to 0 so the boost only applies while dragging.
*//*__________________________________________________________________________*/

#include "Fan.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Cube geometry — 8 corners, grayscale shade per corner (front ~1.0, back ~0.6),
// 36 indices (6 faces * 2 triangles * 3 indices).
// ---------------------------------------------------------------------------

static const GLfloat kPositions[8 * 3] = {
    -0.5f, -0.5f,  0.5f,   // 0 front-bottom-left
     0.5f, -0.5f,  0.5f,   // 1 front-bottom-right
     0.5f,  0.5f,  0.5f,   // 2 front-top-right
    -0.5f,  0.5f,  0.5f,   // 3 front-top-left
    -0.5f, -0.5f, -0.5f,   // 4 back-bottom-left
     0.5f, -0.5f, -0.5f,   // 5 back-bottom-right
     0.5f,  0.5f, -0.5f,   // 6 back-top-right
    -0.5f,  0.5f, -0.5f    // 7 back-top-left
};

static const GLfloat kShades[8] = {
    1.0f, 1.0f, 1.0f, 1.0f,   // front corners
    0.6f, 0.6f, 0.6f, 0.6f    // back corners
};

static const GLushort kIndices[36] = {
    0, 1, 2,  2, 3, 0,   // front
    1, 5, 6,  6, 2, 1,   // right
    5, 4, 7,  7, 6, 5,   // back
    4, 0, 3,  3, 7, 4,   // left
    3, 2, 6,  6, 7, 3,   // top
    4, 5, 1,  1, 0, 4    // bottom
};

// ---------------------------------------------------------------------------
// Part colours
// ---------------------------------------------------------------------------

static const glm::vec3 kBaseColor  (0.45f, 0.28f, 0.12f);
static const glm::vec3 kPoleColor  (0.55f, 0.55f, 0.58f);
static const glm::vec3 kHubColor   (0.20f, 0.20f, 0.22f);
static const glm::vec3 kBladeColors[4] = {
    glm::vec3(1.0f, 0.0f, 0.0f),   // red
    glm::vec3(0.0f, 0.0f, 1.0f),   // blue
    glm::vec3(1.0f, 0.5f, 0.0f),   // orange
    glm::vec3(0.0f, 1.0f, 0.0f)    // green
};

// ---------------------------------------------------------------------------
// Fan
// ---------------------------------------------------------------------------

#ifdef PLATFORM_ANDROID
Fan::Fan(AAssetManager* assetMgr) : mgr(assetMgr) {}
#else
Fan::Fan() = default;
#endif

Fan::~Fan()
{
    glDeleteBuffers(1, &ibo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(programID);
}

void Fan::InitModel()
{
    LOGI("Fan::InitModel");

    // Zeroes/identities all four matrix stacks (model, view, projection,
    // texture) and sets the baseline GL state (depth test, CCW front face,
    // back-face culling) the Transform stack is designed to run under.
    transform.TransformInit();

#ifdef PLATFORM_ANDROID
    programID = ShaderHelper::buildProgramFromAssets(mgr, "shader/FanVertex.glsl",
                                                   "shader/FanFragment.glsl");
#else
    programID = ShaderHelper::buildProgramFromFile("FanVertex.glsl",
                                                 "FanFragment.glsl");
#endif

    if (!programID) { LOGE("Fan: could not create program"); return; }

    uMVP       = glGetUniformLocation(programID, "MODELVIEWPROJECTIONMATRIX");
    uPartColor = glGetUniformLocation(programID, "PARTCOLOR");

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // One VBO, two sub-regions: positions, then shades.
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kPositions) + sizeof(kShades), nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,                sizeof(kPositions), kPositions);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(kPositions), sizeof(kShades),  kShades);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0,
                           reinterpret_cast<void*>(sizeof(kPositions)));

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices, GL_STATIC_DRAW);

    // Seal the VAO: unbind it first, then the buffers. The IBO binding was
    // captured as VAO state while vao was bound, so unbinding it now only
    // touches the default (0) VAO's binding, not this one's.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    LOGI("Fan::InitModel done");
}

void Fan::drawPart(const glm::vec3& partColor)
{
    glUniformMatrix4fv(uMVP, 1, GL_FALSE,
                        glm::value_ptr(*transform.TransformGetModelViewProjectionMatrix()));
    glUniform3fv(uPartColor, 1, glm::value_ptr(partColor));
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);
}

void Fan::Render()
{
    // Advance the blade angle. fanOn is toggled by TouchEventRelease and
    // dragBoost is set/reset by TouchEventMove/Release below.
    if (fanOn) spinAngle += kBaseSpeed + dragBoost;
    if (spinAngle >= 360.0f) spinAngle -= 360.0f;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(programID);
    glBindVertexArray(vao);

    transform.TransformSetMatrixMode(MODEL_MATRIX);
    transform.TransformLoadIdentity();

    // world: shared pull-back + 3D tilt, every part starts from here.
    transform.TransformTranslate(0.0f, 0.8f, -8.0f);
    transform.TransformRotate(glm::radians(20.0f), 0.0f, 1.0f, 0.0f);

    // base
    transform.TransformPushMatrix();
        transform.TransformTranslate(0.0f, -2.6f, 0.0f);
        transform.TransformScale(1.6f, 0.25f, 0.8f);
        drawPart(kBaseColor);
    transform.TransformPopMatrix();

    // pole
    transform.TransformPushMatrix();
        transform.TransformTranslate(0.0f, -1.21f, 0.0f);
        transform.TransformScale(0.15f, 2.53f, 0.15f);
        drawPart(kPoleColor);
    transform.TransformPopMatrix();

    // hub
    transform.TransformPushMatrix();
        transform.TransformTranslate(0.0f, 0.2f, 0.0f);
        transform.TransformScale(0.3f, 0.3f, 0.3f);
        drawPart(kHubColor);
    transform.TransformPopMatrix();

    // blades — rotate around the hub's Z axis, 90 deg apart, driven by spinAngle
    for (int i = 0; i < 4; ++i) {
        transform.TransformPushMatrix();
            transform.TransformTranslate(0.0f, 0.2f, 0.15f);
            transform.TransformRotate(glm::radians(spinAngle + i * 90.0f), 0.0f, 0.0f, 1.0f);
            transform.TransformTranslate(0.0f, 0.55f, 0.0f);
            transform.TransformScale(0.22f, 0.8f, 0.05f);
            drawPart(kBladeColors[i]);
        transform.TransformPopMatrix();
    }

    glBindVertexArray(0);
}

void Fan::Resize(int w, int h)
{
    glViewport(0, 0, w, h);
    aspect = (h != 0) ? static_cast<float>(w) / static_cast<float>(h) : 1.0f;

    transform.TransformSetMatrixMode(PROJECTION_MATRIX);
    transform.TransformLoadIdentity();
    transform.TransformSetPerspective(glm::radians(60.0f), aspect, 0.01f, 1000.0f, 0.0f);
}

// ---------------------------------------------------------------------------
// Gestures — a tap (down+release, little movement) toggles fanOn; a drag's
// speed (pixels/ms since the last Move) boosts the spin while held.
// ---------------------------------------------------------------------------

void Fan::TouchEventDown(float x, float y)
{
    lastX = x;
    lastY = y;
    movedDistance = 0.0f;
    lastMoveTime = std::chrono::steady_clock::now();
}

void Fan::TouchEventMove(float x, float y)
{
    float dx = x - lastX;
    float dy = y - lastY;
    float dist = std::sqrt(dx * dx + dy * dy);   // pixels this event
    movedDistance += dist;

    auto now  = std::chrono::steady_clock::now();
    float dtMs = std::chrono::duration<float, std::milli>(now - lastMoveTime).count();
    lastMoveTime = now;

    if (dtMs > 0.0f) {
        float velocity = dist / dtMs;   // pixels per millisecond
        dragBoost = std::min(velocity * kBoostScale, kMaxBoost);
        LOGI("Fan drag velocity=%.3f px/ms -> boost=%.2f", velocity, dragBoost);
    }

    lastX = x;
    lastY = y;
}

void Fan::TouchEventRelease(float x, float y)
{
    (void)x;
    (void)y;
    if (movedDistance < kTapThreshold) {
        fanOn = !fanOn;
        LOGI("Fan %s", fanOn ? "ON" : "OFF");
    }
    dragBoost = 0.0f;   // boost applies only while dragging
}
