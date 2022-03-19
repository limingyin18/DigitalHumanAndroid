#include "Render.hpp"

using namespace std;
using namespace Eigen;
using namespace Crane;


void Render::updateCullData()
{
	drawCullData.view = camera.view;//get_view_matrix();

	bufferDrawCullData.update(&drawCullData);
}

void Render::compactDraws()
{
	IndirectBatch firstDraw;
	firstDraw.renderable = &renderables[0];
	firstDraw.first = 0;
	firstDraw.count = 1;

	draws.push_back(firstDraw);
	drawsFlat.resize(renderables.size());

	for (int i = 1; i < renderables.size(); i++)
	{
		//compare the mesh and material with the end of the vector of draws
		bool sameMesh = renderables[i].mesh == draws.back().renderable->mesh;
		bool sameMaterial = renderables[i].material == draws.back().renderable->material;

		if (sameMesh && sameMaterial)
		{
			//all matches, add count
			draws.back().count++;
		}
		else
		{
			//add new draw
			IndirectBatch newDraw;
			newDraw.renderable = &renderables[i];
			newDraw.first = i;
			newDraw.count = 1;

			draws.push_back(newDraw);
		}
		drawsFlat[i].batchID = draws.size() - 1;
		drawsFlat[i].objectID = i;
	}

	bufferDrawsFlat.create(*vmaAllocator, sizeof(FlatBatch) * drawsFlat.size(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	bufferDrawsFlat.update(drawsFlat.data());
	descriptorBufferDrawsFlat.buffer = vk::Buffer(bufferDrawsFlat.buffer);
	descriptorBufferDrawsFlat.range = bufferDrawsFlat.size;

	cullObjCandidates.resize(renderables.size());
	for (auto& d : draws)
	{
		Vector4f spherebound = renderables[d.first].SphereBound();
		for (uint32_t i = d.first; i < d.count; ++i)
		{
			cullObjCandidates[i].model = *renderables[i].transformMatrix;
			cullObjCandidates[i].spherebound = spherebound;
			cullObjCandidates[i].cullFlag = renderables[i].cullFlag;
		}
	}
	bufferCullObjCandidate.create(*vmaAllocator, sizeof(ObjectData) * cullObjCandidates.size(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	bufferCullObjCandidate.update(cullObjCandidates.data());
	descriptorBufferInfoCullObjCandidate.buffer = vk::Buffer(bufferCullObjCandidate.buffer);
	descriptorBufferInfoCullObjCandidate.range = bufferCullObjCandidate.size;


	drawCullData.view = camera.view;
	drawCullData.fov = camera.fov;
	drawCullData.aspect = camera.aspect;
	drawCullData.znear = camera.near_;
	drawCullData.zfar = camera.far_;
	drawCullData.drawCount = static_cast<uint32_t>(renderables.size());

	bufferDrawCullData.create(*vmaAllocator, sizeof(DrawCullData),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	bufferDrawCullData.update(&drawCullData);
	descriptorBufferInfoCullData.buffer = vk::Buffer(bufferDrawCullData.buffer);
	descriptorBufferInfoCullData.range = bufferDrawCullData.size;

	bufferInstanceID.create(*vmaAllocator, sizeof(uint32_t)*renderables.size(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	vector<uint32_t> instaces(renderables.size());
	for (uint32_t i = 0; i < renderables.size(); ++i)
	{
		instaces[i] = i;
	}
	bufferInstanceID.update(instaces.data());
	descriptorBufferInfoInstanceID.buffer = vk::Buffer(bufferInstanceID.buffer);
	descriptorBufferInfoInstanceID.range = bufferInstanceID.size;
}


