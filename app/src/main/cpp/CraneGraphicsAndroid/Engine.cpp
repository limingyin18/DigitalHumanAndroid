#include "Engine.hpp"

using namespace std;
using namespace Crane;

Engine::Engine() : Render()
{
	camera.cameraMoveSpeed = 0.025f;
	camera.cameraRotateSpeed = 0.5f;

	rand_generator.seed(static_cast<unsigned>(time(nullptr)));
}

void Crane::Engine::initEngine()
{
	LOGI("创建剔除管线相关")
	{
		pipelinePassCull.device = device.get();
		auto shaderCodeCull = Loader::readFile("shaders/cull.comp.spv");
		pipelinePassCull.addShader(shaderCodeCull, vk::ShaderStageFlagBits::eCompute);
		pipelinePassCull.buildDescriptorSetLayout();
		pipelinePassCull.buildPipelineLayout();
		pipelinePassCull.buildPipeline(nullptr);

		materialBuilderCull.descriptorPool = descriptorPool.get();
		materialBuilderCull.pipelinePass = &pipelinePassCull;

		materialBuilderCull.descriptorInfos[0][0].first = &descriptorBufferInfoCullObjCandidate;
		materialBuilderCull.descriptorInfos[0][0].first = &descriptorBufferInfoCullObjCandidate; // object candidate
		materialBuilderCull.descriptorInfos[0][1].first = &descriptorBufferInfoIndirect; // draw command
		materialBuilderCull.descriptorInfos[0][2].first = &descriptorBufferDrawsFlat; // flat batch
		materialBuilderCull.descriptorInfos[0][3].first = &descriptorBufferInfoInstanceID; // instance id
		materialBuilderCull.descriptorInfos[0][5].first = &descriptorBufferInfoCullData; // cull data
		materialCull = materialBuilderCull.build();
	}

	LOGI("创建冯氏着色模型管线")
	{
		pipelinePassPhong.device = device.get();

		pipelinePassPhong.renderPass = renderPass.get();

		auto vertShaderCode = Loader::readFile("shaders/phong.vert.spv");
		pipelinePassPhong.addShader(vertShaderCode, vk::ShaderStageFlagBits::eVertex);
		auto fragShaderCode = Loader::readFile("shaders/phong.frag.spv");
		pipelinePassPhong.addShader(fragShaderCode, vk::ShaderStageFlagBits::eFragment);
		pipelinePassPhong.bindings[0][0].descriptorType = vk::DescriptorType::eUniformBufferDynamic;

		pipelinePassPhong.buildDescriptorSetLayout();

		pipelinePassPhong.buildPipelineLayout();
		pipelinePassPhong.buildPipeline(pipelineBuilder);

		// line
		pipelineBuilder.rs.polygonMode = vk::PolygonMode::eLine;

		pipelinePassLinePhong.device = device.get();

		pipelinePassLinePhong.renderPass = renderPass.get();

		pipelinePassLinePhong.addShader(vertShaderCode, vk::ShaderStageFlagBits::eVertex);
		pipelinePassLinePhong.addShader(fragShaderCode, vk::ShaderStageFlagBits::eFragment);
		pipelinePassLinePhong.bindings[0][0].descriptorType = vk::DescriptorType::eUniformBufferDynamic;

		pipelinePassLinePhong.buildDescriptorSetLayout();

		pipelinePassLinePhong.buildPipelineLayout();
		pipelinePassLinePhong.buildPipeline(pipelineBuilder);

		pipelineBuilder.rs.polygonMode = vk::PolygonMode::eFill;
	}

	LOGI("创建冯氏着色材质构建工厂")
	{
		materialBuilderPhong.descriptorPool = descriptorPool.get();
		materialBuilderPhong.pipelinePass = &pipelinePassPhong;
		materialBuilderPhong.descriptorInfos[0][0].first = &sceneParameterBufferDescriptorInfo;
		materialBuilderPhong.descriptorInfos[0][1].first = &modelMatrixBufferDescriptorInfo;
		materialBuilderPhong.descriptorInfos[0][2].first = &descriptorBufferInfoInstanceID;
		materialBuilderPhong.descriptorInfos[1][0].second = &descriptorImageInfoBlank;
	}
}

void Engine::updateEngine()
{
	auto timeNow = chrono::system_clock::now();
	std::chrono::duration<float> elapsed = timeNow - mTime;
	mTime = timeNow;
	dt = elapsed.count();
	dt = dt > 0.1f ? 0.1f : dt;


	dtAll += dt;

	updateInput();
}

void Engine::updateInput()
{
	if (input.keys["lctrl"])
	{
		float x = (input.mousePos.first - input.mousePosPrev.first) / width * 10;
		float y = (input.mousePos.second - input.mousePosPrev.second) / height * 10;

		camera.rotate(x, y);
		camera.focus(input.scrollOffset.first);

		input.mousePosPrev = input.mousePos;
		input.scrollOffset = { 0.f, 0.f };
	}

	if (input.keys["a"])
	{
		camera.target = camera.target - camera.right * camera.cameraMoveSpeed;
	}

	if (input.keys["w"])
	{
		camera.target = camera.target + camera.front * camera.cameraMoveSpeed;
	}

	if (input.keys["s"])
	{
		camera.target = camera.target - camera.front * camera.cameraMoveSpeed;
	}

	if (input.keys["d"])
	{
		camera.target = camera.target + camera.right * camera.cameraMoveSpeed;
	}

	if (input.keys["q"])
	{
		camera.target = camera.target + camera.up * camera.cameraMoveSpeed;
	}

	if (input.keys["e"])
	{
		camera.target = camera.target - camera.up * camera.cameraMoveSpeed;
	}

	camera.computeTransformation();
}

bool Engine::load_prefab(assets::PrefabInfo prefab, Eigen::Matrix4f root, vector<RenderableBase>& renderables, PipelinePassGraphics& pipelinePassGraphics)
{
	/*
	std::unordered_map<uint64_t, Eigen::Matrix4f> node_worldmats;

	std::vector<std::pair<uint64_t, Eigen::Matrix4f>> pending_nodes;
	for (auto& [k, v] : prefab.node_matrices)
	{
		Eigen::Matrix4f nodematrix = Eigen::Matrix4f::Identity();

		auto nm = prefab.matrices[v];
		memcpy(&nodematrix, &nm, sizeof(Eigen::Matrix4f));

		//check if it has parents
		auto matrixIT = prefab.node_parents.find(k);
		if (matrixIT == prefab.node_parents.end())
		{
			//add to worldmats
			node_worldmats[k] = root * nodematrix;
		}
		else
		{
			//enqueue
			pending_nodes.push_back({ k, nodematrix });
		}
	}

	//process pending nodes list until it empties
	while (pending_nodes.size() > 0)
	{
		for (int i = 0; i < pending_nodes.size(); i++)
		{
			uint64_t node = pending_nodes[i].first;
			uint64_t parent = prefab.node_parents[node];

			//try to find parent in cache
			auto matrixIT = node_worldmats.find(parent);
			if (matrixIT != node_worldmats.end())
			{

				//transform with the parent
				Eigen::Matrix4f nodematrix = (matrixIT)->second * pending_nodes[i].second;

				node_worldmats[node] = nodematrix;

				//remove from queue, pop last
				pending_nodes[i] = pending_nodes.back();
				pending_nodes.pop_back();
				i--;
			}
		}
	}

	renderables.reserve(renderables.size() + prefab.node_meshes.size());
	loadImages.reserve(renderables.size() + prefab.node_meshes.size());
	loadImageViews.reserve(renderables.size() + prefab.node_meshes.size());

	for (auto& [k, v] : prefab.node_meshes)
	{
		// 读取模型
		if (v.mesh_path.find("Sky") != std::string::npos)
			continue;

		assets::AssetFile file;
		if (!assets::load_binaryfile((string("assets/") + v.mesh_path).c_str(), file))
			throw std::runtime_error(string("Error when loading mesh ") + v.mesh_path.c_str());
		assets::MeshInfo meshinfo = assets::read_mesh_info(&file);

		if (loadMeshs.find(v.mesh_path.c_str()) == loadMeshs.end())
		{
			loadMeshs[v.mesh_path.c_str()] = make_shared<MeshBase>();
			loadMeshs[v.mesh_path.c_str()]->data.resize(meshinfo.vertexBuferSize / sizeof(Vertex));
			loadMeshs[v.mesh_path.c_str()]->indices.resize(meshinfo.indexBuferSize / sizeof(uint32_t));
			assets::unpack_mesh(&meshinfo, file.binaryBlob.data(), file.binaryBlob.size(),
				reinterpret_cast<char*>(loadMeshs[v.mesh_path.c_str()]->data.data()),
				reinterpret_cast<char*>(loadMeshs[v.mesh_path.c_str()]->indices.data()));


			materials[v.mesh_path.c_str()].descriptorPool = descriptorPool.get();
			materials[v.mesh_path.c_str()].pipelinePass = &pipelinePassGraphics;
			materials[v.mesh_path.c_str()].buildDescriptorSets();

			// 场景参数
			vk::WriteDescriptorSet writeDescriptorSet0B0{
				.dstSet = materials[v.mesh_path.c_str()].descriptorSets[0],
				.dstBinding = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eUniformBufferDynamic,
				.pBufferInfo = &sceneParameterBufferDescriptorInfo };
			materials[v.mesh_path.c_str()].writeDescriptorSets.push_back(writeDescriptorSet0B0);

			// instance
			vk::WriteDescriptorSet writeDescriptorSet0B2{
				.dstSet = materials[v.mesh_path.c_str()].descriptorSets[0],
				.dstBinding = 2,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eStorageBuffer,
				.pBufferInfo = &descriptorBufferInfoInstanceID };
			materials[v.mesh_path.c_str()].writeDescriptorSets.push_back(writeDescriptorSet0B2);

			// 模型位姿
			vk::WriteDescriptorSet writeDescriptorSet0B1{
			.dstSet = materials[v.mesh_path.c_str()].descriptorSets[0],
			.dstBinding = 1,
			.descriptorCount = 1,
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pBufferInfo = &modelMatrixBufferDescriptorInfo };
			materials[v.mesh_path.c_str()].writeDescriptorSets.push_back(writeDescriptorSet0B1);
		}

		renderables.emplace_back(loadMeshs[v.mesh_path.c_str()].get(), &materials[v.mesh_path.c_str()]);
		auto matrixIT = node_worldmats.find(k);
		if (matrixIT != node_worldmats.end())
			renderables.back().transformMatrix = node_worldmats[k];

		// 读取材质
		{
			assets::AssetFile materialFile;
			bool loaded = assets::load_binaryfile((string("assets/") + v.material_path).c_str(), materialFile);
			if (!loaded)
				throw std::runtime_error("load material failed: " + v.material_path);
			assets::MaterialInfo material = assets::read_material_info(&materialFile);

			// 基础颜色
			{
				auto nameBaseColor = material.textures["baseColor"];
				if (nameBaseColor == "") nameBaseColor = "default.tx";

				assets::AssetFile assetFileTexture;
				if (!load_binaryfile((string("assets/") + nameBaseColor).c_str(), assetFileTexture))
					throw runtime_error("load asset failed");

				assets::TextureInfo textureInfo = assets::read_texture_info(&assetFileTexture);
				VkDeviceSize imageSize = textureInfo.textureSize;
				vector<uint8_t> pixels(imageSize);
				assets::unpack_texture(&textureInfo, assetFileTexture.binaryBlob.data(),
					assetFileTexture.binaryBlob.size(), reinterpret_cast<char*>(pixels.data()));

				int texWidth, texHeight, texChannels;
				texChannels = 4;
				texWidth = textureInfo.pages[0].width;
				texHeight = textureInfo.pages[0].height;

				if (loadImageViews.find(nameBaseColor) == loadImageViews.end())
				{
					std::tie(loadImages[nameBaseColor], loadImageViews[nameBaseColor]) = createTextureImage(texWidth, texHeight, texChannels, pixels.data());
				}

				descriptorImageInfos[nameBaseColor].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				descriptorImageInfos[nameBaseColor].imageView = loadImageViews[nameBaseColor].get();
				descriptorImageInfos[nameBaseColor].sampler = textureSampler.get();

				vk::WriteDescriptorSet writeDescriptorSet1B0{
					.dstSet = materials[v.mesh_path.c_str()].descriptorSets[1],
					.dstBinding = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eCombinedImageSampler,
					.pImageInfo = &descriptorImageInfos[nameBaseColor] };
				materials[v.mesh_path.c_str()].writeDescriptorSets.push_back(writeDescriptorSet1B0);
			}

			// ��ȡ����ͼ
			{
				auto nameNormals = material.textures["normals"];
				if (nameNormals.size() <= 3)
				{
					nameNormals = "Sponza/glTF/white.tx";
				}
				assets::AssetFile assetFileTexture;
				if (!load_binaryfile((string("assets/") + nameNormals).c_str(), assetFileTexture))
					throw runtime_error("load asset failed");

				assets::TextureInfo textureInfo = assets::read_texture_info(&assetFileTexture);
				VkDeviceSize imageSize = textureInfo.textureSize;
				vector<uint8_t> pixels(imageSize);
				assets::unpack_texture(&textureInfo, assetFileTexture.binaryBlob.data(),
					assetFileTexture.binaryBlob.size(), reinterpret_cast<char*>(pixels.data()));

				int texWidth, texHeight, texChannels;
				texChannels = 4;
				texWidth = textureInfo.pages[0].width;
				texHeight = textureInfo.pages[0].height;

				if (normalImageViews.find(nameNormals) == normalImageViews.end())
				{
					std::tie(normalImages[nameNormals], normalImageViews[nameNormals]) =
						createTextureImage(texWidth, texHeight, texChannels, pixels.data());
				}
				//renderables.back().normal = normalImageViews[nameNormals];
			}

			// ��ȡ������ͼ
			{
				auto nameMetallic = material.textures["metallicRoughness"];
				if (nameMetallic.size() <= 3)
				{
					nameMetallic = "Sponza/glTF/white.tx";
				}
				assets::AssetFile assetFileTexture;
				if (!load_binaryfile((string("assets/") + nameMetallic).c_str(), assetFileTexture))
					throw runtime_error("load asset failed");

				assets::TextureInfo textureInfo = assets::read_texture_info(&assetFileTexture);
				VkDeviceSize imageSize = textureInfo.textureSize;
				vector<uint8_t> pixels(imageSize);
				assets::unpack_texture(&textureInfo, assetFileTexture.binaryBlob.data(),
					assetFileTexture.binaryBlob.size(), reinterpret_cast<char*>(pixels.data()));

				int texWidth, texHeight, texChannels;
				texChannels = 4;
				texWidth = textureInfo.pages[0].width;
				texHeight = textureInfo.pages[0].height;

				if (metallicImageViews.find(nameMetallic) == metallicImageViews.end())
				{
					std::tie(metallicImages[nameMetallic], metallicImageViews[nameMetallic]) =
						createTextureImage(texWidth, texHeight, texChannels, pixels.data());
				}

				//renderables.back().metallic = metallicImageViews[nameMetallic];
			}
		}
	}
	*/

	/*
			{
				vkutil::SampledTexture tex;
				tex.view = _loadedTextures[texture].imageView;
				tex.sampler = smoothSampler;

				vkutil::MaterialData info;
				info.parameters = nullptr;

				if (material.transparency == assets::TransparencyMode::Transparent)
				{
					info.baseTemplate = "texturedPBR_transparent";
				}
				else {
					info.baseTemplate = "texturedPBR_opaque";
				}

				info.textures.push_back(tex);

				objectMaterial = _materialSystem->build_material(materialName, info);
			}
		}


		MeshObject loadmesh;
		//transparent objects will be invisible

		loadmesh.bDrawForwardPass = true;
		loadmesh.bDrawShadowPass = true;


		Eigen::Matrix4f nodematrix{ 1.f };

		auto matrixIT = node_worldmats.find(k);
		if (matrixIT != node_worldmats.end())
		{
			auto nm = (*matrixIT).second;
			memcpy(&nodematrix, &nm, sizeof(Eigen::Matrix4f));
		}

		loadmesh.mesh = get_mesh(v.mesh_path.c_str());
		loadmesh.transformMatrix = nodematrix;
		loadmesh.material = objectMaterial;

		//refresh_renderbounds(&loadmesh);

		//sort key from location
		int32_t lx = int(loadmesh.bounds.origin.x / 10.f);
		int32_t ly = int(loadmesh.bounds.origin.y / 10.f);

		uint32_t key =  uint32_t(std::hash<int32_t>()(lx) ^ std::hash<int32_t>()(ly^1337));

		loadmesh.customSortKey = 0;// rng;// key;


		prefab_renderables.push_back(loadmesh);
		//_renderables.push_back(loadmesh);
	}

	_renderScene.register_object_batch(prefab_renderables.data(), static_cast<uint32_t>(prefab_renderables.size()));
*/
	return true;
}