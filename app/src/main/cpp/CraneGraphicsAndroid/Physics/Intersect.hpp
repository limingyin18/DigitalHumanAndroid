#include <optional>

#include <Eigen/Eigen>

namespace CranePhysics
{
    /**
     * @brief �����볤�����ཻ����
     * 
     * @param bmin - ��������С��
     * @param bmax - ����������
     * @param c - ����������
     * @param t - ��������ת
     * @param p - ��������
     * @param r - ����뾶
     * 
     * @return - �Ӵ��㷨�ߣ���͸���
     */
    std::optional<std::pair<Eigen::Vector3f, float>> SphereCubeIntersect(
        Eigen::Vector3f bmin, Eigen::Vector3f bmax, Eigen::Vector3f c, Eigen::Quaternionf t,
        Eigen::Vector3f p, float r);

    /**
     * @brief �����������ཻ����
     * 
     * @param c1 ����1
     * @param r1 �뾶1
     * @param c2 ����2
     * @param r2 �뾶2
     * 
     * @return �Ӵ��㷨�ߣ���͸���
     */
    std::optional<std::pair<Eigen::Vector3f, float>> SphereSphereIntersect(
        Eigen::Vector3f c1, float r1, Eigen::Vector3f c2, float r2);

    /**
     * @brief �����������ཻ����
     * @param originRay �������
     * @param direction ���߷���
     * @param originSphere ��������
     * @param radius ����뾶
     * @return �ཻ��λ�ò���
    */
    std::optional<std::pair<float, float >> RaySphereIntersect(const Eigen::Vector3f& originRay, const Eigen::Vector3f& direction,
        const Eigen::Vector3f& originSphere, float radius);
}