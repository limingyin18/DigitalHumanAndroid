//
// Created by ylm on 1/6/22.
//

#include "PoseApp.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

using namespace std;
using namespace Crane;
using namespace PirateNdkEngine;
//static const int WIDTH = 1280;
//static const int HEIGHT = 720;

static const string APP_NAME = "Testing";
//static const uint32_t APP_VERSION = VK_MAKE_VERSION(1, 0, 0);

/**
 * MAX_BUF_COUNT:
 *   Max buffers in this ImageReader.
 */
#define MAX_IMAGE_READER_BUF_COUNT 4

PoseApp::PoseApp(AAssetManager* mgr, ANativeWindow* win) : Engine(),
    imguiContext{ nullptr, ImGui::DestroyContext },
    window{win}, assetManager{mgr}
{
    Crane::mgr = mgr;
    preferPresentMode = vk::PresentModeKHR::eFifo;
    camera.target = Vector3f{ 0.f, 1.8f, 10.f };
    camera.rotation[0] = -0.0f;
    camera.cameraMoveSpeed = 1.f;

    drawGUIFlag = true;
}

PoseApp::~PoseApp() noexcept
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplAndroid_Shutdown();
}

void PoseApp::updateApp()
{
    updateEngine();
}

void PoseApp::createAssetApp()
{
    LOGI("create chessboard");

    string name = "chessboard";
    loadMeshs[name] = make_shared<Crane::Chessboard>(11, 11);
    loadMeshs[name]->setVertices([](uint32_t, Vertex& v) {v.position *= 100; });
    loadMeshs[name]->recomputeNormals();
    chessboard.mesh = loadMeshs[name];

    materials[name] = materialBuilderPhong.build();
    chessboard.material = &materials[name];

    chessboard.setRotation(Vector3f{3.14f/2.f, 0.f, 0.f});
    renderables.emplace_back(chessboard.mesh.get(), chessboard.material, &chessboard.transform);


    vk::CommandPoolCreateInfo guiCmdPoolCreateInfo = {
            .flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = graphicsQueueFamilyIndex };
    guiCommandPool = device->createCommandPoolUnique(guiCmdPoolCreateInfo);

    vk::CommandBufferAllocateInfo cmdAllocInfo = {
            .commandPool = guiCommandPool.get(),
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = static_cast<uint32_t>(swapchainImages.size()) };
    guiCmdBuffs = device->allocateCommandBuffersUnique(cmdAllocInfo);

    ImguiInit();
}

void PoseApp::createSurface()
{
    VkAndroidSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.window = window;

    VkSurfaceKHR surface;
    if (vkCreateAndroidSurfaceKHR(VkInstance(instance.get()), &createInfo, nullptr, &surface) != VK_SUCCESS)
        throw runtime_error("failed to create surface");

    this->surface = vk::UniqueSurfaceKHR(vk::SurfaceKHR(surface), instance.get());
}

void PoseApp::drawGUI()
{
    //imgui new frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplAndroid_NewFrame();

    ImGui::NewFrame();

    setImgui();

    ImGui::Render();

    vk::CommandBufferBeginInfo commandBufferBeginInfo{};
    guiCmdBuffs[currBuffIndex]->begin(commandBufferBeginInfo);

    VkRenderPassBeginInfo rpBeginInfo{};
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.pNext = nullptr;
    rpBeginInfo.renderPass = (VkRenderPass)guiRenderPass.get();
    rpBeginInfo.framebuffer = (VkFramebuffer)framebuffers[currBuffIndex].get();
    rpBeginInfo.renderArea.offset.x = 0;
    rpBeginInfo.renderArea.offset.y = 0;
    rpBeginInfo.renderArea.extent.width = width;
    rpBeginInfo.renderArea.extent.height = height;
    rpBeginInfo.clearValueCount = 0;
    rpBeginInfo.pClearValues = nullptr;

    vkCmdBeginRenderPass((VkCommandBuffer)guiCmdBuffs[currBuffIndex].get(), &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), (VkCommandBuffer)guiCmdBuffs[currBuffIndex].get());

    vkCmdEndRenderPass((VkCommandBuffer)guiCmdBuffs[currBuffIndex].get());

    if (vkEndCommandBuffer((VkCommandBuffer)guiCmdBuffs[currBuffIndex].get()) != VK_SUCCESS)
        throw runtime_error("failed to record command buffer");

    submitInfo.pCommandBuffers = &guiCmdBuffs[currBuffIndex].get();
    submitInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame].get();
    submitInfo.pSignalSemaphores = &guiRenderFinishedSemaphores[currentFrame].get();
    graphicsQueue.submit(submitInfo, inFlightFences[currentFrame].get());
    presentInfo.pWaitSemaphores = &guiRenderFinishedSemaphores[currentFrame].get();
}

