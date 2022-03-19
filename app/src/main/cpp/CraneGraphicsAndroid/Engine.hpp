#pragma once

#include <chrono>
#include <random>

#include "Render/Render.hpp"
#include "Actor.hpp"
#include "Input.hpp"
#include "Utils.hpp"
#include "Physics/PositionBasedDynamics.hpp"
#include "Physics/Constraint.hpp"
#include "Physics/Voxelize.hpp"
#include "Physics/BVH.hpp"

#include "asset_loader.h"
#include "texture_asset.h"
#include "mesh_asset.h"
#include "prefab_asset.h"
#include "material_asset.h"

#include <android_native_app_glue.h>
#include "PirateNdkEngine/NdkUtils.hpp"

namespace Crane
{
    /**
     * engine base class
     */
    class Engine : public Render
    {
    public:
        explicit Engine();
        virtual ~Engine() = default;
        Engine(const Engine& rhs) = delete;
        Engine(Engine&& rhs) = delete;
        Engine& operator=(const Engine& rhs) = delete;
        Engine& operator=(Engine&& rhs) = delete;

        void initEngine() override;
        void updateEngine();
        void updateInput();
        Input input;

        bool load_prefab(assets::PrefabInfo prefab, Eigen::Matrix4f root, std::vector<Crane::RenderableBase>& renderables, PipelinePassGraphics &pipelinePassGraphics);

    public:
        std::chrono::time_point<std::chrono::system_clock> mTime =
            std::chrono::system_clock::now();
        float dtAll = 0.0f;
        float dt = 0.0f;

        std::default_random_engine rand_generator;
        std::normal_distribution<float> normal_dist{0.f, 1.f};
    };
}