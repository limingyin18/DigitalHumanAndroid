#include <jni.h>
#include <android/native_window_jni.h>
#include <android/asset_manager_jni.h>

#include "spdlog/sinks/android_sink.h"

#include "PoseApp.h"

static PoseApp *poseApp = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_com_ylm_myapplication_VulkanAppBridge_nativeCreate(JNIEnv *env, jobject vulkanAppBridge,
                                                     jobject surface, jobject pAssetManager)
{
    std::string tag = "pd";
    auto android_logger = spdlog::android_logger_mt("android1", tag);
    spdlog::set_default_logger(android_logger);

    if (poseApp)
    {
        delete poseApp;
        poseApp = nullptr;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "mc-native-VulkanAppBridge", "create");
    auto window = ANativeWindow_fromSurface(env, surface);
    auto assetManager = AAssetManager_fromJava(env, pAssetManager);
    poseApp = new PoseApp(assetManager, window);
    poseApp->init();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ylm_myapplication_VulkanAppBridge_nativeDestroy(JNIEnv *env, jobject vulkanAppBridge) {
    __android_log_print(ANDROID_LOG_DEBUG, "mc-native-VulkanAppBridge", "destroy");
    if (poseApp) {
        delete poseApp;
        poseApp = nullptr;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ylm_myapplication_VulkanAppBridge_nativeResize(JNIEnv *env, jobject vulkanAppBridge, jint width, jint height) {
    __android_log_print(ANDROID_LOG_DEBUG, "mc-native-VulkanAppBridge", "resize: %dx%d", width, height);
    if (poseApp) {
        //poseApp->setSize(width, height);
        //poseApp->isResizeNeeded = true;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ylm_myapplication_VulkanAppBridge_nativeDraw(JNIEnv *env, jobject vulkanAppBridge) {
    __android_log_print(ANDROID_LOG_DEBUG, "mc-native-VulkanAppBridge", "draw");
    if (poseApp) {
        poseApp->update();
    }
}