#include <jni.h>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>
#include <opencv2/highgui.hpp>
#include <android/bitmap.h>

extern "C"
JNIEXPORT jstring JNICALL
Java_ups_com_aplicacionnativa_MainActivity_stringFromJNI(JNIEnv *env, jobject thiz) {
    // BASE METHOD
    return env->NewStringUTF("Hello from C++");
}

void bitmapToMat(JNIEnv * env, jobject bitmap, cv::Mat &dst, jboolean needUnPremultiplyAlpha){
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
            cv::Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if(needUnPremultiplyAlpha) cv::cvtColor(tmp, dst, cv::COLOR_mRGBA2RGBA);
            else tmp.copyTo(dst);
        } else {
            cv::Mat tmp(info.height, info.width, CV_8UC2, pixels);
            cv::cvtColor(tmp, dst, cv::COLOR_BGR5652RGBA);
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
        env->ThrowNew(je, "Excepción desconocida en el código JNI {nBitmapToMat}");
        return;
    }
}

void matToBitmap(JNIEnv * env, cv::Mat src, jobject bitmap, jboolean needPremultiplyAlpha) {
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
            cv::Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if(src.type() == CV_8UC1)
            {
                cv::cvtColor(src, tmp, cv::COLOR_GRAY2BGRA);
            } else if(src.type() == CV_8UC3){
                cv::cvtColor(src, tmp, cv::COLOR_RGB2BGRA);
            } else if(src.type() == CV_8UC4){
                if(needPremultiplyAlpha) cv::cvtColor(src, tmp, cv::COLOR_RGBA2mRGBA);
                else src.copyTo(tmp);
            }
        } else {
            cv::Mat tmp(info.height, info.width, CV_8UC2, pixels);
            if(src.type() == CV_8UC1)
            {
                cv::cvtColor(src, tmp, cv::COLOR_GRAY2BGR565);
            } else if(src.type() == CV_8UC3){
                cv::cvtColor(src, tmp, cv::COLOR_RGB2BGR565);
            } else if(src.type() == CV_8UC4){
                cv::cvtColor(src, tmp, cv::COLOR_RGBA2BGR565);
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
        env->ThrowNew(je, "Excepción desconocida en el código JNI {nMatToBitmap}");
        return;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_ups_com_aplicacionnativa_MainActivity_eliminarChroma(JNIEnv *env, jobject instance, jobject bitmap) {

    cv::Mat image;
    bitmapToMat(env, bitmap, image, false);

    if (image.empty()) {
        return;
    }

    // Convertir la imagen a HSV
    cv::Mat hsv;
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

    // Definir los límites del color verde en HSV
    cv::Scalar lower_green(35, 80, 80);  // Ajuste para verde
    cv::Scalar upper_green(100, 255, 255);

    // Aplicar umbralización para detectar el verde;
    cv::Mat mask;
    cv::inRange(hsv, lower_green, upper_green, mask);

    // Crear una imagen con fondo transparente
    cv::Mat result(image.size(), CV_8UC4, cv::Scalar(0, 0, 0, 0));

    cv::flip(result, result, 1);

    // Copiar solo las áreas que no son verdes
    image.copyTo(result, ~mask);

    // Asignar al canal alfa de las áreas no verdes un valor mínimo (0) para que sean totalmente transparentes
    for (int y = 0; y < result.rows; ++y) {
        for (int x = 0; x < result.cols; ++x) {
            if (mask.at<uchar>(y, x) != 0) {
                result.at<cv::Vec4b>(y, x) = cv::Vec4b(0, 0, 0, 0); // Establecer todos los canales a 0 para las áreas no verdes
            }
        }
    }

    // Convertir la Mat resultante a Bitmap y actualizar la imagen en Java
    matToBitmap(env, result, bitmap, false);

}

