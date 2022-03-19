#include <cassert>
#include <jni.h>
#include <string>
#include <array>
#include <vector>

using namespace std;

extern "C" JNIEXPORT jstring JNICALL
Java_com_ylm_myapplication_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}



std::array<std::pair<std::pair<uint32_t, uint32_t>, float>, 17>
GetMaxPreds(const std::vector<float> &heatmap);





