#include <optional>

#include <Eigen/Eigen>

namespace CranePhysics
{
    /**
     * @brief 球体与长方体相交测试
     * 
     * @param bmin - 长方体最小点
     * @param bmax - 长方体最大点
     * @param c - 长方体中心
     * @param t - 长方体旋转
     * @param p - 球体中心
     * @param r - 球体半径
     * 
     * @return - 接触点法线，穿透深度
     */
    std::optional<std::pair<Eigen::Vector3f, float>> SphereCubeIntersect(
        Eigen::Vector3f bmin, Eigen::Vector3f bmax, Eigen::Vector3f c, Eigen::Quaternionf t,
        Eigen::Vector3f p, float r);

    /**
     * @brief 球体与球体相交测试
     * 
     * @param c1 球心1
     * @param r1 半径1
     * @param c2 球心2
     * @param r2 半径2
     * 
     * @return 接触点法线，穿透深度
     */
    std::optional<std::pair<Eigen::Vector3f, float>> SphereSphereIntersect(
        Eigen::Vector3f c1, float r1, Eigen::Vector3f c2, float r2);

    /**
     * @brief 射线与球体相交测试
     * @param originRay 射线起点
     * @param direction 射线方向
     * @param originSphere 球体中心
     * @param radius 球体半径
     * @return 相交点位置参数
    */
    std::optional<std::pair<float, float >> RaySphereIntersect(const Eigen::Vector3f& originRay, const Eigen::Vector3f& direction,
        const Eigen::Vector3f& originSphere, float radius);
}