package com.example.face_det_mobapp;

import android.Manifest;
import android.app.Activity;
import androidx.appcompat.app.AppCompatActivity;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.Spinner;
import android.view.View;
import android.view.WindowManager;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.util.Log;

import com.example.face_det_mobapp.databinding.ActivityMainBinding;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

public class MainActivity extends Activity implements SurfaceHolder.Callback
{
    public static final int REQUEST_CAMERA = 100;

    private CameraModule scrfdncnn = new CameraModule();
    private int facing = 0;

    private Spinner spinnerModel;
    private Spinner spinnerCPUGPU;

    private int current_model = 0;
    private int current_cpugpu = 0;

    private SurfaceView cameraView;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        cameraView = (SurfaceView) findViewById(R.id.cameraview);

        cameraView.getHolder().setFormat(PixelFormat.RGBA_8888);
        cameraView.getHolder().addCallback(this);

        Button buttonSwitchCamera = (Button) findViewById(R.id.buttonSwitchCamera);
        buttonSwitchCamera.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {

                int new_facing = 1 - facing;

                scrfdncnn.closeCamera();

                scrfdncnn.openCamera(new_facing);

                facing = new_facing;
            }
        });

        boolean ret_init = scrfdncnn.loadModel(getAssets());
        if (!ret_init)
        {
            Log.e("MainActivity", "scrfdncnn loadModel failed");
        }
    }


    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
    {
        scrfdncnn.setOutputWindow(holder.getSurface());
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder)
    {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
    }

    @Override
    public void onResume()
    {
        super.onResume();

        if (ContextCompat.checkSelfPermission(getApplicationContext(), Manifest.permission.CAMERA) == PackageManager.PERMISSION_DENIED)
        {
            ActivityCompat.requestPermissions(this, new String[] {Manifest.permission.CAMERA}, REQUEST_CAMERA);
        }

        scrfdncnn.openCamera(facing);
    }


    @Override
    public void onPause()
    {
        super.onPause();

        scrfdncnn.closeCamera();
    }
}


//public class MainActivity extends AppCompatActivity {
//
//    // Used to load the 'face_det_mobapp' library on application startup.
//    static {
//        System.loadLibrary("face_det_mobapp");
//    }
//
//    private ActivityMainBinding binding;
//
//    @Override
//    protected void onCreate(Bundle savedInstanceState) {
//        super.onCreate(savedInstanceState);
//
//        binding = ActivityMainBinding.inflate(getLayoutInflater());
//        setContentView(binding.getRoot());
//
//        Button btn_classify = (Button) findViewById(R.id.bt_detect);
//        btn_classify.setOnClickListener(new View.OnClickListener() {
//            public void onClick(View v) {
//                Resources res = getResources();
//                Bitmap bitmapIn = BitmapFactory.decodeResource(res, R.drawable.aika);
//                ImageView imgres = (ImageView) findViewById(R.id.imageView2);
//                imgres.setImageBitmap(stringFromJNI(bitmapIn, getAssets()));
//            }
//        });
//
//    }
//
//    /**
//     * A native method that is implemented by the 'face_det_mobapp' native library,
//     * which is packaged with this application.
//     */
//    public native Bitmap stringFromJNI(Bitmap bitmapIn,
//                                        AssetManager assetManager);
//}