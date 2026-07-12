/*!
@file       NativeTemplate.cpp
@author     nurnadianatasya.b@digipen.edu
@date       13/07/2026

@brief
Thin JNI bridge between the Java/Android layer and Renderer::Instance().
Converts the Java AssetManager to a native AAssetManager* on init and
forwards every lifecycle call (init/resize/render) and every touch event
(down/move/release) straight through; it has no knowledge of any concrete
shape class directly - that boundary lives entirely in Scene/Renderer.cpp.

Native method signatures match MainActivity.java:
  package com.example.glpiframeworkintro
  class   MainActivity
*//*__________________________________________________________________________*/

#define LOG_TAG "GLPIFrameworkIntroNative"

#include <jni.h>
#include "Renderer.h"
#include <android/asset_manager_jni.h>

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_glpiframeworkintro_MainActivity_nativeInit(
        JNIEnv* env, jobject /*thiz*/, jobject assetManager)
{
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    Renderer::Instance().setAssetManager(mgr);
    return Renderer::Instance().initializeRenderer() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_glpiframeworkintro_MainActivity_nativeResize(
        JNIEnv* /*env*/, jobject /*thiz*/, jint width, jint height)
{
    Renderer::Instance().resize(width, height);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_glpiframeworkintro_MainActivity_nativeRender(
        JNIEnv* /*env*/, jobject /*thiz*/)
{
    Renderer::Instance().render();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_glpiframeworkintro_MainActivity_nativeTouchDown(
        JNIEnv* /*env*/, jobject /*thiz*/, jfloat x, jfloat y)
{
    Renderer::Instance().TouchEventDown(x, y);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_glpiframeworkintro_MainActivity_nativeTouchMove(
        JNIEnv* /*env*/, jobject /*thiz*/, jfloat x, jfloat y)
{
    Renderer::Instance().TouchEventMove(x, y);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_glpiframeworkintro_MainActivity_nativeTouchRelease(
        JNIEnv* /*env*/, jobject /*thiz*/, jfloat x, jfloat y)
{
    Renderer::Instance().TouchEventRelease(x, y);
}