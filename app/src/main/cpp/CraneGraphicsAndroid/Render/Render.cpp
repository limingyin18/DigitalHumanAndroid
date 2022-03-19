// CraneEngine.cpp : Defines the entry point for the application.
//
#define TRACY_ENABLE
#define VMA_IMPLEMENTATION
#include "Render.hpp"
using namespace std;
using namespace Eigen;
using namespace Crane;


VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		LOGE("{}", pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		LOGW("{}", pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		LOGI("{}", pCallbackData->pMessage);
		break;
	default:
		LOGD("{}", pCallbackData->pMessage);
		break;
	}

	return VK_FALSE;
}

Render::Render() : vmaAllocator{ nullptr,[](VmaAllocator* vma) {vmaDestroyAllocator(*vma); } }
				   //tracyVkCtxPtr{nullptr, [](TracyVkCtx* ctx) {TracyVkDestroy(*ctx); }}
{
	appName = "Test";
	engineName = "Crane Vision";
	apiVersion = VK_API_VERSION_1_1;
	uint32_t appVersion = VK_MAKE_VERSION(1, 0, 0);
	uint32_t engineVersion = VK_MAKE_VERSION(1, 0, 0);

	instanceExtensions = {
	VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
	VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
	VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#ifndef NDEBUG
#ifndef ANDROID
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
#endif

	};

	layers = {
	#ifndef NDEBUG
    #ifndef ANDROID
		"VK_LAYER_KHRONOS_validation",
    #endif
	#endif
		//"VK_LAYER_KHRONOS_synchronization2"
	};

	deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
		VK_KHR_MAINTENANCE1_EXTENSION_NAME,
		//VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
		//VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
		VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
		//VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME,
		//VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
	};

	// set default clear values
	clearValues[0].color.float32.operator[](0) = 0.f;
	clearValues[0].color.float32.operator[](1) = 0.f;
	clearValues[0].color.float32.operator[](2) = 0.f;
	clearValues[0].color.float32.operator[](3) = 0.f;
	//clearValues[0].color.float32[0] = 0.0f;
	//clearValues[0].color.float32[1] = 0.0f;
	//clearValues[0].color.float32[2] = 0.0f;
	//clearValues[0].color.float32[3] = 0.0f;
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;
}

void Render::init()
{
	loadAPI();
	createInstance();
	createSurface();
	createLogicalDevice();

	createVmaAllocator();
	createCommandPool();
	createDescriptorPool();

	createSwapchain();
	createDepthStencilImage();
	createRenderPass();
	createFrameBuffers();

	buildPipelineBuilder();

	allocateCommandBuffer();

	createSynchronization();

	initEngine();
	createAsset();

	//tracyVkCtx = TracyVkContext(physicalDevice, device.get(), graphicsQueue, commandBuffer[0].get());
	//tracyVkCtxPtr.reset(&tracyVkCtx);

	LOGI("初始化完成");
}

void Crane::Render::loadAPI()
{
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
	apiVersion = vk::enumerateInstanceVersion();
	LOGI("支持的 Vulkan 版本 {}.{}.{}", VK_VERSION_MAJOR(apiVersion), VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion));
}

void Render::createInstance()
{
	LOGI("创建 vulkan instance");

	validataInstanceExtensions();
	validataInstanceLayers();

	// initialize the vk::ApplicationInfo structure
	vk::ApplicationInfo applicationInfo{ .pApplicationName = appName.c_str(),
										.applicationVersion = appVersion,
										.pEngineName = engineName.c_str(),
										.engineVersion = engineVersion,
										.apiVersion = apiVersion };

	// initialize the vk::InstanceCreateInfo
	vk::InstanceCreateInfo instanceCreateInfo{ .pApplicationInfo = &applicationInfo,
											  .enabledLayerCount = static_cast<uint32_t>(layers.size()),
											  .ppEnabledLayerNames = layers.data(),
											  .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
											  .ppEnabledExtensionNames = instanceExtensions.data() };

	instance = vk::createInstanceUnique(instanceCreateInfo, nullptr);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance.get());

#ifndef NDEBUG
#ifndef ANDROID
	vk::DebugUtilsMessengerCreateInfoEXT createInfoDebug{ .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
																			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
																			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
													   .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
																	vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
																	vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
													   .pfnUserCallback = debugCallback };
	debugMessenger = instance->createDebugUtilsMessengerEXTUnique(createInfoDebug);
#endif
#endif
}

void Render::validataInstanceExtensions()
{
	std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();

	std::set<std::string> requiredExtensions(instanceExtensions.cbegin(), instanceExtensions.cend());
	for (const auto& extension : availableExtensions)
		requiredExtensions.erase(extension.extensionName);
	if (!requiredExtensions.empty())
	{
		LOGE("extension not availabe:");
		for (const auto& extension : requiredExtensions)
			LOGE("\t{}", extension);
		throw runtime_error("instance extension required not availiable");
	}
	else
	{
		LOGI("enable intance extension:");
		for (const auto& extension : instanceExtensions)
			LOGI("\t{}", extension);
	}
}

void Render::validataInstanceLayers()
{
	std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

	std::set<std::string> requiredLayers(layers.cbegin(), layers.cend());
	for (const auto& layer : availableLayers)
		requiredLayers.erase(layer.layerName);
	if (!requiredLayers.empty())
	{
		LOGE("layers not availabe:");
		for (const auto& layer : requiredLayers)
			LOGE("\t{}", layer);
		throw runtime_error("insance layer required not availiable");
	}
	else
	{
		LOGI("enable layer:");
		for (const auto& layer : layers)
			LOGI("\t{}", layer);
	}
}

void Render::createLogicalDevice()
{
	LOGI("创建逻辑设备");

	getPhysicalDevice();

	getQueueFamilyIndex();

	validataDeviceExtensions();

	auto supportedFeatures = physicalDevice.getFeatures();
	vk::PhysicalDeviceFeatures features{};
	vk::PhysicalDeviceFeatures2 features2{};
	features2.features = features;
#ifndef ANDROID
	vk::PhysicalDeviceFeatures features{
		.multiDrawIndirect = true,
		.fillModeNonSolid = true,
		.samplerAnisotropy = true,
		};
	features2.features = features;

	//vk::PhysicalDeviceSynchronization2FeaturesKHR synchronization2Features{.synchronization2=true };
	//features2.pNext = &synchronization2Features;
	
	auto supportedFeatures = physicalDevice.getFeatures();
	if (!supportedFeatures.multiDrawIndirect)
	{
		LOGE("feature not availabe:");
		LOGE("\tmultuDrawIndirect");
		throw runtime_error("feature required not availiable");
	}
	if (!supportedFeatures.samplerAnisotropy)
	{
		LOGE("feature not availabe:");
		LOGE("\tsamplerAnisotropy");
		throw runtime_error("feature required not availiable");
	}
	if (!supportedFeatures.fillModeNonSolid)
	{
		LOGE("feature not availabe:");
		LOGE("\tfillModeNonSolid");
		throw runtime_error("feature required not availiable");
	}

	LOGI("启用特性:");
	LOGI("\tmultiDrawIndirect");
	LOGI("\tsamplerAnisotropy");
	LOGI("\tfillModeNonSolid");
#endif
	// 指定队列信息

	float queuePriorities[1] = { 0.f };
	vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	queueCreateInfos.push_back({ .queueFamilyIndex = graphicsQueueFamilyIndex,
													.queueCount = 1,
													.pQueuePriorities = queuePriorities });

	if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
	{
		queueCreateInfos.push_back({ .queueFamilyIndex = presentQueueFamilyIndex,
														.queueCount = 1,
														.pQueuePriorities = queuePriorities });
	}

	if (computeQueueFamilyIndex != graphicsQueueFamilyIndex)
	{
		queueCreateInfos.push_back({ .queueFamilyIndex = computeQueueFamilyIndex,
														.queueCount = 1,
														.pQueuePriorities = queuePriorities });
	}

	// 指定设备创建信息

	vk::DeviceCreateInfo ci{
		.pNext = &features2,
		.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledLayerCount = static_cast<uint32_t>(layers.size()),
		.ppEnabledLayerNames = layers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures = nullptr,
	};


	device = physicalDevice.createDeviceUnique(ci);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(device.get());
	//vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface.get());
	graphicsQueue = device->getQueue(graphicsQueueFamilyIndex, 0);
	presentQueue = device->getQueue(presentQueueFamilyIndex, 0);
	computeQueue = device->getQueue(computeQueueFamilyIndex, 0);
}

void Render::validataDeviceExtensions()
{
	std::vector<vk::ExtensionProperties> availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions)
		requiredExtensions.erase(extension.extensionName);

	if (!requiredExtensions.empty())
	{
		LOGE("device extension not availabe:");
		for (const auto& extension : requiredExtensions)
			LOGE("\t{}", extension);
		throw runtime_error("device extension required not availiable");
	}
	else
	{
		LOGI("enable device extension:");
		for (const auto& ext : deviceExtensions)
			LOGI("\t{}", ext);
	}
}

void Render::getPhysicalDevice()
{
	vector<vk::PhysicalDevice> gpus = instance->enumeratePhysicalDevices();
	if (gpus.empty()) throw runtime_error("can't find GPU");

	for (uint32_t i = 0; i < gpus.size(); ++i)
	{
		if (gpus[i].getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			physicalDevice = gpus[i];
	}
	if (physicalDevice.operator VkPhysicalDevice() == nullptr)
	    physicalDevice = gpus[0];

	physicalDeviceProperties = physicalDevice.getProperties();
}

void Render::getQueueFamilyIndex()
{
	vector<vk::QueueFamilyProperties> queueFamilyProps = physicalDevice.getQueueFamilyProperties();

	// find graphics and present queue family index
	graphicsQueueFamilyIndex = UINT32_MAX;
	presentQueueFamilyIndex = UINT32_MAX;
	for (unsigned i = 0; i < queueFamilyProps.size(); ++i)
	{
		if (queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eGraphics)
			graphicsQueueFamilyIndex = i;

		VkBool32 supportPresent;
#ifndef ANDROID
		auto res = physicalDevice.getSurfaceSupportKHR(i, surface.get(), &supportPresent);
#else
		if (queueFamilyProps.size() == 1)
			supportPresent = true;
#endif
		if (supportPresent)
			presentQueueFamilyIndex = i;
	}

	if(queueFamilyProps.size() == 1)
	{
		graphicsQueueFamilyIndex = 0;
		presentQueueFamilyIndex = 0;
	}
	else
	{
		// find queue family support graphics and present both
		for (unsigned i = 0; i < queueFamilyProps.size(); ++i)
		{
			VkBool32 supportPresent;
			vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice(physicalDevice), i, VkSurfaceKHR(surface.get()), &supportPresent);
			if (supportPresent)
			{
				if (queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eGraphics)
				{
					graphicsQueueFamilyIndex = i;
					presentQueueFamilyIndex = i;
				}
			}
		}
	}

	if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX)
		throw runtime_error("failed to find graphics queue family");

	// find compute queue family
	computeQueueFamilyIndex = UINT32_MAX;
	for (unsigned i = 0; i < queueFamilyProps.size(); ++i)
	{
		if (queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eCompute)
			computeQueueFamilyIndex = i;
	}

	// find compute queue family but different with graphics
	for (unsigned i = 0; i < queueFamilyProps.size(); ++i)
	{
		if ((queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eCompute) && i != graphicsQueueFamilyIndex)
			computeQueueFamilyIndex = i;
	}

	if (computeQueueFamilyIndex == UINT32_MAX)
		throw runtime_error("failed to find compute queue family");
}

void Render::createVmaAllocator()
{
	LOGI("创建缓冲分配器 vmaAllocator");

	auto vkGetImageMemoryRequirements2KHR =
		PFN_vkGetImageMemoryRequirements2KHR(vkGetDeviceProcAddr(VkDevice(device.get()), "vkGetImageMemoryRequirements2KHR"));

	auto vkGetBufferMemoryRequirements2KHR =
		PFN_vkGetBufferMemoryRequirements2KHR(vkGetDeviceProcAddr(VkDevice(device.get()), "vkGetBufferMemoryRequirements2KHR"));

	VmaVulkanFunctions vmaVulkanFunc{};
	vmaVulkanFunc.vkAllocateMemory = vkAllocateMemory;
	vmaVulkanFunc.vkBindBufferMemory = vkBindBufferMemory;
	vmaVulkanFunc.vkBindImageMemory = vkBindImageMemory;
	vmaVulkanFunc.vkCreateBuffer = vkCreateBuffer;
	vmaVulkanFunc.vkCreateImage = vkCreateImage;
	vmaVulkanFunc.vkDestroyBuffer = vkDestroyBuffer;
	vmaVulkanFunc.vkDestroyImage = vkDestroyImage;
	vmaVulkanFunc.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
	vmaVulkanFunc.vkFreeMemory = vkFreeMemory;
	vmaVulkanFunc.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
	vmaVulkanFunc.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
	vmaVulkanFunc.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
	vmaVulkanFunc.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
	vmaVulkanFunc.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
	vmaVulkanFunc.vkMapMemory = vkMapMemory;
	vmaVulkanFunc.vkUnmapMemory = vkUnmapMemory;
	vmaVulkanFunc.vkCmdCopyBuffer = vkCmdCopyBuffer;
	vmaVulkanFunc.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
	vmaVulkanFunc.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = VkPhysicalDevice(physicalDevice);
	allocatorInfo.device = VkDevice(device.get());
	allocatorInfo.instance = VkInstance(instance.get());
	allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
	allocatorInfo.pVulkanFunctions = &vmaVulkanFunc;

	VmaAllocator* vma = new VmaAllocator;
	if (vmaCreateAllocator(&allocatorInfo, vma) != VK_SUCCESS)
		throw runtime_error("failed to create vma vmaAllocator");
	vmaAllocator.reset(vma);
}
