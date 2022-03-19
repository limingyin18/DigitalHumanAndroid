#include "Intersect.hpp"

using namespace std;
using namespace Eigen;
using namespace CranePhysics;

optional<pair<Vector3f, float>> CranePhysics::SphereCubeIntersect(
    Vector3f bmin, Vector3f bmax, Vector3f c, Eigen::Quaternionf t, Vector3f p, float r)
{
    // ת��Բ��p������������ϵ
    Vector3f pc = p - c;
    Quaternionf pcQ = t.inverse() * Quaternionf(0, pc.x(), pc.y(), pc.z()) * t;
    Vector3f p1{pcQ.x(), pcQ.y(), pcQ.z()};

    // ����������ӽ�Բ�ĵĵ�q
    Vector3f q1 = p1.cwiseMax(bmin).cwiseMin(bmax);

    // ת�������q����������ϵ
    Quaternionf qQ = t * Quaternionf(0, q1.x(), q1.y(), q1.z()) * t.inverse();
    Vector3f q{qQ.x(), qQ.y(), qQ.z()};
    q += c;

    // �����嵽Բ�ĵ����ʸ��
    Vector3f u = p - q;

    float l = u.squaredNorm();

    // �����ڳ�������
    if (l == 0)
    {
        uint32_t indexMin, indexMax;
        float minMin = (p1 - bmin).minCoeff(&indexMin);
        float minMax = (bmax- p1).minCoeff(&indexMax);

        if (minMin < minMax) //��������ǰ��
        {
            u[indexMin] = 1.0f;
            Quaternionf uQ = t.inverse() * Quaternionf(0, u.x(), u.y(), u.z()) * t;
            Vector3f u1{ uQ.x(), uQ.y(), uQ.z() };
            return pair{u1, r + minMin};

        }
        else //�������Ϻ���
        {
            u[indexMax] = 1.0f;
            Quaternionf uQ = t.inverse() * Quaternionf(0, u.x(), u.y(), u.z()) * t;
            Vector3f u1{ uQ.x(), uQ.y(), uQ.z() };
            return pair{u1, r + minMax};
        }
    }
    if (l < r * r)
        return pair{u, r - u.norm()};
    else
        return nullopt;
}

std::optional<std::pair<Eigen::Vector3f, float>> CranePhysics::SphereSphereIntersect(
    Eigen::Vector3f c1, float r1, Eigen::Vector3f c2, float r2)
{
    Vector3f d = c1-c2;
    float r = r1 + r2;
    if(d.squaredNorm() <= r*r)
        return pair{d, r - d.norm()};
    else
        return nullopt;
}

std::optional<std::pair<float, float>> CranePhysics::RaySphereIntersect(const Eigen::Vector3f& originRay, const Eigen::Vector3f& direction, const Eigen::Vector3f& originSphere, float radius)
{
    Vector3f oc = originRay - originSphere;
    float a = direction.dot(direction);
    float b = oc.dot(direction);
    float c = oc.dot(oc) - radius * radius;
    float discriminant = b * b - (a * c);
	if (discriminant > 0.0f)
	{
		float tmp = sqrt(discriminant);
		float t0 = (-b - tmp) / a;
		float t1 = (-b + tmp) / a;
        return pair{ t0, t1 };
	}
    else
        return nullopt;
}
