#include <jni.h>
#include <string>
#include <android/bitmap.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <vector>
#include <platform.h>
#include <benchmark.h>
#include "face.h"
#include "ndkcamera.h"


void Bitmap2Mat(JNIEnv * env, jobject bitmap, cv::Mat& dst, jboolean needUnPremultiplyAlpha)
{
    AndroidBitmapInfo  info;    // uses jnigraphics
    void*              pixels = 0;

    CV_Assert( AndroidBitmap_getInfo(env, bitmap, &info) >= 0 );
    CV_Assert( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
               info.format == ANDROID_BITMAP_FORMAT_RGB_565 );
    CV_Assert( AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0 );
    CV_Assert( pixels );
    dst.create(info.height, info.width, CV_8UC4);
    auto stop = 1;

    if( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 )
    {
        cv::Mat tmp(info.height, info.width, CV_8UC4, pixels);
        if(needUnPremultiplyAlpha) cvtColor(tmp, dst, cv::COLOR_mRGBA2RGBA);
        else tmp.copyTo(dst);
    } else {
        cv::Mat tmp(info.height, info.width, CV_8UC2, pixels);
        cvtColor(tmp, dst, cv::COLOR_BGR5652RGBA);
    }
    AndroidBitmap_unlockPixels(env, bitmap);
    return;
}

jobject Mat2Bitmap(JNIEnv * env, cv::Mat& src, bool needPremultiplyAlpha, jobject bitmap_config){
    jclass java_bitmap_class = (jclass)env->FindClass("android/graphics/Bitmap");
    jmethodID mid = env->GetStaticMethodID(java_bitmap_class,
                                           "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");

    jobject bitmap = env->CallStaticObjectMethod(java_bitmap_class,
                                                 mid, src.size().width, src.size().height, bitmap_config);
    AndroidBitmapInfo  info;
    void* pixels = 0;


    //validate
    CV_Assert(AndroidBitmap_getInfo(env, bitmap, &info) >= 0);
    CV_Assert(src.type() == CV_8UC1 || src.type() == CV_8UC3 || src.type() == CV_8UC4);
    CV_Assert(AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0);
    CV_Assert(pixels);

    //type mat
    if(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888){
        cv::Mat tmp(info.height, info.width, CV_8UC4, pixels);
        if(src.type() == CV_8UC1){
            cvtColor(src, tmp, cv::COLOR_GRAY2RGBA);
        } else if(src.type() == CV_8UC3){
            cvtColor(src, tmp, cv::COLOR_RGB2RGBA);
        } else if(src.type() == CV_8UC4){
            if(needPremultiplyAlpha){
                cvtColor(src, tmp, cv::COLOR_RGBA2mRGBA);
            }else{
                src.copyTo(tmp);
            }
        }

    } else{
        cv::Mat tmp(info.height, info.width, CV_8UC2, pixels);
        if(src.type() == CV_8UC1){
            cvtColor(src, tmp, cv::COLOR_GRAY2BGR565);
        } else if(src.type() == CV_8UC3){
            cvtColor(src, tmp, cv::COLOR_RGB2BGR565);
        } else if(src.type() == CV_8UC4){
            cvtColor(src, tmp, cv::COLOR_RGBA2BGR565);
        }
    }
    AndroidBitmap_unlockPixels(env, bitmap);
    return bitmap;

}

static ncnn::Mutex lock;
static FaceDetection* face_detection = 0;

static int draw_fps(cv::Mat& rgb)
{
    // resolve moving average
    float avg_fps = 0.f;
    {
        static double t0 = 0.f;
        static float fps_history[10] = {0.f};

        double t1 = ncnn::get_current_time();
        if (t0 == 0.f)
        {
            t0 = t1;
            return 0;
        }

        float fps = 1000.f / (t1 - t0);
        t0 = t1;

        for (int i = 9; i >= 1; i--)
        {
            fps_history[i] = fps_history[i - 1];
        }
        fps_history[0] = fps;

        if (fps_history[9] == 0.f)
        {
            return 0;
        }

        for (int i = 0; i < 10; i++)
        {
            avg_fps += fps_history[i];
        }
        avg_fps /= 10.f;
    }

    char text[32];
    sprintf(text, "FPS=%.2f", avg_fps);

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    int y = 0;
    int x = rgb.cols - label_size.width;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                  cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));

    return 0;
}

class MyNdkCamera : public NdkCameraWindow
{
public:
    virtual void on_image_render(cv::Mat& rgb) const;
};

void MyNdkCamera::on_image_render(cv::Mat& rgb) const
{
    {
        ncnn::MutexLockGuard g(lock);

        const int max_side = 480;
        float long_side = std::max(rgb.cols, rgb.rows);
        float scale = max_side/long_side;

        std::vector<bbox> boxes;
        bbox oneBox;

        face_detection->Detect(rgb, boxes);

        cv::Mat res;
        if (boxes.size() > 0){
            oneBox = face_detection->chooseOneFace(boxes, scale);
            res = face_detection->alignFace(rgb, oneBox);
            face_detection->draw(rgb, oneBox, res);
        }
    }
    draw_fps(rgb);
}

static MyNdkCamera* g_camera = 0;

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "JNI_OnLoad");

    g_camera = new MyNdkCamera;

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved)
{
    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "JNI_OnUnload");

    {
        ncnn::MutexLockGuard g(lock);
    }

    delete g_camera;
    g_camera = 0;
}


JNIEXPORT jboolean JNICALL Java_com_example_face_1det_1mobapp_CameraModule_loadModel(JNIEnv* env, jobject thiz, jobject assetManager)
{
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "loadModel %p", mgr);
    face_detection = new FaceDetection;
    face_detection->loadModel(mgr);
    return JNI_TRUE;

}

// public native boolean openCamera(int facing);
JNIEXPORT jboolean JNICALL Java_com_example_face_1det_1mobapp_CameraModule_openCamera(JNIEnv* env, jobject thiz, jint facing)
{
    if (facing < 0 || facing > 1)
        return JNI_FALSE;

    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "openCamera %d", facing);

    g_camera->open((int)facing);

    return JNI_TRUE;
}

// public native boolean closeCamera();
JNIEXPORT jboolean JNICALL Java_com_example_face_1det_1mobapp_CameraModule_closeCamera(JNIEnv* env, jobject thiz)
{
    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "closeCamera");

    g_camera->close();

    return JNI_TRUE;
}

// public native boolean setOutputWindow(Surface surface);
JNIEXPORT jboolean JNICALL Java_com_example_face_1det_1mobapp_CameraModule_setOutputWindow(JNIEnv* env, jobject thiz, jobject surface)
{
    ANativeWindow* win = ANativeWindow_fromSurface(env, surface);

    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "setOutputWindow %p", win);

    g_camera->set_window(win);

    return JNI_TRUE;
}
}

//extern "C" JNIEXPORT jobject JNICALL Java_com_example_face_1det_1mobapp_MainActivity_stringFromJNI(
//        JNIEnv* env,
//        jobject,
//        jobject bitmapIn,
//        jobject assetManager) {
//    cv::Mat img;
//    Bitmap2Mat(env, bitmapIn, img, false);
//    cv::Mat resized_src;
//    cv::resize(img, resized_src, cv::Size(960, 1280), cv::INTER_LINEAR);
//    cv::cvtColor(resized_src, resized_src, cv::COLOR_RGB2BGR);
//
//    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
//
//    const int max_side = 640;
//    float long_side = std::max(resized_src.cols, resized_src.rows);
//    float scale = max_side/long_side;
//    cv::Mat img_scale;
//    cv::resize(resized_src, img_scale, cv::Size(resized_src.cols*scale, resized_src.rows*scale));
//
//
//    std::vector<bbox> boxes;
//    bbox oneBox;
//
//    Detect(img_scale, boxes, mgr);
//
//    cv::Mat res;
//    if (boxes.size() > 0){
//        oneBox = chooseOneFace(boxes, scale);
//        res = alignFace(resized_src, oneBox);
//    }
//
//    jclass java_bitmap_class = (jclass)env->FindClass("android/graphics/Bitmap");
//    jmethodID mid = env->GetMethodID(java_bitmap_class, "getConfig", "()Landroid/graphics/Bitmap$Config;");
//    jobject bitmap_config = env->CallObjectMethod(bitmapIn, mid);
//
//
//    return Mat2Bitmap(env, res, false, bitmap_config);
//}
