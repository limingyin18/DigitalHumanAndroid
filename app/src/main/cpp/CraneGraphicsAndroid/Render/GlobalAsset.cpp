#include "Render.hpp"

using namespace std;
using namespace Eigen;
using namespace Crane;

void Render::buildPipelineBuilder()
{
	pipelineBuilder.device = device.get();

	pipelineBuilder.viewport = vk::Viewport{
		.y = (float)height,
		.width = (float)width,
		.height = -(float)height,
		.minDepth = 0.f,
		.maxDepth = 1.f };
	pipelineBuilder.scissor = vk::Rect2D{
		.extent = {.width = width, .height = height} };
	pipelineBuilder.vp = vk::PipelineViewportStateCreateInfo{
		.viewportCount = 1,
		.pViewports = &pipelineBuilder.viewport,
		.scissorCount = 1,
		.pScissors = &pipelineBuilder.scissor };

	pipelineBuilder.pipelineCache = pipelineCache.get();
}

void Render::createAsset()
{
	createCameraPushConstant();
	createSceneParametersUniformBuffer();

	LOGI("����sampler")
		vk::SamplerCreateInfo samplerInfo{
		.magFilter = vk::Filter::eNearest,
		.minFilter = vk::Filter::eNearest,
		.mipmapMode = vk::SamplerMipmapMode::eNearest,
		.addressModeU = vk::SamplerAddressMode::eRepeat,
		.addressModeV = vk::SamplerAddressMode::eRepeat,
		.addressModeW = vk::SamplerAddressMode::eRepeat,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = physicalDevice.getProperties().limits.maxSamplerAnisotropy,
		.compareEnable = VK_FALSE,
		.compareOp = vk::CompareOp::eAlways,
		.borderColor = vk::BorderColor::eIntOpaqueBlack,
		.unnormalizedCoordinates = VK_FALSE };
	textureSampler = device->createSamplerUnique(samplerInfo);

	LOGI("������ɫ")
		vector<uint8_t> blankPiexls{ 255, 255, 255, 255 };
	vector<uint8_t> lilacPiexls{ 179, 153, 255, 255 };
	std::tie(imageBlank, imageViewBlank) = createTextureImage(1, 1, 4, blankPiexls.data());
	std::tie(imageLilac, imageViewLilac) = createTextureImage(1, 1, 4, lilacPiexls.data());

	descriptorImageInfoBlank.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	descriptorImageInfoBlank.imageView = imageViewBlank.get();
	descriptorImageInfoBlank.sampler = textureSampler.get();

	descriptorImageInfoLilac.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	descriptorImageInfoLilac.imageView = imageViewLilac.get();
	descriptorImageInfoLilac.sampler = textureSampler.get();

	createAssetApp();

	if (!renderables.empty()) buildRenderable();
}

void Render::createCameraPushConstant()
{
	LOGI("������� push constant");

	cameraPushConstants.resize(swapchainImages.size());
	for (auto& c : cameraPushConstants)
	{
		Eigen::Vector4f position;
		position.head(3) = camera.getCameraPos();
		position[3] = 1.0f;
		c.position = position;
		c.projView = camera.projection * camera.view;
	}
}

void Render::createSceneParametersUniformBuffer()
{
	LOGI("����������������");

	sceneParameters.ambientColor = { 0.1f, 0.1f, 0.1f, 1.0f };
	sceneParameters.fogColor = { 0.2f, 0.2f, 0.2f, 1.0f };
	sceneParameters.fogDistances = { 0.2f, 0.2f, 0.2f, 1.0f };
	sceneParameters.sunlightColor = { 0.9f, 0.9f, 0.9f, 1.0f };
	Eigen::Vector3f sunDirection = {0.f, -1.f, -0.5f };
	sunDirection.normalize();
	sceneParameters.sunlightDirection = { sunDirection.x(), sunDirection.y(), sunDirection.z(), 1.0f };

	vector<SceneParameters> sceneParametersData(swapchainImages.size(), sceneParameters);
	sceneParametersUniformOffset = padUniformBufferSize(sizeof(SceneParameters), physicalDevice.getProperties());

	const size_t sceneParameterBufferSize = swapchainImages.size() * sceneParametersUniformOffset;
	sceneParameterBuffer.create(*vmaAllocator, sceneParameterBufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	sceneParameterBuffer.update(sceneParametersData.data());

	sceneParameterBufferDescriptorInfo.buffer = sceneParameterBuffer.buffer;
	sceneParameterBufferDescriptorInfo.offset = 0;
	sceneParameterBufferDescriptorInfo.range = sizeof(SceneParameters);
}

void Render::buildRenderable()
{
	LOGI("��������Ⱦ����");

	compactDraws();

	modelMatrixOffset = padUniformBufferSize(sizeof(Eigen::Matrix4f), physicalDevice.getProperties());
	const size_t modelMatrixBufferSize = renderables.size() * modelMatrixOffset;
	modelMatrix.resize(modelMatrixBufferSize);
	auto modelMatrixPtr = modelMatrix.data();

	uint32_t offset = 0;
	for (auto& d : draws)
	{
		auto& v = renderables[d.first];
		for (size_t i = 0; i < v.mesh->data.size(); i++)
			vertices.push_back(v.mesh->data[i]);
		for (size_t i = 0; i < v.mesh->indices.size(); i++)
			indices.push_back(offset + v.mesh->indices[i]);
		offset += v.mesh->data.size();

		for (uint32_t i = 0; i < d.count; ++i)
		{
			*(reinterpret_cast<Eigen::Matrix4f*>(modelMatrixPtr)) = *renderables[d.first + i].transformMatrix;
			modelMatrixPtr = modelMatrixPtr + modelMatrixOffset;
		}
	}

	modelMatrixBuffer.create(*vmaAllocator, modelMatrixBufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	modelMatrixBuffer.update(modelMatrix.data());

	modelMatrixBufferDescriptorInfo.buffer = modelMatrixBuffer.buffer;
	modelMatrixBufferDescriptorInfo.offset = 0;
	modelMatrixBufferDescriptorInfo.range = modelMatrixBufferSize;


	for_each(renderables.begin(), renderables.end(), [](const auto&v) {
		v.material->update();
		});

	LOGI("�������㻺��");
	{
		vk::CommandBuffer cmdBuffVert = beginSingleTimeCommands();
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		Buffer stagingBuffer = Buffer::createStagingBuffer(vertices.data(), bufferSize,
			*vmaAllocator);

		vertBuff.create(*vmaAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		VkBufferCopy copyRegion{};
		copyRegion.size = bufferSize;
		vkCmdCopyBuffer(VkCommandBuffer(cmdBuffVert), VkBuffer(stagingBuffer.buffer), VkBuffer(vertBuff.buffer), 1, &copyRegion);
		endSingleTimeCommands(cmdBuffVert);
	}

	LOGI("������������");
	{
		vk::CommandBuffer cmdBuffIndex = beginSingleTimeCommands();
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
		Buffer stagingBuffer = Buffer::createStagingBuffer(indices.data(), bufferSize,
			*vmaAllocator);

		indexBuffer.create(*vmaAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		VkBufferCopy copyRegion{};
		copyRegion.size = bufferSize;
		vkCmdCopyBuffer(VkCommandBuffer(cmdBuffIndex), VkBuffer(stagingBuffer.buffer), VkBuffer(indexBuffer.buffer), 1, &copyRegion);
		endSingleTimeCommands(cmdBuffIndex);
	}

	LOGI("������ӻ��ƻ���");
	{
		bufferIndirect.create(*vmaAllocator, draws.size() * sizeof(vk::DrawIndexedIndirectCommand),
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		VmaAllocationInfo allocInfo;
		vmaGetAllocationInfo(*vmaAllocator, bufferIndirect.bufferMemory, &allocInfo);
		auto bufferIndirectP = static_cast<vk::DrawIndexedIndirectCommand*>(allocInfo.pMappedData);
		for (uint32_t i = 0, firstIndex = 0; i < draws.size(); ++i)
		{
			bufferIndirectP[i].firstIndex = firstIndex;
			bufferIndirectP[i].firstInstance = draws[i].first;
			bufferIndirectP[i].indexCount = renderables[draws[i].first].mesh->indices.size();
			bufferIndirectP[i].instanceCount = 0;
			bufferIndirectP[i].vertexOffset = 0;

			firstIndex += bufferIndirectP[i].indexCount;
		}
		descriptorBufferInfoIndirect.buffer = bufferIndirect.buffer;
		descriptorBufferInfoIndirect.range = bufferIndirect.size;
	}

	device->updateDescriptorSets(materialCull.writeDescriptorSets.size(),
		materialCull.writeDescriptorSets.data(), 0, nullptr);

	// record command
	vk::CommandBufferBeginInfo commandBufferBeginInfo{};
	commandBuffersCompute[0]->begin(commandBufferBeginInfo);

	commandBuffersCompute[0]->bindPipeline(vk::PipelineBindPoint::eCompute, materialCull.pipelinePass->pipeline.get());
	commandBuffersCompute[0]->bindDescriptorSets(vk::PipelineBindPoint::eCompute,
		materialCull.pipelinePass->pipelineLayout.get(), 0, materialCull.descriptorSets.size(), materialCull.descriptorSets.data(), 0, nullptr);

	commandBuffersCompute[0]->dispatch(renderables.size() / 1024+1, 1, 1);

	commandBuffersCompute[0]->end();
}

void Render::createDescriptorPool()
{
	LOGI("������������");
	std::vector<vk::DescriptorPoolSize> descriptorPoolSizes =
	{ {.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = 10},
	  {.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = 10},
	  {.type = vk::DescriptorType::eSampler, .descriptorCount = 10},
	  {.type = vk::DescriptorType::eUniformBufferDynamic, .descriptorCount = 10},
	  {.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 10}};

	vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{ .maxSets = 100, .poolSizeCount = (uint32_t)descriptorPoolSizes.size(), .pPoolSizes = descriptorPoolSizes.data() };

	descriptorPool = device->createDescriptorPoolUnique(descriptorPoolCreateInfo);
}