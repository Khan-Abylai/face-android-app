package com.example.face_det_mobapp;

import android.content.res.AssetManager;
import android.view.Surface;

public class CameraModule {

    public native boolean loadModel(AssetManager mgr);
    public native boolean openCamera(int facing);
    public native boolean closeCamera();
    public native boolean setOutputWindow(Surface surface);

    static {
        System.loadLibrary("face_det_mobapp");
    }
}
