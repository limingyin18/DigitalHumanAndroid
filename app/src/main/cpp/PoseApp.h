//
// Created by ylm on 1/6/22.
//

#ifndef CRANEPOSE_POSEAPP_H
#define CRANEPOSE_POSEAPP_H

#include <unistd.h>
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <android/native_window.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>
#include "CraneGraphicsAndroid/Engine.hpp"
#include "PirateNdkEngine/CameraNDK.h"
#include "PirateNdkEngine/NdkUtils.hpp"

#include "imgui.h"
#include "backends/imgui_impl_android.h"
#include "backends/imgui_impl_vulkan.h"

class PoseApp final : public Crane::Engine {
public:
    explicit PoseApp(AAssetManager* assetManager, ANativeWindow* window);
    ~PoseApp();
    PoseApp(const PoseApp &rhs) = delete;
    PoseApp(PoseApp &&rhs) = delete;
    PoseApp &operator=(const PoseApp &rhs) = delete;
    PoseApp &operator=(PoseApp &&rhs) = delete;

private:
    void createSurface() override;
    void createAssetApp() override;
    void updateApp() override;

    void ImguiInit();
    void drawGUI() override;
    virtual void setImgui();

private:
    AAssetManager* assetManager;
    ANativeWindow* window;
    Crane::Actor chessboard;
    std::unique_ptr<ImGuiContext, decltype(&ImGui::DestroyContext)> imguiContext;
    vk::UniqueDescriptorPool imguiPool;
    vk::UniqueCommandPool guiCommandPool;
    std::vector<vk::UniqueCommandBuffer> guiCmdBuffs;
};

#endif //CRANEPOSE_POSEAPP_H
