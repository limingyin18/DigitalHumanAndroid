#pragma once

#include <vector>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include "spirv_reflect.h"

#include "Logging.hpp"
#include "MeshBase.hpp"
#include "Buffer.hpp"


namespace Crane
{
	class PipelineBuilder
	{
	public:
		PipelineBuilder();

		vk::Device device;
		vk::PipelineCache pipelineCache;

		std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions;
		vk::PipelineVertexInputStateCreateInfo vi;

		vk::PipelineInputAssemblyStateCreateInfo ia;

		vk::PipelineRasterizationStateCreateInfo rs;

		vk::PipelineColorBlendAttachmentState att_state[1];
		vk::PipelineColorBlendStateCreateInfo cb;

		vk::Viewport viewport;

		vk::Rect2D scissor;

		vk::PipelineViewportStateCreateInfo vp;

		vk::PipelineDepthStencilStateCreateInfo ds;

		vk::PipelineMultisampleStateCreateInfo ms;


		vk::UniquePipeline build(const std::vector<vk::PipelineShaderStageCreateInfo>& pipelineShaderStageCreateInfos,
			vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass);
	};


	class PipelinePass
	{
	public:
		vk::Device device;

		vk::UniquePipeline pipeline;
		vk::UniquePipelineLayout pipelineLayout;

		std::vector<vk::UniqueShaderModule> shaderModules;
		std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

		std::vector<vk::PushConstantRange> pushConstantRanges;
		std::unordered_map<uint32_t, std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding>> bindings;

		std::vector<vk::UniqueDescriptorSetLayout> setLayouts;
		std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;

		void addShader(std::vector<char>& shaderCode, vk::ShaderStageFlagBits);
		void buildDescriptorSetLayout();
		void buildPipelineLayout();
	};

	class PipelinePassCompute : public PipelinePass
	{
	public:
		void buildPipeline(vk::PipelineCache pipelineCache = nullptr);
	};

	class PipelinePassGraphics : public PipelinePass
	{
	public:
		vk::RenderPass renderPass;

		void buildPipeline(PipelineBuilder& pipelineBuilder);
	};

	
	class Material
	{
	public:
		std::vector<vk::WriteDescriptorSet> writeDescriptorSets;
		std::vector<vk::DescriptorSet> descriptorSets;

		PipelinePass* pipelinePass = nullptr;
		void update();
	};

	class MaterialBuilder
	{
	public:
		vk::DescriptorPool descriptorPool;
		PipelinePass* pipelinePass = nullptr;

		std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::pair<vk::DescriptorBufferInfo*, vk::DescriptorImageInfo*>>> descriptorInfos;

		virtual Material build();
	};
}
