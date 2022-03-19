#pragma once

#include <vector>
#include <memory>
#include "Rigidbody.hpp"
#include "Collision.hpp"
#include "BVH.hpp"

namespace CranePhysics
{
    class Constraint;

    class PositionBasedDynamics
    {
    public:
        std::vector<std::shared_ptr<Constraint>> constraints;
        std::vector<std::shared_ptr<Rigidbody>> rigidbodies;
        float dt;

        BVH bvh;

    public:
        PositionBasedDynamics(/* args */);
        ~PositionBasedDynamics();

        void run();
        void externalForceIntegral();
        void generateCollisionConstraint();
        void internalForceIntegral();
        void updateVelocity();
    };

    
}