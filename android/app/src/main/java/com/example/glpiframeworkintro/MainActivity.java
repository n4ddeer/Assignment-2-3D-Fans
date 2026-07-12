package com.example.glpiframeworkintro;

import androidx.appcompat.app.AppCompatActivity;

import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/*!
@file       MainActivity.java
@author     nurnadianatasya.b@digipen.edu
@date 30/06/2026

@brief
Renders the 3D Fan via the shared Model/Renderer architecture in Scene/.
Touch gestures (tap to toggle, drag to boost spin speed) are forwarded to
native code via queueEvent() so they run on the GL thread.
*//*__________________________________________________________________________*/
public class MainActivity extends AppCompatActivity {

    private static final String TAG = "GLPIFrameworkIntro";

    static {
        System.loadLibrary("glpiframeworkintro");
    }

    private GLSurfaceView glSurfaceView;
    private boolean rendererInitialized = false;

    private native boolean nativeInit(AssetManager assetManager);
    private native void    nativeResize(int width, int height);
    private native void    nativeRender();
    private native void    nativeTouchDown(float x, float y);
    private native void    nativeTouchMove(float x, float y);
    private native void    nativeTouchRelease(float x, float y);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.activity_main);

        glSurfaceView = findViewById(R.id.gl_surface_view);
        glSurfaceView.setEGLContextClientVersion(3);

        glSurfaceView.setRenderer(new GLSurfaceView.Renderer() {
            @Override
            public void onSurfaceCreated(GL10 gl, EGLConfig config) {
                Log.d(TAG, "onSurfaceCreated");
                boolean ok = nativeInit(getAssets());
                if (ok) {
                    rendererInitialized = true;
                    Log.d(TAG, "Renderer initialized");
                } else {
                    Log.e(TAG, "Renderer initialization failed");
                }
            }

            @Override
            public void onSurfaceChanged(GL10 gl, int width, int height) {
                Log.d(TAG, "onSurfaceChanged: " + width + "x" + height);
                if (rendererInitialized) nativeResize(width, height);
            }

            @Override
            public void onDrawFrame(GL10 gl) {
                if (rendererInitialized) nativeRender();
            }
        });

        glSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

        // Forward touch events to native code - must run on the GL thread.
        glSurfaceView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (!rendererInitialized) return true;

                final float x = event.getX();
                final float y = event.getY();

                switch (event.getActionMasked()) {
                    case MotionEvent.ACTION_DOWN:
                        glSurfaceView.queueEvent(() -> nativeTouchDown(x, y));
                        break;
                    case MotionEvent.ACTION_MOVE:
                        glSurfaceView.queueEvent(() -> nativeTouchMove(x, y));
                        break;
                    case MotionEvent.ACTION_UP:
                    case MotionEvent.ACTION_CANCEL:
                        glSurfaceView.queueEvent(() -> nativeTouchRelease(x, y));
                        break;
                }
                return true;
            }
        });
    }

    @Override
    protected void onPause()  { super.onPause();  glSurfaceView.onPause();  }

    @Override
    protected void onResume() { super.onResume(); glSurfaceView.onResume(); }
}
