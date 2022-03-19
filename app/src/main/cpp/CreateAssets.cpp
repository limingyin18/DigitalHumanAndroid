//
// Created by ylm on 1/6/22.
//

#include "PoseApp.h"

using namespace std;
using namespace Crane;
using namespace PirateNdkEngine;

void PoseApp::ImguiInit()
{
    //1: create descriptor pool for IMGUI
    // the size of the pool is very oversize, but it's copied from imgui demo itself.
    vk::DescriptorPoolSize pool_sizes[] =
            {
                    {vk::DescriptorType::eSampler, 1000},
                    {vk::DescriptorType::eCombinedImageSampler, 1000},
                    {vk::DescriptorType::eSampledImage, 1000},
                    {vk::DescriptorType::eStorageImage, 1000},
                    {vk::DescriptorType::eUniformTexelBuffer, 1000},
                    {vk::DescriptorType::eStorageTexelBuffer, 1000},
                    {vk::DescriptorType::eUniformBuffer, 1000},
                    {vk::DescriptorType::eStorageBuffer, 1000},
                    {vk::DescriptorType::eUniformBufferDynamic, 1000},
                    {vk::DescriptorType::eStorageBufferDynamic, 1000},
                    {vk::DescriptorType::eInputAttachment, 1000}};

    vk::DescriptorPoolCreateInfo pool_info = {
            .flags =vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = 1000,
            .poolSizeCount = std::size(pool_sizes),
            .pPoolSizes = pool_sizes
    };

    imguiPool = device->createDescriptorPoolUnique(pool_info);

    // 2: initialize imgui library

    //this initializes the core structures of imgui
    imguiContext.reset(ImGui::CreateContext());
    ImGui::SetCurrentContext(imguiContext.get());
    /*
    ImGui_ImplVulkan_LoadFunctions(
        [](const char *function_name, void *)
        { return vkGetInstanceProcAddr(instance.get(), function_name); });*/
    //this initializes imgui for SDL
    //ImGui_ImplSDL2_InitForVulkan(window.get());
    ImGui_ImplAndroid_Init(window);

    //this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = (VkInstance) instance.get();
    init_info.PhysicalDevice = (VkPhysicalDevice)physicalDevice;
    init_info.Device = (VkDevice)device.get();
    init_info.Queue = (VkQueue)graphicsQueue;
    init_info.DescriptorPool = (VkDescriptorPool)imguiPool.get();
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;

    ImGui_ImplVulkan_Init(&init_info, (VkRenderPass)renderPass.get());

    //execute a gpu command to upload imgui font textures
    auto singleCmd = beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture((VkCommandBuffer)singleCmd);
    endSingleTimeCommands(singleCmd);

    //clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void PoseApp::setImgui()
{
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 0, main_viewport->WorkPos.y + 60), ImGuiCond_FirstUseEver);
    static float f = 0.0f;
    static int counter = 0;
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove
                                   | ImGuiWindowFlags_NoDecoration
                                   | ImGuiWindowFlags_AlwaysAutoResize
                                   | ImGuiWindowFlags_NoBackground;
    ImGui::Begin("Hello, world!", nullptr, windowFlags); // Create a window called "Hello, world!" and append into it.

//    if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
//        counter++;
//    ImGui::SameLine();
//    ImGui::Text("counter = %d", counter);
    ImGui::SetWindowFontScale(3.f);
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 0, main_viewport->WorkPos.y + 1600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    // Create a window called "Control Walk!"
    ImGui::Begin("Control Walk", nullptr, windowFlags);
    ImGui::PushButtonRepeat(true);
    if (ImGui::Button("left")) // Buttons return true when clicked (most widgets return true when edited/activated)
    {
        input.buttonDown("a");
    }
    else
    {
        input.buttonUp("a");
    }
    if (ImGui::Button("right")) // Buttons return true when clicked (most widgets return true when edited/activated)
    {
        input.buttonDown("d");
    }
    else
    {
        input.buttonUp("d");
    }
    if (ImGui::Button("forward")) // Buttons return true when clicked (most widgets return true when edited/activated)
    {
        input.buttonDown("w");
    }
    else
    {
        input.buttonUp("w");
    }
    if (ImGui::Button("backward")) // Buttons return true when clicked (most widgets return true when edited/activated)
    {
        input.buttonDown("s");
    }
    else
    {
        input.buttonUp("s");
    }
    if (ImGui::Button("up")) // Buttons return true when clicked (most widgets return true when edited/activated)
    {
        input.buttonDown("q");
    }
    else
    {
        input.buttonUp("q");
    }
    if (ImGui::Button("down")) // Buttons return true when clicked (most widgets return true when edited/activated)
    {
        input.buttonDown("e");
    }
    else
    {
        input.buttonUp("e");
    }
    ImGui::PopButtonRepeat();
    ImGui::End();
}