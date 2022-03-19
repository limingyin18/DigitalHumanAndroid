#include <jni.h>
#include <android/native_window_jni.h>
#include <android/asset_manager_jni.h>
#include "spdlog/sinks/android_sink.h"
#include "PoseDetection.hpp"

using namespace std;

static PoseDetection *poseDetection = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_com_ylm_myapplication_VulkanPoseDetectionSurfaceView_workWithFloatBuffer(
        JNIEnv* env,
        jobject /* this */,
        jobject input) {
    jfloat *buffer =(jfloat *) env->GetDirectBufferAddress(input);
    for(int k = 0; k < 3; ++k)
    {
        for(int j = 0; j < 255; ++j)
        {
            for(int i = 0; i < 255; ++i)
            {
                int dst_index = i + 255*j + 255*255*k;
                int src_index = k + 3*i + 3*255*j;
                buffer[dst_index] = float(poseDetection->pixels[src_index])/255.f;
            }
        }
    }
}

/**
 * @brief get predictions from score maps
 *
 * @param batch_heatmaps : vector([batch_size, num_joints, height, width])
 */
std::array<std::pair<std::pair<uint32_t, uint32_t>, float>, 17>
GetMaxPreds(const std::vector<float> &heatmap)
{
    uint32_t joints = 17;
    uint32_t height = 64;
    uint32_t weight = 64;

    std::array<std::pair<std::pair<uint32_t, uint32_t>, float>, 17> points;

    for (uint32_t i = 0; i < joints; ++i)
    {
        float maxValue = 0.f;
        for (uint32_t j = 0; j < height; ++j)
        {
            for (uint32_t k = 0; k < weight; ++k)
            {
                float value = heatmap[i * weight * height + j * weight + k];
                if (value > maxValue)
                {
                    points[i] = {std::pair<uint32_t, uint32_t>(j, k), maxValue};
                    maxValue = value;
                }
            }
        }
    }
    return points;
}

extern "C" JNIEXPORT void JNICALL
Java_com_ylm_myapplication_VulkanPoseDetectionSurfaceView_heatmapJNI(
        JNIEnv* env,
        jobject /* this */,
        jfloatArray scores
        ) {
    jfloat * scoresPointer = (*env).GetFloatArrayElements(scores, nullptr);
    jsize len = (*env).GetArrayLength(scores);
    std::vector<float> heatmap(len);
    for(size_t i = 0; i < heatmap.size(); ++i)
    {
        heatmap[i] = *(scoresPointer + i);
    }
    auto predsVector = GetMaxPreds(heatmap);
    poseDetection->setMarkerPos(predsVector);

    env->ReleaseFloatArrayElements(scores, scoresPointer, 0);
}

extern "C" JNIEXPORT void JNICALL
Java_com_ylm_myapplication_VulkanPoseDetectionSurfaceView_nativeCreate(JNIEnv *env,
                                                                       jobject vulkanPDSV,
                                                                       jobject surface,
                                                                       jobject pAssetManager)
{
    std::string tag = "pd";
    auto android_logger = spdlog::android_logger_mt("android", tag);
    spdlog::set_default_logger(android_logger);
    // Forward cout/cerr to logcat
    cout.rdbuf(new PirateNdkEngine::AndroidBuffer(ANDROID_LOG_INFO));
    clog.rdbuf(new PirateNdkEngine::AndroidBuffer(ANDROID_LOG_INFO));
    cerr.rdbuf(new PirateNdkEngine::AndroidBuffer(ANDROID_LOG_ERROR));

    if (poseDetection)
    {
        delete poseDetection;
        poseDetection = nullptr;
    }

    auto assetManager = AAssetManager_fromJava(env, pAssetManager);
    Crane::mgr = assetManager;

    auto window = ANativeWindow_fromSurface(env, surface);
    poseDetection = new PoseDetection(window);
    poseDetection->init();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ylm_myapplication_VulkanPoseDetectionSurfaceView_nativeDestroy(JNIEnv *env, jobject vulkanPDSV)
{
    __android_log_print(ANDROID_LOG_DEBUG, "mc-native-VulkanAppBridge", "destroy");
    if (poseDetection)
    {
        delete poseDetection;
        poseDetection = nullptr;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ylm_myapplication_VulkanPoseDetectionSurfaceView_nativeResize(JNIEnv *env, jobject vulkanAppBridge,
                                                        jint width, jint height)
{
    __android_log_print(ANDROID_LOG_DEBUG, "mc-native-VulkanAppBridge", "resize: %dx%d", width,
                        height);
    if (poseDetection)
    {
        //poseDetection->setSize(width, height);
        //poseDetection->isResizeNeeded = true;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ylm_myapplication_VulkanPoseDetectionSurfaceView_nativeDraw(JNIEnv *env, jobject vulkanAppBridge)
{
    __android_log_print(ANDROID_LOG_DEBUG, "mc-native-VulkanAppBridge", "draw");
    if (poseDetection)
    {
        poseDetection->update();
    }
}