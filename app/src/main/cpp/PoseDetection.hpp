#pragma once

#include <unistd.h>
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <android/native_window.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>
#include "CraneGraphicsAndroid/Engine.hpp"
#include "PirateNdkEngine/CameraNDK.h"
#include "PirateNdkEngine/ImageReader.h"
#include "PirateNdkEngine/NdkUtils.hpp"

#include "imgui.h"
#include "backends/imgui_impl_android.h"
#include "backends/imgui_impl_vulkan.h"

class PoseDetection final : public Crane::Engine
{
public:
    explicit PoseDetection(ANativeWindow *window);

    ~PoseDetection();

    PoseDetection(const PoseDetection &rhs) = delete;

    PoseDetection(PoseDetection
                  &&rhs) = delete;

    PoseDetection &operator=(const PoseDetection &rhs) = delete;

    PoseDetection &operator=(PoseDetection &&rhs) = delete;

    unsigned char *pixels = nullptr;
    void setMarkerPos(std::array<std::pair<std::pair<uint32_t, uint32_t>, float>, 17> &preds);

private:
    void createSurface() override;

    void createAssetApp() override;

    void updateApp() override;

    void initCamImageVulkan();

private:
    ANativeWindow *window;
    Crane::Actor plane;
    Crane::Actor marker;
    std::array<Crane::Actor, 17> markers;

    Crane::PipelinePassGraphics pipelineBackground;
    Crane::MaterialBuilder materialBuilderBackground;

    Crane::Image imageBackground;
    vk::UniqueImageView imageViewBackground;
    vk::DescriptorImageInfo descriptorImageInfoBackground;

    CameraNDK cameraNDK;
    ImageReader imageReader;
    VkSamplerYcbcrConversion conversion;
    VkSampler samplerYcbcr;
    VkImage texImage;
    VkImageView texImageView;
    VkDeviceMemory texImageMemory;
    VkDescriptorImageInfo texImageDesc;

    uint32_t *out;
    unsigned char *pixelsTexture;
};