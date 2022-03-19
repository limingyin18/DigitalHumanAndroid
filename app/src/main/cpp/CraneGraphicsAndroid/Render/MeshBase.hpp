#pragma once

#include <vector>
#include <functional>

#include <vulkan/vulkan.hpp>
#include <Eigen/Eigen>

namespace Crane
{
    /**
     * @brief 顶点结构
     */
    struct Vertex
    {
        Vertex() = default;
        Vertex(Eigen::Vector3f p, Eigen::Vector3f n, Eigen::Vector3f c, Eigen::Vector2f u):
            position{p}, normal{}, color{c}, uv{u}{};

        Eigen::Vector3f position{0.f, 0.f, 0.f}; // 位置
        Eigen::Vector3f normal{1.f, 1.f, 1.f};   // 法线
        Eigen::Vector3f color{ 1.f, 1.f, 1.f };  // 颜色
        Eigen::Vector2f uv{ 0.f, 0.f };          // 纹理

        static std::vector<vk::VertexInputBindingDescription> GetVertexInputBindingDescription();

        static std::vector<vk::VertexInputAttributeDescription> GetVertexInputAttributeDescription();

        static vk::PipelineInputAssemblyStateCreateFlags GetPipelineInputAssemblyStateCreateFlags();
    };

    /**
     * @brief 网格基类
     */
    class MeshBase
    {
    public:
        std::vector<Vertex> data;      // 顶点数组
        std::vector<uint32_t> indices; // 索引数组

        /**
         * @brief 设置顶点
         *
         * @param fun 设置函数
         */
        void setVertices(std::function<void(uint32_t, Vertex &)> const &fun);

        /**
         * @brief 法线计算
         * @details 三角形两边叉乘计算法线，相邻三角形面积控制占比
         */
        void recomputeNormals();
    };

    /**
     * @brief 平面
     */
    class Plane : public MeshBase
    {
    public:
        explicit Plane(uint32_t n, uint32_t m);
    };

    /**
     * @brief 立方体
     */
    class Cube : public MeshBase
    {
    public:
        explicit Cube(int n);
    };

    /**
     * @brief 棋盘
     */
    class Chessboard : public MeshBase
    {
    public:
        explicit Chessboard(uint32_t n, uint32_t m);
    };

    /**
     * @brief 球类
     */
    class Sphere : public MeshBase
    {
    public:
        explicit Sphere(uint32_t lng, uint32_t lat, float radius = 1.f);
    };

}
