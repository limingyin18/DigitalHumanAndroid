#include "PositionBasedDynamics.hpp"
#include "Constraint.hpp"

#include <iostream>

using namespace std;
using namespace CranePhysics;

static const float g = 9.8f;

PositionBasedDynamics::PositionBasedDynamics(/* args */)
{
}

PositionBasedDynamics::~PositionBasedDynamics()
{
}


void PositionBasedDynamics::externalForceIntegral()
{
    for (auto rb : rigidbodies)
    {
        // ÖØÁ¦
        if (rb->invMass > 0.f)
        {
            rb->velocity[1] += -dt * g;
            rb->position += dt * rb->velocity;
        }
    }
}

void PositionBasedDynamics::internalForceIntegral()
{
    for (size_t i = 0; i < 5; ++i)
    {
        for (auto &c : constraints)
        {
            c->solveConstraint();
        }
    }
}

void PositionBasedDynamics::updateVelocity()
{
    for (auto rb : rigidbodies)
    {
        rb->velocity = (rb->position - rb->positionPrime) / dt;
    }

    for (auto& c : constraints)
    {
        c->solveVelocity();
    }
}

void PositionBasedDynamics::generateCollisionConstraint()
{
    for (uint32_t i = 0; i < rigidbodies.size() - 1; ++i)
    {
        for (uint32_t j = i + 1; j < rigidbodies.size(); ++j)
        {
            CollisionDetectDispatch(rigidbodies[i].get(), rigidbodies[j].get(), *this);
        }
    }
}

void PositionBasedDynamics::run()
{
    externalForceIntegral();
    int n = constraints.size();
    generateCollisionConstraint();
    internalForceIntegral();
    updateVelocity();

    while (constraints.size() > n)
        constraints.pop_back();

    for (auto rb : rigidbodies)
    {
        rb->velocityPrime = rb->velocity;
        rb->positionPrime = rb->position;
    }
}