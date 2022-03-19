#include "PoseDetection.hpp"

using namespace std;

void PoseDetection::initCamImageVulkan()
{
    AHardwareBuffer *aHardBuff;
    while ((aHardBuff = imageReader.GetLatestBuffer()) == nullptr)
    {
        usleep(1000u);
    }

    PirateNdkEngine::YUV420PTORGBA(out, imageReader.images_[imageReader.cur_index_].get());
    auto singleCmd = beginSingleTimeCommands();
    imageBackground.update(out, singleCmd);
    endSingleTimeCommands(singleCmd);
}