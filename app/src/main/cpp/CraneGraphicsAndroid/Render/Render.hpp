// CraneVision.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vector>
#include <string>
#include <stack>
#include <set>
#include <tuple>
#include <algorithm>
#include <execution>

#include "Eigen/Eigen"
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
//#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.hpp>
//#define _VMA_JSON_WRITER_FUNCTIONS
#include <vk_mem_alloc.h>


//#include <Tracy.hpp>
//#include <TracyVulkan.hpp>

#include "Logging.hpp"
#include "Buffer.hpp"
#include "Image.hpp"
#include "Camera.hpp"
#include "RenderableBase.hpp"
#include "MeshBase.hpp"
#include "MaterialSystem.hpp"

#include "DataStruct.hpp"


namespace Crane
{
	class Render
	{
	public:
		Render();

		void init();

		void update();
		virtual void updateApp() = 0;

	protected:
		void draw();
		virtual void drawGUI() {};

		void loadAPI();
		void createInstance();
		/**
		 * @brief create surface with WSI system implmented in child class
		 */
		virtual void createSurface() = 0;

		virtual void createAssetApp() = 0;

		virtual void initEngine() = 0;

		void validataInstanceExtensions();
		void validataInstanceLayers();

		void createLogicalDevice();
		void validataDeviceExtensions();
		void getPhysicalDevice();
		void getQueueFamilyIndex();

		void createSwapchain();

		void createVmaAllocator();

		// render pass
		void createDepthStencilImage();
		void createRenderPass();
		void createFrameBuffers();

		void buildPipelineBuilder();

		void createAsset();
		void createCameraPushConstant();
		void createSceneParametersUniformBuffer();

		// build mesh
		void buildRenderable();

		void createDescriptorPool();

		void createCommandPool();
		void allocateCommandBuffer();

		// synchronization
		void createSynchronization();

		void updateCameraBuffer();
		void updateSceneParameters();

		void updateCullData();

		void compactDraws();

	public:
		vk::CommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(vk::CommandBuffer cmdBuffer);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

		size_t padUniformBufferSize(size_t originalSize, VkPhysicalDeviceProperties gpuProperties);

		std::tuple<Image, vk::UniqueImageView> createTextureImage(uint32_t texWidth, uint32_t texHeight,
			uint32_t texChannels, void* pixels);
		
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	public:
		std::string appName;
		std::string engineName;
		uint32_t apiVersion;
		uint32_t appVersion;
		uint32_t engineVersion;

		std::vector<const char*> instanceExtensions;
		std::vector<const char*> layers;
		std::vector<const char*> deviceExtensions;

		vk::DynamicLoader dl;
		vk::UniqueInstance instance;
		vk::UniqueDebugUtilsMessengerEXT debugMessenger;
		vk::UniqueSurfaceKHR surface;
		vk::PhysicalDevice physicalDevice;
		vk::PhysicalDeviceProperties physicalDeviceProperties;
		vk::UniqueDevice device;
		uint32_t graphicsQueueFamilyIndex, presentQueueFamilyIndex, computeQueueFamilyIndex;
		vk::Queue graphicsQueue, presentQueue, computeQueue;

		vk::UniqueCommandPool commandPool, commandPoolCompute;
		std::vector<vk::UniqueCommandBuffer> commandBuffer, commandBuffersCompute;

		std::unique_ptr<VmaAllocator, void(*)(VmaAllocator*)> vmaAllocator;

		uint32_t width, height;
		vk::Format swapchainImageFormat;
		vk::PresentModeKHR preferPresentMode;
		vk::UniqueSwapchainKHR swapchain;
		std::vector<vk::Image> swapchainImages;
		std::vector<vk::UniqueImageView> swapchainImageViews;

		Image depthStencilImage;
		vk::UniqueImageView depthStencilImageView;
		vk::UniqueRenderPass renderPass, guiRenderPass;
		vk::ClearValue clearValues[2];
		std::vector<vk::UniqueFramebuffer> framebuffers;

		// global asset
		Camera camera;
		std::vector<CameraPushConstant> cameraPushConstants;

		SceneParameters sceneParameters;
		Buffer sceneParameterBuffer;
		vk::DescriptorBufferInfo sceneParameterBufferDescriptorInfo;
		uint32_t sceneParametersUniformOffset;

		vk::UniqueSampler textureSampler;
		Image imageBlank, imageLilac;
		vk::UniqueImageView imageViewBlank, imageViewLilac;
		vk::DescriptorImageInfo descriptorImageInfoBlank, descriptorImageInfoLilac;

		// renderable
		std::vector<RenderableBase> renderables;
		std::unordered_map<std::string, std::shared_ptr<MeshBase>> loadMeshs;
		std::unordered_map<std::string, Image> loadImages;
		std::unordered_map<std::string, vk::UniqueImageView> loadImageViews;
		std::unordered_map<std::string, vk::DescriptorImageInfo> descriptorImageInfos;
		std::unordered_map<std::string, Image> normalImages;
		std::unordered_map<std::string, vk::UniqueImageView> normalImageViews;
		std::vector<vk::DescriptorImageInfo> descriptorImageInfosNormal;
		std::unordered_map<std::string, Image> metallicImages;
		std::unordered_map<std::string, vk::UniqueImageView> metallicImageViews;
		std::vector<vk::DescriptorImageInfo> descriptorImageInfosMetallic;
		std::unordered_map<std::string, Material> materials;

		uint32_t modelMatrixOffset;
		std::vector<uint8_t> modelMatrix;
		Buffer modelMatrixBuffer;
		vk::DescriptorBufferInfo modelMatrixBufferDescriptorInfo;


		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		const vk::DeviceSize vertOffsets[1] = { 0 };
		Buffer vertBuff, indexBuffer;

		vk::UniqueDescriptorPool descriptorPool;

		vk::UniquePipelineCache pipelineCache;

		// synchronization
		std::vector<vk::UniqueSemaphore> imageAcquiredSemaphores;
		std::vector<vk::UniqueSemaphore> renderFinishedSemaphores;
		std::vector<vk::UniqueSemaphore> guiRenderFinishedSemaphores;
		std::vector<vk::UniqueFence> inFlightFences;
		std::vector<vk::Fence> imagesInFlightFences;
		uint32_t currentFrame = 0;
		uint32_t currBuffIndex = 0;

		std::array<vk::PipelineStageFlags, 1> waitPipelineStageFlags;
		vk::SubmitInfo submitInfo, submitInfoCompute;
		vk::PresentInfoKHR presentInfo;
		bool drawGUIFlag = false;

		PipelineBuilder pipelineBuilder;
		MaterialBuilder materialBuilder;


		/*cull*/

		PipelinePassCompute pipelinePassCull;
		MaterialBuilder materialBuilderCull;
		Material materialCull;

		std::vector<IndirectBatch> draws;
		std::vector<FlatBatch> drawsFlat;
		Buffer bufferDrawsFlat;
		vk::DescriptorBufferInfo descriptorBufferDrawsFlat;

		std::vector<ObjectData> cullObjCandidates;
		Buffer bufferCullObjCandidate;
		vk::DescriptorBufferInfo descriptorBufferInfoCullObjCandidate;

		DrawCullData drawCullData;
		Buffer bufferDrawCullData;
		vk::DescriptorBufferInfo descriptorBufferInfoCullData;

		Buffer bufferIndirect;
		vk::DescriptorBufferInfo descriptorBufferInfoIndirect;

		Buffer bufferInstanceID;
		vk::DescriptorBufferInfo descriptorBufferInfoInstanceID;

		Crane::PipelinePassGraphics pipelinePassPhong;
		Crane::MaterialBuilder materialBuilderPhong;

		Crane::PipelinePassGraphics pipelinePassLinePhong;

		// profiler
		//TracyVkCtx tracyVkCtx;
		//std::unique_ptr<TracyVkCtx, void(*)(TracyVkCtx*)> tracyVkCtxPtr;
	};
}
