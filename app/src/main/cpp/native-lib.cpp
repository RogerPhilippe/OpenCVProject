#include <jni.h>
#include <string>
#include <android/bitmap.h>
#include <android/log.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/stitching.hpp>
#include "opencv-utils.h"

void bitmapToMat(JNIEnv *env, jobject bitmap, Mat& dst, jboolean needUnPremultiplyAlpha) {
    AndroidBitmapInfo  info;
    void*              pixels = 0;

    try {
        CV_Assert( AndroidBitmap_getInfo(env, bitmap, &info) >= 0 );
        CV_Assert( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                   info.format == ANDROID_BITMAP_FORMAT_RGB_565 );
        CV_Assert( AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0 );
        CV_Assert( pixels );
        dst.create(info.height, info.width, CV_8UC4);
        if( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 )
        {
            Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if(needUnPremultiplyAlpha) cvtColor(tmp, dst, COLOR_mRGBA2RGBA);
            else tmp.copyTo(dst);
        } else {
            // info.format == ANDROID_BITMAP_FORMAT_RGB_565
            Mat tmp(info.height, info.width, CV_8UC2, pixels);
            cvtColor(tmp, dst, COLOR_BGR5652RGBA);
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch(const cv::Exception& e) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nBitmapToMat}");
        return;
    }
}

void matToBitmap(JNIEnv* env, Mat src, jobject bitmap, jboolean needPremultiplyAlpha) {
    AndroidBitmapInfo  info;
    void*              pixels = 0;

    try {
        CV_Assert( AndroidBitmap_getInfo(env, bitmap, &info) >= 0 );
        CV_Assert( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                   info.format == ANDROID_BITMAP_FORMAT_RGB_565 );
        CV_Assert( src.dims == 2 && info.height == (uint32_t)src.rows && info.width == (uint32_t)src.cols );
        CV_Assert( src.type() == CV_8UC1 || src.type() == CV_8UC3 || src.type() == CV_8UC4 );
        CV_Assert( AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0 );
        CV_Assert( pixels );
        if( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 )
        {
            Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if(src.type() == CV_8UC1)
            {
                cvtColor(src, tmp, COLOR_GRAY2RGBA);
            } else if(src.type() == CV_8UC3){
                cvtColor(src, tmp, COLOR_RGB2RGBA);
            } else if(src.type() == CV_8UC4){
                if(needPremultiplyAlpha) cvtColor(src, tmp, COLOR_RGBA2mRGBA);
                else src.copyTo(tmp);
            }
        } else {
            // info.format == ANDROID_BITMAP_FORMAT_RGB_565
            Mat tmp(info.height, info.width, CV_8UC2, pixels);
            if(src.type() == CV_8UC1)
            {
                cvtColor(src, tmp, COLOR_GRAY2BGR565);
            } else if(src.type() == CV_8UC3){
                cvtColor(src, tmp, COLOR_RGB2BGR565);
            } else if(src.type() == CV_8UC4){
                cvtColor(src, tmp, COLOR_RGBA2BGR565);
            }
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch(const cv::Exception& e) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nMatToBitmap}");
        return;
    }
}

std::string doubleToString(double value) {
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

extern "C" JNIEXPORT jstring JNICALL
Java_br_com_mc1_opencvproject_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_br_com_mc1_opencvproject_MainActivity_myFlipJNI(
        JNIEnv* env,
        jobject,
        jobject bitmapIn,
        jobject bitmapOut) {
    Mat src;
    bitmapToMat(env, bitmapIn, src, false);
    myFlip(src);
    matToBitmap(env, src, bitmapOut, false);
}

extern "C" JNIEXPORT void JNICALL
Java_br_com_mc1_opencvproject_MainActivity_myBlurJNI(
        JNIEnv* env,
        jobject,
        jobject bitmapIn,
        jobject bitmapOut,
        jfloat sigma) {
    Mat src;
    bitmapToMat(env, bitmapIn, src, false);
    myBlur(src, sigma);
    matToBitmap(env, src, bitmapOut, false);
}

extern "C" JNIEXPORT void JNICALL
        Java_br_com_mc1_opencvproject_MainActivity_stitchImagesJNI(
                JNIEnv* env,
                jobject,
                jobjectArray bitmapsIn,
                jobjectArray bitmapsOut) {


    std::vector<cv::Mat> matricesIn;
    std::vector<cv::Mat> matricesOut;
    int length = env->GetArrayLength(bitmapsIn);
    for (int i = 0; i < length; i++) {
        jobject bitmapIn = env->GetObjectArrayElement(bitmapsIn, i);
        Mat src;
        bitmapToMat(env, bitmapIn, src, false);
        matricesIn.push_back(src);
        env->DeleteLocalRef(bitmapIn);
    }

    Stitcher stitcher = Stitcher();
    stitcher.stitch(matricesIn, matricesOut);

    int index = 0;
    for(const Mat& matrix : matricesOut) {

        jclass bitmapConfigClass = env->FindClass("android/graphics/Bitmap$Config");
        jfieldID defaultBitmapConfigField = env->GetStaticFieldID(bitmapConfigClass, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
        jobject defaultBitmapConfig = env->GetStaticObjectField(bitmapConfigClass, defaultBitmapConfigField);

        jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
        jmethodID createBitmapMethod = env->GetStaticMethodID(bitmapClass, "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
        jobject out = env->CallStaticObjectMethod(bitmapClass, createBitmapMethod, matrix.cols, matrix.rows, defaultBitmapConfig);

        matToBitmap(env, matrix, out, false);
        env->SetObjectArrayElement(bitmapsOut, index, out);
        index++;

    }

}

extern "C" JNIEXPORT jboolean JNICALL
Java_br_com_mc1_opencvproject_MainActivity_blurDetectJNI(
        JNIEnv* env,
        jobject,
        jobject bitmapIn,
        jdouble threshold) {

    Mat src;
    Mat gray;

    bitmapToMat(env, bitmapIn, src, false);

    // Converta a imagem para escala de cinza
    cv::cvtColor(src,gray,cv::COLOR_BGR2GRAY);

    // Calcule o operador Laplaciano
    cv::Mat laplacian;
    cv::Laplacian(gray, laplacian, CV_64F);

    // Calcule a variância do Laplaciano
    cv::Scalar mean, stddev;
    cv::meanStdDev(laplacian, mean, stddev);

    double variance = stddev.val[0] * stddev.val[0];
    std::string result = std::string("variance: ") + std::string(doubleToString(variance));
    __android_log_print(ANDROID_LOG_DEBUG, "blurDetectJNI", "%s", result.c_str());

    if (variance < threshold) return JNI_TRUE; else return JNI_FALSE;

}