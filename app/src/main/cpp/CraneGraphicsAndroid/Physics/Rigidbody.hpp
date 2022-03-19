#pragma once

#include <vector>
#include <optional>
#include <memory>
#include <Eigen/Eigen>

#include "Collision.hpp"

using namespace Eigen;

namespace CranePhysics
{
    /**
     * ����
     */
    class Rigidbody
    {
    public:
        float invMass = 0.f;
        Matrix3f inertiaMoment;

        Vector3f velocity = Vector3f{0.f, 0.f, 0.f};
        Vector3f angularVelocity = Vector3f{ 0.f, 0.f, 0.f };
        Vector3f position = Vector3f{0.f, 0.f, 0.f};
        Quaternionf rotation = Quaternionf{1.0f, 0.f, 0.f, 0.f};

        Vector3f velocityPrime = Vector3f{0.f, 0.f, 0.f};
        Vector3f positionPrime = Vector3f{0.f, 0.f, 0.f};

        Vector3f aa, bb;

    public:
        explicit Rigidbody(float inv = 1.f);
        virtual ~Rigidbody() = default;

        virtual void computeAABB() = 0;

        virtual Collider* getCollider() = 0;
    };

    /**
     * ����/����
     */
    class Particle : public Rigidbody
    {
    public:
        explicit Particle(float inv = 1.f);
        ~Particle() = default;

        ParticleCollider* getCollider() override
        {
            return &collider;
        };

        void computeAABB() override
        {
            aa = position.array() - radius;
            bb = position.array() + radius;
        };

        float radius = 1.0f;
        ParticleCollider collider;
    };

    using Sphere = Particle;

    /**
     * ������
     */
    class Cube : public Rigidbody
    {
    public:
        explicit Cube(float inv = 1.f);
        ~Cube() = default;

        CubeCollider* getCollider() override
        {
            return &collider;
        };
        
        void computeAABB() override
        {
            aa = Vector3f{ position.x() - width / 2.f, position.y() - height / 2.f, position.z() - depth / 2.f, };
            bb = Vector3f{ position.x() + width / 2.f, position.y() + height / 2.f, position.z() + depth / 2.f, };
        };

        float width = 1.0f;
        float height = 1.0f;
        float depth = 1.0f;

        CubeCollider collider;
    };
}
