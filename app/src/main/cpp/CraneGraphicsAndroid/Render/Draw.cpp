#include "Render.hpp"

using namespace std;
using namespace Eigen;
using namespace Crane;

void Crane::Render::draw()
{
	commandBuffer[currBuffIndex]->reset(vk::CommandBufferResetFlagBits{});
	vk::CommandBufferBeginInfo commandBufferBeginInfo{};
	commandBuffer[currBuffIndex]->begin(commandBufferBeginInfo);

	if (currBuffIndex == 0)
	{
		//TracyVkZone(tracyVkCtx, commandBuffer[currBuffIndex].get(), "All Frame");
	}

	vk::RenderPassBeginInfo rpBeginInfo{
		.renderPass = renderPass.get(),
		.framebuffer = framebuffers[currBuffIndex].get(),
		.renderArea = {.offset = {.x = 0, .y = 0},
						.extent = {.width = width, .height = height}},
		.clearValueCount = 2,
		.pClearValues = clearValues };
	commandBuffer[currBuffIndex]->beginRenderPass(rpBeginInfo, vk::SubpassContents::eInline);

	// begin
	{
		vk::Pipeline pipelineLast = nullptr;

		commandBuffer[currBuffIndex]->bindVertexBuffers(uint32_t(0), 1, (vk::Buffer*)&vertBuff.buffer, vertOffsets);
		commandBuffer[currBuffIndex]->bindIndexBuffer(indexBuffer.buffer, 0, vk::IndexType::eUint32);
		for (uint32_t i = 0; i < draws.size(); ++i)
		{
			IndirectBatch& draw = draws[i];
			vk::Pipeline pipelineNew = draw.renderable->material->pipelinePass->pipeline.get();
			vk::PipelineLayout pipelineLayout = draw.renderable->material->pipelinePass->pipelineLayout.get();
			vk::DescriptorSet* descriptorSetP = draw.renderable->material->descriptorSets.data();
			uint32_t descriptorSetCount = draw.renderable->material->descriptorSets.size();
			MeshBase *meshNew = draw.renderable->mesh;
			if (pipelineNew != pipelineLast)
			{
				commandBuffer[currBuffIndex]->bindPipeline(vk::PipelineBindPoint::eGraphics, pipelineNew);

				commandBuffer[currBuffIndex]->pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
					0, sizeof(cameraPushConstants[currBuffIndex]), &cameraPushConstants[currBuffIndex]);

				pipelineLast = pipelineNew;
			}

			vector<uint32_t> offsets{ sceneParametersUniformOffset * currBuffIndex };
			commandBuffer[currBuffIndex]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSetCount, descriptorSetP, offsets.size(), offsets.data());
			VkDeviceSize offsetIndirect = i * sizeof(vk::DrawIndexedIndirectCommand);
			commandBuffer[currBuffIndex]->drawIndexedIndirect(bufferIndirect.buffer, offsetIndirect, 1, sizeof(vk::DrawIndexedIndirectCommand));
		}
	}
	// end

	commandBuffer[currBuffIndex]->endRenderPass();
	commandBuffer[currBuffIndex]->end();

	submitInfo.pCommandBuffers = &commandBuffer[currBuffIndex].get();
	submitInfo.pWaitSemaphores = &imageAcquiredSemaphores[currentFrame].get();
	submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame].get();

	presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame].get();

	if (drawGUIFlag) // drawGUIFlag
	{
		graphicsQueue.submit(submitInfo);
		drawGUI();
	}
	else
	{
		graphicsQueue.submit(submitInfo, inFlightFences[currentFrame].get());
	}
}

void Crane::Render::update()
{

	device->waitForFences(inFlightFences[currentFrame].get(), true, UINT64_MAX);
	currBuffIndex = device->acquireNextImageKHR(swapchain.get(), UINT64_MAX, imageAcquiredSemaphores[currentFrame].get());
	device->waitForFences(imagesInFlightFences[currBuffIndex], true, UINT64_MAX);
	imagesInFlightFences[currBuffIndex] = inFlightFences[currentFrame].get();
	device->resetFences(inFlightFences[currentFrame].get());

	updateApp();

	updateCameraBuffer();
	updateSceneParameters();

	updateCullData();

	VmaAllocationInfo allocInfo;
	vmaGetAllocationInfo(*vmaAllocator, bufferIndirect.bufferMemory, &allocInfo);
	auto bufferIndirectP = static_cast<vk::DrawIndexedIndirectCommand*>(allocInfo.pMappedData);
	for (uint32_t i = 0, firstIndex = 0; i < draws.size(); ++i)
	{
		bufferIndirectP[i].instanceCount = 0;
	}

	computeQueue.submit(1, &submitInfoCompute, vk::Fence{});
	computeQueue.waitIdle();

	draw();

	//present
	presentInfo.pImageIndices = &currBuffIndex;
	presentQueue.presentKHR(presentInfo);
	currentFrame = (currentFrame + 1) % swapchainImages.size();
}

void Crane::Render::updateCameraBuffer()
{
	Eigen::Vector4f position;
	position.head(3) = camera.getCameraPos();
	position[3] = 1.0f;
	cameraPushConstants[currBuffIndex].position = position;
	cameraPushConstants[currBuffIndex].projView = camera.projection * camera.view;
}

void Crane::Render::updateSceneParameters()
{
	VmaAllocationInfo allocInfo;
	vmaGetAllocationInfo(*vmaAllocator, sceneParameterBuffer.bufferMemory, &allocInfo);
	memcpy(static_cast<uint8_t*>(allocInfo.pMappedData) +
		(sceneParametersUniformOffset * currBuffIndex),
		&sceneParameters, sizeof(SceneParameters));
}
