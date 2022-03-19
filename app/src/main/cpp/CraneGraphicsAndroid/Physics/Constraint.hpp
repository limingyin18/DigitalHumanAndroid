#pragma once

#include <vector>
#include <Eigen/Eigen>
#include "Rigidbody.hpp"

namespace CranePhysics
{
    class Constraint
    {
    public:
        Constraint() = default;
        virtual ~Constraint() = default;

        virtual void solveConstraint() = 0;
        virtual void solveVelocity() {};
    };

    class Stretching : public Constraint
    {
    private:
        float restLength = 0.f;
        float compressionStiffness = 1.0f;
        float stretchStiffness = 1.0f;
        Rigidbody &rb0, &rb1;

    public:
        Stretching(Rigidbody &r0, Rigidbody &r1, float restL, float compress, float stretch);
        virtual ~Stretching() = default;

        void solveConstraint() override;
    };

    class ShapeMatchingConstraint : public Constraint
    {
    public:
        ShapeMatchingConstraint(std::vector<Rigidbody*> &rs);
        virtual ~ShapeMatchingConstraint() = default;

        void solveConstraint() override;

    private:
        Vector3f mRestCM;
        Matrix3f mInvRestMat;
        std::vector<Eigen::Vector3f> mX0;
        std::vector<Eigen::Vector3f> mX;
        std::vector<Eigen::Vector3f> mCorr;
        std::vector<Eigen::Vector3f> mNumClusters;
        std::vector<float> mW;
        std::vector<Rigidbody*> rbs;
    };

    class LongRangeAttachmentConstraint : public Constraint
    {
    public:
        LongRangeAttachmentConstraint(Rigidbody &at, Rigidbody &r1, float restL, float stretch);
        virtual ~LongRangeAttachmentConstraint() = default;

        void solveConstraint() override;

    private:
        Rigidbody &attachemnt;
        Rigidbody &rb1;
        float restLength = 0.f;
        float stretchStiffness = 1.0f;
    };

    class FollowTheLeaderConstraint : public Constraint
    {
    public:
        FollowTheLeaderConstraint(std::vector<std::shared_ptr<Rigidbody>> &rs);
        virtual ~FollowTheLeaderConstraint() = default;

        void solveConstraint() override;
    private:
        std::vector<std::shared_ptr<Rigidbody>> rbs;
        std::vector<float> ds;
    };

    class CollisionConstraint : public Constraint
    {
    public:
        Rigidbody& rb1;
        Rigidbody& rb2;
        Eigen::Vector3f contactNormal;
        float penetration;

        CollisionConstraint(Rigidbody& rb1, Rigidbody& rb2, Eigen::Vector3f cN, float pt);

        void solveVelocity() override;
    };

    class ParticleRigidbodyCollisionConstraint : public Constraint
    {
    public:
        ParticleRigidbodyCollisionConstraint(Particle &p, Rigidbody &rb, Eigen::Vector3f cN, float pt);
        void solveConstraint() override;
        void solveVelocity() override;

        Particle &particle;
        Rigidbody &rigidbody;
        Eigen::Vector3f contactNormal;
        float penetration;
    };

    class ParticleCubeCollisionConstraint : public CollisionConstraint
    {
    public:
        ParticleCubeCollisionConstraint(Particle &p, Cube &rb, Eigen::Vector3f cN, float pt);
        void solveConstraint() override;

        Particle &particle;
        Cube &cube;
    };
}