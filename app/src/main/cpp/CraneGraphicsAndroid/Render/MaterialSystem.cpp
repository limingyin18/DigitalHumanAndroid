#include "MaterialSystem.hpp"

using namespace std;
using namespace Crane;

PipelineBuilder::PipelineBuilder()
{
	vertexInputBindingDescriptions = Vertex::GetVertexInputBindingDescription();
	vertexInputAttributeDescriptions = Vertex::GetVertexInputAttributeDescription();
	vi = vk::PipelineVertexInputStateCreateInfo{
		.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindingDescriptions.size()),
		.pVertexBindingDescriptions = vertexInputBindingDescriptions.data(),
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescriptions.size()),
		.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data() };

	ia = vk::PipelineInputAssemblyStateCreateInfo{ .topology = vk::PrimitiveTopology::eTriangleList };

	rs = vk::PipelineRasterizationStateCreateInfo{
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eNone,
		.frontFace = vk::FrontFace::eCounterClockwise,
		.lineWidth = 1.0f };

	att_state[0].colorWriteMask = vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eR |
		vk::ColorComponentFlagBits::eA;
	att_state[0].alphaBlendOp = vk::BlendOp::eAdd;
	att_state[0].colorBlendOp = vk::BlendOp::eAdd;
	cb = vk::PipelineColorBlendStateCreateInfo{
		.logicOp = vk::LogicOp::eNoOp,
		.attachmentCount = 1,
		.pAttachments = att_state,
		.blendConstants = array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f} };

	ds = vk::PipelineDepthStencilStateCreateInfo{
		.depthTestEnable = true,
		.depthWriteEnable = true,
		.depthCompareOp = vk::CompareOp::eLessOrEqual,
		.front = {.failOp = vk::StencilOp::eKeep,
				.passOp = vk::StencilOp::eKeep,
				.depthFailOp = vk::StencilOp::eKeep,
				.compareOp = vk::CompareOp::eAlways},
		.back = {.failOp = vk::StencilOp::eKeep,
				.passOp = vk::StencilOp::eKeep,
				.depthFailOp = vk::StencilOp::eKeep,
				.compareOp = vk::CompareOp::eAlways},
		.minDepthBounds = 0.f,
		.maxDepthBounds = 1.0f };

	ms = vk::PipelineMultisampleStateCreateInfo{ .rasterizationSamples = vk::SampleCountFlagBits::e1 };
}

vk::UniquePipeline PipelineBuilder::build(const std::vector<vk::PipelineShaderStageCreateInfo>& pipelineShaderStageCreateInfos,
	vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass)
{
	vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
	.stageCount = (uint32_t)pipelineShaderStageCreateInfos.size(),
	.pStages = pipelineShaderStageCreateInfos.data(),
	.pVertexInputState = &vi,
	.pInputAssemblyState = &ia,
	.pViewportState = &vp,
	.pRasterizationState = &rs,
	.pMultisampleState = &ms,
	.pDepthStencilState = &ds,
	.pColorBlendState = &cb,
	.layout = pipelineLayout,
	.renderPass = renderPass };
	return device.createGraphicsPipelineUnique(pipelineCache, pipelineCreateInfo);
}

void PipelinePass::addShader(std::vector<char>& shaderCode, vk::ShaderStageFlagBits stage)
{
	LOGI("������ɫ��");

	vk::ShaderModuleCreateInfo shaderModuleCreateInfo{ .codeSize = shaderCode.size(),
		.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data()) };
	shaderModules.push_back(device.createShaderModuleUnique(shaderModuleCreateInfo));

	vk::PipelineShaderStageCreateInfo shaderCreateInfo{
		.stage = stage,
		.module = shaderModules.back().get(),
		.pName = "main" };
	pipelineShaderStageCreateInfos.push_back(shaderCreateInfo);

	// Generate reflection data for a shader
	SpvReflectShaderModule module;
	SpvReflectResult result = spvReflectCreateShaderModule(shaderCode.size(), shaderCode.data(), &module);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	uint32_t count = 0;
	result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	std::vector<SpvReflectDescriptorSet*> sets(count);
	result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	for (size_t i = 0; i < sets.size(); ++i)
	{
		const SpvReflectDescriptorSet& refl_set = *(sets[i]);
		auto& bindings = this->bindings[refl_set.set];
		for (uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding)
		{
			const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[i_binding]);
			auto& binding = bindings[refl_binding.binding];
			binding.binding = refl_binding.binding;
			binding.descriptorType = static_cast<vk::DescriptorType>(refl_binding.descriptor_type);
			binding.descriptorCount = 1;
			for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim)
			{
				binding.descriptorCount *= refl_binding.array.dims[i_dim];
			}
			binding.stageFlags |= static_cast<vk::ShaderStageFlagBits>(module.shader_stage);
		}
	}

	result = spvReflectEnumeratePushConstantBlocks(&module, &count, NULL);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	std::vector<SpvReflectBlockVariable*> pconstants(count);
	result = spvReflectEnumeratePushConstantBlocks(&module, &count, pconstants.data());
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	for (size_t i = 0; i < pconstants.size(); ++i)
	{
		vk::PushConstantRange pcs{
		.stageFlags = static_cast<vk::ShaderStageFlagBits>(module.shader_stage),
		.offset = pconstants[i]->offset,
		.size = pconstants[i]->size,
		};
		pushConstantRanges.push_back(pcs);
	}

	// Destroy the reflection data when no longer required.
	spvReflectDestroyShaderModule(&module);
}

void Crane::PipelinePass::buildDescriptorSetLayout()
{
	setLayouts.resize(bindings.size());
	descriptorSetLayouts.resize(bindings.size());
	for (size_t i = 0; i < bindings.size(); ++i)
	{
		vector<vk::DescriptorSetLayoutBinding> bindingsVec{};
		for (const auto& [s, b] : this->bindings[i])
			bindingsVec.push_back(b);
		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{ .bindingCount = (uint32_t)bindingsVec.size(),
			.pBindings = bindingsVec.data() };
		setLayouts[i] = device.createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo);
		descriptorSetLayouts[i] = setLayouts[i].get();
	}
}

void Crane::PipelinePass::buildPipelineLayout()
{
	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.setLayoutCount = (uint32_t)descriptorSetLayouts.size(),
		.pSetLayouts = descriptorSetLayouts.data(),
		.pushConstantRangeCount = (uint32_t)pushConstantRanges.size(),
		.pPushConstantRanges = pushConstantRanges.data() };
	pipelineLayout = device.createPipelineLayoutUnique(pipelineLayoutCreateInfo);
}

void Crane::PipelinePassCompute::buildPipeline(vk::PipelineCache pipelineCache)
{
	vk::PipelineShaderStageCreateInfo computeShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eCompute,
	.module = shaderModules[0].get(), .pName = "main" };
	vk::ComputePipelineCreateInfo computePipelineCreateInfo{ .stage = computeShaderStageCreateInfo,
		.layout = pipelineLayout.get(), };
	pipeline = device.createComputePipelineUnique(pipelineCache, computePipelineCreateInfo);
}

void Crane::PipelinePassGraphics::buildPipeline(PipelineBuilder& pipelineBuilder)
{
	pipeline = pipelineBuilder.build(pipelineShaderStageCreateInfos, pipelineLayout.get(), renderPass);
}

Material Crane::MaterialBuilder::build()
{
	Material m;
	m.pipelinePass = pipelinePass;

	vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{
			.descriptorPool = descriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(pipelinePass->descriptorSetLayouts.size()),
			.pSetLayouts = pipelinePass->descriptorSetLayouts.data() };
	m.descriptorSets = pipelinePass->device.allocateDescriptorSets(descriptorSetAllocateInfo);

	for (auto& [i, s] : pipelinePass->bindings)
	{
		for (auto& [j, b] : s)
		{
			vk::WriteDescriptorSet writeDescriptorSet{
				.dstSet = m.descriptorSets[i],
				.dstBinding = b.binding,
				.descriptorCount = b.descriptorCount,
				.descriptorType = b.descriptorType,
				.pImageInfo = descriptorInfos[i][j].second,
				.pBufferInfo = descriptorInfos[i][j].first};
			m.writeDescriptorSets.push_back(writeDescriptorSet);
		}
	}

	return m;
}

void Material::update()
{
	pipelinePass->device.updateDescriptorSets(writeDescriptorSets.size(),
		writeDescriptorSets.data(), 0, nullptr);
}
