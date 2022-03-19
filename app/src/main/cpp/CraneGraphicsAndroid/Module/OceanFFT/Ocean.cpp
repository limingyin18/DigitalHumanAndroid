#include "Ocean.hpp"

using namespace std;
using namespace Eigen;
using namespace CranePhysics;
using namespace Crane;

constexpr int N = 32;
constexpr int M = 32;
constexpr float LX = 100.0f;
constexpr float LZ = 100.0f;
constexpr float AMPLITUDE = 100.f;
constexpr float WIND_SPEED = 100.0f;
constexpr complex<float> WIND_DIRECTION = { 1.f, 1.f };
constexpr float CHOPPY_FACTOR = 1.0f;


Ocean::Ocean() : oceanAmpl(N, M, LX, LZ, AMPLITUDE, WIND_SPEED, WIND_DIRECTION, CHOPPY_FACTOR)
{
}

Crane::Ocean::~Ocean()
{
	deleteVkFFT(&vkFFTAppAmpl);
	deleteVkFFT(&vkFFTAppNormalX);
	deleteVkFFT(&vkFFTAppNormalZ);
	deleteVkFFT(&vkFFTAppDx);
	deleteVkFFT(&vkFFTAppDz);
}

void Ocean::update(float dtAll)
{
	t.update(&dtAll);

	context->graphicsQueue.waitIdle();
	computeQueue.submit(1, &computeSubmitInfo, vk::Fence{});

	computeQueue.waitIdle();
}

void Ocean::init(Crane::Render* ctx)
{
	context = ctx;

	LOGI("�������������Դ")
	{
		createBufferOcean(bufferAmpl, imageAmpl, imageViewAmpl, deviceMeomoryAmpl, descriptorBufferInfoAmpl, descriptorImageInfoAmpl);
		createBufferOcean(bufferNormalX, imageNormalX, imageViewNormalX, deviceMeomoryNormalX, descriptorBufferInfoNormalX, descriptorImageInfoNormalX);
		createBufferOcean(bufferNormalZ, imageNormalZ, imageViewNormalZ, deviceMeomoryNormalZ, descriptorBufferInfoNormalZ, descriptorImageInfoNormalZ);
		createBufferOcean(bufferDx, imageDx, imageViewDx, deviceMeomoryDx, descriptorBufferInfoDx, descriptorImageInfoDx);
		createBufferOcean(bufferDz, imageDz, imageViewDz, deviceMeomoryDz, descriptorBufferInfoDz, descriptorImageInfoDz);

		h_tlide_0.create(*context->vmaAllocator, N * M * 2 * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		h_tlide_0.update(oceanAmpl.h_tlide_0.data());

		h_tlide_0_conj.create(*context->vmaAllocator, N * M * 2 * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		h_tlide_0_conj.update(oceanAmpl.h_tlide_0_conj.data());

		NB.create(*context->vmaAllocator, sizeof(N), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		NB.update(&N);

		MB.create(*context->vmaAllocator, sizeof(M), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		MB.update(&M);

		Lx.create(*context->vmaAllocator, sizeof(LX), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		Lx.update(&LX);

		Lz.create(*context->vmaAllocator, sizeof(LZ), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		Lz.update(&LZ);

		t.create(*context->vmaAllocator, 4, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		lambda.create(*context->vmaAllocator, 4, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		lambda.update(&oceanAmpl.lambda);
	}


	createAmpl();
	createIfft2();
	createSign();
	createRender();

	createCommand();
	initVkFFT();
	writeCommand();
}

void Crane::Ocean::createBufferOcean(vk::UniqueBuffer& buffer, vk::UniqueImage& image, vk::UniqueImageView& imageView, vk::UniqueDeviceMemory& deviceMemory, vk::DescriptorBufferInfo& descriptorBufferInfo, vk::DescriptorImageInfo& descriptorImageInfo)
{
	size_t size = N * M * 2 * 4;
	vk::BufferCreateInfo bufferInfo{};
	bufferInfo.size = size;
	bufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;
	buffer = context->device->createBufferUnique(bufferInfo);
	descriptorBufferInfo.buffer = buffer.get();
	descriptorBufferInfo.range = size;
	descriptorBufferInfo.offset = 0;

	vk::Extent3D extent;
	extent.width = N;
	extent.height = M;
	extent.depth = 1;

	vk::ImageCreateInfo imageInfo{};
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent = extent;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = vk::Format::eR32G32Sfloat;
	imageInfo.tiling = vk::ImageTiling::eLinear;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = vk::ImageUsageFlagBits::eSampled;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;
	image = context->device->createImageUnique(imageInfo);



	vk::MemoryRequirements memRequirementsBuffer = context->device->getBufferMemoryRequirements(buffer.get());
	vk::MemoryRequirements memRequirementsImage = context->device->getImageMemoryRequirements(image.get());
	vk::MemoryAllocateInfo allocInfo{};
	allocInfo.allocationSize = size;
	allocInfo.memoryTypeIndex = context->findMemoryType(memRequirementsBuffer.memoryTypeBits & memRequirementsImage.memoryTypeBits
		, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	deviceMemory = context->device->allocateMemoryUnique(allocInfo);
	context->device->bindBufferMemory(buffer.get(), deviceMemory.get(), 0);
	context->device->bindImageMemory(image.get(), deviceMemory.get(), 0);

	vk::CommandBuffer cmdBuffSingle = context->beginSingleTimeCommands();

	vk::ImageMemoryBarrier barrier{};
	barrier.oldLayout = vk::ImageLayout::eUndefined;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image.get();
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = vk::AccessFlagBits::eMemoryWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

	VkImageMemoryBarrier vkm = barrier;
	vkCmdPipelineBarrier(cmdBuffSingle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
		nullptr, 1, &vkm);
	context->endSingleTimeCommands(cmdBuffSingle);

	vk::ImageViewCreateInfo imageViewCreateInfo{ .image = image.get(),
										.viewType = vk::ImageViewType::e2D,
										.format = vk::Format::eR32G32Sfloat,
										.subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
															 .baseMipLevel = 0,
															 .levelCount = 1,
															 .baseArrayLayer = 0,
															 .layerCount = 1} };
	imageView = context->device->createImageViewUnique(imageViewCreateInfo);

	descriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	descriptorImageInfo.imageView = imageView.get();
	descriptorImageInfo.sampler = context->textureSampler.get();

}

void Ocean::createAmpl()
{
	LOGI("��������Ƶ�׹���")
	{
		amplPipelinePass.device = context->device.get();

		auto amplShaderCode = Loader::readFile("shaders/ampl.comp.spv");
		amplPipelinePass.addShader(amplShaderCode, vk::ShaderStageFlagBits::eCompute);

		amplPipelinePass.buildDescriptorSetLayout();

		amplPipelinePass.buildPipelineLayout();
		amplPipelinePass.buildPipeline();
	}

	LOGI("��������Ƶ�׼�����Դ���ʹ�������")
	{
		materialBuilderAmpl.descriptorPool = context->descriptorPool.get();
		materialBuilderAmpl.pipelinePass = &amplPipelinePass;

		materialBuilderAmpl.descriptorInfos[0][0].first = &descriptorBufferInfoAmpl;
		materialBuilderAmpl.descriptorInfos[0][1].first = &descriptorBufferInfoNormalX;
		materialBuilderAmpl.descriptorInfos[0][2].first = &descriptorBufferInfoNormalZ;
		materialBuilderAmpl.descriptorInfos[0][3].first = &descriptorBufferInfoDx;
		materialBuilderAmpl.descriptorInfos[0][4].first = &descriptorBufferInfoDz;
		materialBuilderAmpl.descriptorInfos[0][5].first = &h_tlide_0.descriptorBufferInfo;
		materialBuilderAmpl.descriptorInfos[0][6].first = &h_tlide_0_conj.descriptorBufferInfo;
		materialBuilderAmpl.descriptorInfos[0][7].first = &NB.descriptorBufferInfo;
		materialBuilderAmpl.descriptorInfos[0][8].first = &MB.descriptorBufferInfo;
		materialBuilderAmpl.descriptorInfos[0][9].first = &Lx.descriptorBufferInfo;
		materialBuilderAmpl.descriptorInfos[0][10].first = &Lz.descriptorBufferInfo;
		materialBuilderAmpl.descriptorInfos[0][11].first = &t.descriptorBufferInfo;
	}

	LOGI("��������Ƶ�׼�����Դ����")
	{
		materialAmpl = materialBuilderAmpl.build();
		materialAmpl.update();
	}
}

void Ocean::createIfft2()
{
	LOGI("����ifft2����")
	{
		iff2PipelinePass.device = context->device.get();

		auto ifft22ShaderCode = Loader::readFile("shaders/ifft2.comp.spv");
		iff2PipelinePass.addShader(ifft22ShaderCode, vk::ShaderStageFlagBits::eCompute);

		iff2PipelinePass.buildDescriptorSetLayout();

		iff2PipelinePass.buildPipelineLayout();
		iff2PipelinePass.buildPipeline();
	}

	LOGI("����ifft2������Դ���ʹ�������")
	{
		materialBuilderIfft2.descriptorPool = context->descriptorPool.get();
		materialBuilderIfft2.pipelinePass = &iff2PipelinePass;

		materialBuilderIfft2.descriptorInfos[0][1].first = &NB.descriptorBufferInfo;
	}

	LOGI("����ifft2������Դ����")
	{
		materialBuilderIfft2.descriptorInfos[0][0].first = &descriptorBufferInfoAmpl;
		materialIff2Ampl = materialBuilderIfft2.build();
		materialIff2Ampl.update();

		materialBuilderIfft2.descriptorInfos[0][0].first = &descriptorBufferInfoNormalX;
		materialIff2NormalX = materialBuilderIfft2.build();
		materialIff2NormalX.update();

		materialBuilderIfft2.descriptorInfos[0][0].first = &descriptorBufferInfoNormalZ;
		materialIff2NormalZ = materialBuilderIfft2.build();
		materialIff2NormalZ.update();

		materialBuilderIfft2.descriptorInfos[0][0].first = &descriptorBufferInfoDx;
		materialIff2Dx = materialBuilderIfft2.build();
		materialIff2Dx.update();

		materialBuilderIfft2.descriptorInfos[0][0].first = &descriptorBufferInfoDz;
		materialIff2Dz = materialBuilderIfft2.build();
		materialIff2Dz.update();
	}
}

void Ocean::createSign()
{
	LOGI("����sign����")
	{
		pipelinePassSign.device = context->device.get();

		auto shaderCodeSign = Loader::readFile("shaders/sign.comp.spv");
		pipelinePassSign.addShader(shaderCodeSign, vk::ShaderStageFlagBits::eCompute);

		pipelinePassSign.buildDescriptorSetLayout();

		pipelinePassSign.buildPipelineLayout();
		pipelinePassSign.buildPipeline();
	}

	LOGI("����sign������Դ���ʹ�������")
	{
		materialBuilderSign.descriptorPool = context->descriptorPool.get();
		materialBuilderSign.pipelinePass = &pipelinePassSign;

		materialBuilderSign.descriptorInfos[0][1].first = &NB.descriptorBufferInfo;
	}

	LOGI("����sign������Դ����")
	{
		materialBuilderSign.descriptorInfos[0][0].first = &descriptorBufferInfoAmpl;
		materialSignAmpl = materialBuilderSign.build();
		materialSignAmpl.update();

		materialBuilderSign.descriptorInfos[0][0].first = &descriptorBufferInfoNormalX;
		materialSignNormalX = materialBuilderSign.build();
		materialSignNormalX.update();

		materialBuilderSign.descriptorInfos[0][0].first = &descriptorBufferInfoNormalZ;
		materialSignNormalZ = materialBuilderSign.build();
		materialSignNormalZ.update();

		materialBuilderSign.descriptorInfos[0][0].first = &descriptorBufferInfoDx;
		materialSignDx = materialBuilderSign.build();
		materialSignDx.update();

		materialBuilderSign.descriptorInfos[0][0].first = &descriptorBufferInfoDz;
		materialSignDz = materialBuilderSign.build();
		materialSignDz.update();
	}
}

void Ocean::createRender()
{
	LOGI("������������")
	{
		context->loadMeshs["Ocean"] = make_shared<Plane>(N, M);
		mesh = context->loadMeshs["Ocean"];
		mesh->setVertices([](uint32_t i, Vertex& v) {v.position *= 100.f; });
	}

	LOGI("����������ɫ����")
	{
		pipelinePass.device = context->device.get();
		pipelinePass.renderPass = context->renderPass.get();

		auto shaderCodeOceanVert = Loader::readFile("shaders/ocean.vert.spv");
		pipelinePass.addShader(shaderCodeOceanVert, vk::ShaderStageFlagBits::eVertex);
		auto shaderCodeOceanFrag = Loader::readFile("shaders/ocean.frag.spv");
		pipelinePass.addShader(shaderCodeOceanFrag, vk::ShaderStageFlagBits::eFragment);
		pipelinePass.bindings[0][0].descriptorType = vk::DescriptorType::eUniformBufferDynamic;

		pipelinePass.buildDescriptorSetLayout();

		pipelinePass.buildPipelineLayout();
		//context->pipelineBuilder.rs.polygonMode = vk::PolygonMode::eLine;
		pipelinePass.buildPipeline(context->pipelineBuilder);
	}

	LOGI("����������ɫ���ʹ�������")
	{
		materialBuilder.descriptorPool = context->descriptorPool.get();
		materialBuilder.pipelinePass = &pipelinePass;

		materialBuilder.descriptorInfos[0][0].first = &context->sceneParameterBufferDescriptorInfo;
		materialBuilder.descriptorInfos[0][1].first = &context->modelMatrixBufferDescriptorInfo;
		materialBuilder.descriptorInfos[0][2].first = &context->descriptorBufferInfoInstanceID;


		materialBuilder.descriptorInfos[1][0].second = &descriptorImageInfoAmpl;
		materialBuilder.descriptorInfos[1][1].second = &descriptorImageInfoNormalX;
		materialBuilder.descriptorInfos[1][2].second = &descriptorImageInfoNormalZ;
		materialBuilder.descriptorInfos[1][3].second = &descriptorImageInfoDx;
		materialBuilder.descriptorInfos[1][4].second = &descriptorImageInfoDz;
		materialBuilder.descriptorInfos[1][5].first = &lambda.descriptorBufferInfo;
	}

	LOGI("����������ɫ����")
	{
		context->materials["Ocean"] = materialBuilder.build();
		material = &context->materials["Ocean"];
	}
}

void Ocean::createCommand()
{
	LOGI("�������󹹽�����")

		vk::CommandPoolCreateInfo commandPoolCreateInfo{ .queueFamilyIndex = context->computeQueueFamilyIndex };

	computeCommandPool = context->device->createCommandPoolUnique(commandPoolCreateInfo);

	// allocate compute command buffer
	vk::CommandBufferAllocateInfo commandBufferAllocateInfo{ .commandPool = computeCommandPool.get(),
		.level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
	computeCommandBuffers = context->device->allocateCommandBuffersUnique(commandBufferAllocateInfo);

	computeSubmitInfo.commandBufferCount = computeCommandBuffers.size();
	computeSubmitInfo.pCommandBuffers = &computeCommandBuffers[0].get();

	computeQueue = context->computeQueue;

	vk::FenceCreateInfo fenceCreateInfo{};
	fftFence = context->device->createFenceUnique(fenceCreateInfo);
}

void Ocean::initVkFFT()
{
	LOGI("��ʼ��vkFFT");

	int resGlslangInitialize = glslang_initialize_process();

	vkFFTConfiguration.FFTdim = 2;
	vkFFTConfiguration.size[0] = N;
	vkFFTConfiguration.size[1] = M;
	vkFFTConfiguration.device = reinterpret_cast<VkDevice*>(&context->device.get());
	buffersize = sizeof(float) * 2 * N * M;
	vkFFTConfiguration.queue = reinterpret_cast<VkQueue*>(&computeQueue);
	vkFFTConfiguration.fence = reinterpret_cast<VkFence*>(&fftFence);
	vkFFTConfiguration.commandPool = reinterpret_cast<VkCommandPool*>(&computeCommandPool);
	vkFFTConfiguration.physicalDevice = reinterpret_cast<VkPhysicalDevice*>(&context->physicalDevice);
	vkFFTConfiguration.bufferSize = &buffersize;
	vkFFTConfiguration.isCompilerInitialized = 1;
	vkFFTConfiguration.makeInversePlanOnly = 1;
	vkFFTConfiguration.normalize = 1;

	initializeVkFFT(&vkFFTAppAmpl, vkFFTConfiguration);
	initializeVkFFT(&vkFFTAppNormalX, vkFFTConfiguration);
	initializeVkFFT(&vkFFTAppNormalZ, vkFFTConfiguration);
	initializeVkFFT(&vkFFTAppDx, vkFFTConfiguration);
	initializeVkFFT(&vkFFTAppDz, vkFFTConfiguration);

	vkFFTLaunchParamsAmpl.buffer = reinterpret_cast<VkBuffer*>(&bufferAmpl.get());
	vkFFTLaunchParamsAmpl.commandBuffer = reinterpret_cast<VkCommandBuffer*>(&computeCommandBuffers[0].get());

	vkFFTLaunchParamsNormalX.buffer = reinterpret_cast<VkBuffer*>(&bufferNormalX.get());
	vkFFTLaunchParamsNormalX.commandBuffer = reinterpret_cast<VkCommandBuffer*>(&computeCommandBuffers[0].get());

	vkFFTLaunchParamsNormalZ.buffer = reinterpret_cast<VkBuffer*>(&bufferNormalZ.get());
	vkFFTLaunchParamsNormalZ.commandBuffer = reinterpret_cast<VkCommandBuffer*>(&computeCommandBuffers[0].get());

	vkFFTLaunchParamsDx.buffer = reinterpret_cast<VkBuffer*>(&bufferDx.get());
	vkFFTLaunchParamsDx.commandBuffer = reinterpret_cast<VkCommandBuffer*>(&computeCommandBuffers[0].get());

	vkFFTLaunchParamsDz.buffer = reinterpret_cast<VkBuffer*>(&bufferDz.get());
	vkFFTLaunchParamsDz.commandBuffer = reinterpret_cast<VkCommandBuffer*>(&computeCommandBuffers[0].get());
}

void Ocean::writeCommand()
{
	vk::CommandBufferBeginInfo commandBufferBeginInfo{};
	computeCommandBuffers[0]->begin(commandBufferBeginInfo);

	// ampl
	computeCommandBuffers[0]->bindPipeline(vk::PipelineBindPoint::eCompute, materialAmpl.pipelinePass->pipeline.get());
	computeCommandBuffers[0]->bindDescriptorSets(vk::PipelineBindPoint::eCompute, materialAmpl.pipelinePass->pipelineLayout.get(), 0, materialAmpl.descriptorSets.size(), materialAmpl.descriptorSets.data(), 0, nullptr);
	computeCommandBuffers[0]->dispatch(1, 1, 1);

	/*
	vk::MemoryBarrier2KHR memoryBarrier = {
		.srcStageMask = vk::PipelineStageFlagBits2KHR::eComputeShader,
		.srcAccessMask = vk::AccessFlagBits2KHR::eShaderWrite,
		.dstStageMask = vk::PipelineStageFlagBits2KHR::eComputeShader,
		.dstAccessMask = vk::AccessFlagBits2KHR::eShaderRead };

	vk::DependencyInfoKHR dependencyInfo = {
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &memoryBarrier
	};
	computeCommandBuffers[0]->pipelineBarrier2KHR(dependencyInfo);
	*/

	vk::MemoryBarrier memoryBarrier = {
		.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
		.dstAccessMask = vk::AccessFlagBits::eShaderRead };

	vkCmdPipelineBarrier( computeCommandBuffers[0].get(),
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // srcStageMask
    	VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // dstStageMask
		{},
    	1,                                    // memoryBarrierCount
    	(VkMemoryBarrier*)&memoryBarrier,                       // pMemoryBarriers);
		0,
		nullptr,
		0,
		nullptr);

	// ifft2
	VkFFTAppend(&vkFFTAppAmpl, 1, &vkFFTLaunchParamsAmpl);
	VkFFTAppend(&vkFFTAppNormalX, 1, &vkFFTLaunchParamsNormalX);
	VkFFTAppend(&vkFFTAppNormalZ, 1, &vkFFTLaunchParamsNormalZ);
	VkFFTAppend(&vkFFTAppDx, 1, &vkFFTLaunchParamsDx);
	VkFFTAppend(&vkFFTAppDz, 1, &vkFFTLaunchParamsDz);

	//computeCommandBuffers[0]->bindPipeline(vk::PipelineBindPoint::eCompute, materialIff2Ampl.pipelinePass->pipeline.get());
	//computeCommandBuffers[0]->bindDescriptorSets(vk::PipelineBindPoint::eCompute, materialIff2Ampl.pipelinePass->pipelineLayout.get(), 0, materialIff2Ampl.descriptorSets.size(), materialIff2Ampl.descriptorSets.data(), 0, nullptr);
	//computeCommandBuffers[0]->dispatch(1, 1, 1);

	//computeCommandBuffers[0]->bindDescriptorSets(vk::PipelineBindPoint::eCompute, materialIff2NormalX.pipelinePass->pipelineLayout.get(), 0, materialIff2NormalX.descriptorSets.size(), materialIff2NormalX.descriptorSets.data(), 0, nullptr);
	//computeCommandBuffers[0]->dispatch(1, 1, 1);

	//computeCommandBuffers[0]->bindDescriptorSets(vk::PipelineBindPoint::eCompute, materialIff2NormalZ.pipelinePass->pipelineLayout.get(), 0, materialIff2NormalZ.descriptorSets.size(), materialIff2NormalZ.descriptorSets.data(), 0, nullptr);
	//computeCommandBuffers[0]->dispatch(1, 1, 1);

	//computeCommandBuffers[0]->bindDescriptorSets(vk::PipelineBindPoint::eCompute, materialIff2Dx.pipelinePass->pipelineLayout.get(), 0, materialIff2Dx.descriptorSets.size(), materialIff2Dx.descriptorSets.data(), 0, nullptr);
	//computeCommandBuffers[0]->dispatch(1, 1, 1);

	//computeCommandBuffers[0]->bindDescriptorSets(vk::PipelineBindPoint::eCompute, materialIff2Dz.pipelinePass->pipelineLayout.get(), 0, materialIff2Dz.descriptorSets.size(), materialIff2Dz.descriptorSets.data(), 0, nullptr);
	//computeCommandBuffers[0]->dispatch(1, 1, 1);

	//computeCommandBuffers[0]->pipelineBarrier2KHR(dependencyInfo);
	vkCmdPipelineBarrier( computeCommandBuffers[0].get(),
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // srcStageMask
    	VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // dstStageMask
		{},
    	1,                                    // memoryBarrierCount
    	(VkMemoryBarrier*)&memoryBarrier,                       // pMemoryBarriers);
		0,
		nullptr,
		0,
		nullptr);
	// sign
	computeCommandBuffers[0]->bindPipeline(vk::PipelineBindPoint::eCompute, materialSignAmpl.pipelinePass->pipeline.get());
	computeCommandBuffers[0]->bindDescriptorSets(vk::PipelineBindPoint::eCompute, materialSignAmpl.pipelinePass->pipelineLayout.get(), 0, materialSignAmpl.descriptorSets.size(), materialSignAmpl.descriptorSets.data(), 0, nullptr);
	computeCommandBuffers[0]->dispatch(1, 1, 1);

	computeCommandBuffers[0]->bindDescriptorSets(vk::PipelineBindPoint::eCompute, materialSignNormalX.pipelinePass->pipelineLayout.get(), 0, materialSignNormalX.descriptorSets.size(), materialSignNormalX.descriptorSets.data(), 0, nullptr);
	computeCommandBuffers[0]->dispatch(1, 1, 1);

	computeCommandBuffers[0]->bindDescriptorSets(vk::PipelineBindPoint::eCompute, materialSignNormalZ.pipelinePass->pipelineLayout.get(), 0, materialSignNormalZ.descriptorSets.size(), materialSignNormalZ.descriptorSets.data(), 0, nullptr);
	computeCommandBuffers[0]->dispatch(1, 1, 1);

	computeCommandBuffers[0]->bindDescriptorSets(vk::PipelineBindPoint::eCompute, materialSignDx.pipelinePass->pipelineLayout.get(), 0, materialSignDx.descriptorSets.size(), materialSignDx.descriptorSets.data(), 0, nullptr);
	computeCommandBuffers[0]->dispatch(1, 1, 1);

	computeCommandBuffers[0]->bindDescriptorSets(vk::PipelineBindPoint::eCompute, materialSignDz.pipelinePass->pipelineLayout.get(), 0, materialSignDz.descriptorSets.size(), materialSignDz.descriptorSets.data(), 0, nullptr);
	computeCommandBuffers[0]->dispatch(1, 1, 1);

	computeCommandBuffers[0]->end();
}
