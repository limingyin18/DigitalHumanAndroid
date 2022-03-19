#pragma once

#include <optional>

#include <Eigen/Eigen>

namespace CranePhysics
{
    class Rigidbody;
    class Particle;
    class Cube;
    class ParticleCollider;
    class CubeCollider;
    class PositionBasedDynamics;

    void CollisionDetectDispatch(Rigidbody *a, Rigidbody *b, PositionBasedDynamics &pbd);
    std::optional<std::pair<Eigen::Vector3f, float>> CollisionDetectFunction(const Particle &a, const Cube &b);
    std::optional<std::pair<Eigen::Vector3f, float>> CollisionDetectFunction(const Particle &a, const Particle &b);

    /**
     * ��ײ���ӿ�
     */
    class Collider
    {
    public:
        Collider(Rigidbody *r) : rb{r} {};

        Rigidbody *rb;

        virtual void ColliderDispatch(Collider *b, PositionBasedDynamics &pbd) = 0;
        virtual void ColliderDispatch(ParticleCollider *B, PositionBasedDynamics &pbd) = 0;
        virtual void ColliderDispatch(CubeCollider *B, PositionBasedDynamics &pbd) = 0;
    };

    /**
     * ������ײ��
     */
    class ParticleCollider : public Collider
    {
    public:
        ParticleCollider(Particle *a);

        void ColliderDispatch(Collider *b, PositionBasedDynamics &pbd) override
        {
            b->ColliderDispatch(this, pbd);
        }

        void ColliderDispatch(ParticleCollider *B, PositionBasedDynamics &pbd) override;

        void ColliderDispatch(CubeCollider *B, PositionBasedDynamics &pbd) override;
    };

    /**
     * ��������ײ��
     */
    class CubeCollider : public Collider
    {
    public:
        CubeCollider(Cube *a);

        void ColliderDispatch(Collider *b, PositionBasedDynamics &pbd) override
        {
            b->ColliderDispatch(this, pbd);
        }
        void ColliderDispatch(ParticleCollider *B, PositionBasedDynamics &pbd) override;

        void ColliderDispatch(CubeCollider *B, PositionBasedDynamics &pbd) override
        {
        }
    };

}