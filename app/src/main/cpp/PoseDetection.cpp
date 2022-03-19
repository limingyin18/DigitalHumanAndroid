#include "PoseDetection.hpp"
#include <stb/stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include <stb/stb_image_resize.h>

using namespace std;
using namespace Crane;
using namespace PirateNdkEngine;

constexpr uint32_t MAX_IMAGES = 2;

PoseDetection::PoseDetection(ANativeWindow *win) :
        Engine(),
        window{win},
        imageReader(1280,
                    960,
                    AIMAGE_FORMAT_YUV_420_888,
                    AHARDWAREBUFFER_USAGE_CPU_READ_MASK,
                    MAX_IMAGES)
{
    preferPresentMode = vk::PresentModeKHR::eFifo;
    camera.target = Vector3f{0.f, 0.f, 10.f};
    camera.rotation[0] = -0.0f;

    //cameraNDK.create(imageReader.GetWindow());
    //cameraNDK.start_capturing();
}

PoseDetection::~PoseDetection()
{
    device->waitIdle();
}

void PoseDetection::updateApp()
{
    try
    {
        /*
        //AHardwareBuffer *buffer = imageReader.GetLatestBuffer();
        AImage *image = nullptr;
        AHardwareBuffer *buffer = nullptr;
        auto res = AImageReader_acquireLatestImage(imageReader.reader_.get(), &image);
        // acquire image
        if (res != AMEDIA_OK || image == nullptr)
        {
            clog << to_string(res) << endl;
            AImage_delete(image);
        }

        if (image != nullptr)
        {
            int32_t widthI, heightI;
            AImage_getWidth(image, &widthI);
            AImage_getHeight(image, &heightI);
            cout << "camera image width: " << widthI << " " << heightI << endl;

            //void* bufferData = NULL;
            //AImage_getHardwareBuffer(image, &buffer);
            //AHardwareBuffer_lock(buffer, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, nullptr, &bufferData);
            PirateNdkEngine::YUV420PTORGBA(out, image);
            auto singleCmd = beginSingleTimeCommands();
            imageBackground.update(out, singleCmd);
            endSingleTimeCommands(singleCmd);
            //AHardwareBuffer_unlock(buffer, nullptr);
            AImage_delete(image);
        }
         */
    }
    catch (std::exception &e)
    {
        cerr << e.what() << endl;
        throw std::runtime_error(e.what());
    }

    updateEngine();

    auto modelMatrixPtr = modelMatrix.data();
    for (auto &v : renderables)
    {
        *(reinterpret_cast<Eigen::Matrix4f *>(modelMatrixPtr)) = *v.transformMatrix;
        modelMatrixPtr += modelMatrixOffset;
    }
    modelMatrixBuffer.update(modelMatrix.data());
}

void PoseDetection::createSurface()
{
    VkAndroidSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.window = window;

    VkSurfaceKHR surface;
    if (vkCreateAndroidSurfaceKHR(VkInstance(instance.get()), &createInfo, nullptr, &surface) !=
        VK_SUCCESS)
        throw runtime_error("failed to create surface");

    this->surface = vk::UniqueSurfaceKHR(vk::SurfaceKHR(surface), instance.get());
}

void PoseDetection::createAssetApp()
{
    out = new uint32_t[1280 * 960];
    std::tie(imageBackground, imageViewBackground) = createTextureImage(960, 1280, 4, (void *) out);

    //initCamImageVulkan();

    LOGI("create background pipeline")
    {
        pipelineBackground.device = device.get();

        pipelineBackground.renderPass = renderPass.get();

        auto vertShaderCode = Loader::readFile("shaders/background.vert.spv");
        pipelineBackground.addShader(vertShaderCode, vk::ShaderStageFlagBits::eVertex);
        auto fragShaderCode = Loader::readFile("shaders/background.frag.spv");
        pipelineBackground.addShader(fragShaderCode, vk::ShaderStageFlagBits::eFragment);
        //pipelineBackground.bindings[0][0].descriptorType = vk::DescriptorType::eUniformBufferDynamic;

        pipelineBackground.buildDescriptorSetLayout();

        pipelineBackground.buildPipelineLayout();
        pipelineBackground.buildPipeline(pipelineBuilder);
    }

    LOGI("create background material builder")
    {
        materialBuilderBackground.descriptorPool = descriptorPool.get();
        materialBuilderBackground.pipelinePass = &pipelineBackground;
        materialBuilderBackground.descriptorInfos[0][1].first = &modelMatrixBufferDescriptorInfo;
        materialBuilderBackground.descriptorInfos[0][2].first = &descriptorBufferInfoInstanceID;
        materialBuilderBackground.descriptorInfos[1][0].second = &descriptorImageInfoBackground;
    }

    LOGI("load image")
    {
        vector<char> imageByte = readFile(mgr, "cai.png");
        int x, y, c;
        unsigned char *input_pixels = stbi_load_from_memory(
                reinterpret_cast<const stbi_uc *>(imageByte.data()), imageByte.size(),
                &x, &y, &c, 3);
        pixels = static_cast<unsigned char *>(malloc(256 * 256 * 3));
        stbir_resize_uint8(input_pixels, x, y, 0,
                           pixels, 256, 256, 0, 3);


        pixelsTexture = stbi_load_from_memory(
                reinterpret_cast<const stbi_uc *>(imageByte.data()), imageByte.size(),
                &x, &y, &c, 4);
        //std::tie(imageBackground, imageViewBackground) = createTextureImage(x, y, 4, pixelsTexture);

        descriptorImageInfoBackground.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        descriptorImageInfoBackground.imageView = imageViewBackground.get();
        descriptorImageInfoBackground.sampler = textureSampler.get();
        //descriptorImageInfoBackground.imageView = texImageView;
        //descriptorImageInfoBackground.sampler = samplerYcbcr;
    }

    LOGI("create plane");
    {
        string name = "plane";
        if (loadMeshs.find(name) == loadMeshs.end())
        {
            loadMeshs[name] = make_shared<Crane::Plane>(2, 2);
            loadMeshs[name]->setVertices([](uint32_t, Vertex &v)
                                         {
                                             v.position.y() = -v.position.z();
                                             v.position.z() = 1;
                                         });
        }
        plane.mesh = loadMeshs[name];

        materials[name] = materialBuilderBackground.build();
        plane.material = &materials[name];

        //plane.setRotation(Vector3f{3.14f / 2.f, 0.f, 0.f});
        renderables.emplace_back(plane.mesh.get(), plane.material, &plane.transform);
    }

    LOGI("create marker")
    {
        Actor &actor = marker;
        string name = "marker";
        if (loadMeshs.find(name) == loadMeshs.end())
        {
            loadMeshs[name] = make_shared<Crane::Plane>(2, 2);
            loadMeshs[name]->setVertices([](uint32_t, Vertex &v)
                                         {
                                             v.position.y() = -v.position.z();
                                             v.position.z() = 1.f;

                                             v.position.x() *= 0.025f;
                                             v.position.y() *= 0.025f;
                                         });
        }
        actor.mesh = loadMeshs[name];

        materialBuilderBackground.descriptorInfos[1][0].second = &descriptorImageInfoLilac;
        materials[name] = materialBuilderBackground.build();
        actor.material = &materials[name];
        renderables.emplace_back(actor.mesh.get(), actor.material, &actor.transform);

        for (int i = 0; i < markers.size(); ++i)
        {
            markers[i].mesh = marker.mesh;
            markers[i].material = marker.material;
            markers[i].setPosition(Vector3f{-0.02f * i, 0.02f * i, 0.f});
            renderables.emplace_back(markers[i].mesh.get(), markers[i].material,
                                     &markers[i].transform);
        }
    }
}

void
PoseDetection::setMarkerPos(std::array<std::pair<std::pair<uint32_t, uint32_t>, float>, 17> &preds)
{
    for (int i = 0; i < preds.size(); ++i)
    {
        float y = 2 * float(64 - preds[i].first.first) / 64.f - 1.f;
        float x = 2 * float(preds[i].first.second) / 64.f - 1.f;
        Vector3f pos = {x, y, 0.f};
        markers[i].setPosition(pos);
    }
}
