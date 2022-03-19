#include "Collision.hpp"
#include "Rigidbody.hpp"
#include "Constraint.hpp"
#include "PositionBasedDynamics.hpp"

#include "Intersect.hpp"

using namespace std;
using namespace Eigen;
using namespace CranePhysics;

ParticleCollider::ParticleCollider(Particle *a) : Collider(dynamic_cast<Rigidbody *>(a)){};

void ParticleCollider::ColliderDispatch(CubeCollider *B, PositionBasedDynamics &pbd)
{
    Particle * rb1 = dynamic_cast<Particle *>(this->rb);
    Cube * rb2 = dynamic_cast<Cube *>(B->rb);

    auto rect = CollisionDetectFunction(*rb1, *rb2);

    if (rect.has_value())
    {
        Vector3f contactNormal = rect.value().first;
        contactNormal.normalize();
        float penetration = rect.value().second;
        pbd.constraints.emplace_back(
            std::make_shared<ParticleCubeCollisionConstraint>(
                *rb1, *rb2, contactNormal, penetration));
    }
}

void ParticleCollider::ColliderDispatch(ParticleCollider *B, PositionBasedDynamics &pbd)
{
    Particle * rb1 = dynamic_cast<Particle *>(this->rb);
    Particle * rb2 = dynamic_cast<Particle *>(B->rb);

    auto rect = CollisionDetectFunction(*rb1, *rb2);

    if (rect.has_value())
    {
        Vector3f contactNormal = rect.value().first;
        contactNormal.normalize();
        float penetration = rect.value().second;
        pbd.constraints.emplace_back(
            std::make_shared<ParticleRigidbodyCollisionConstraint>(
                *rb1, *rb2, contactNormal, penetration));
    }
}

CubeCollider::CubeCollider(Cube *a) : Collider(dynamic_cast<Rigidbody *>(a)){};

void CubeCollider::ColliderDispatch(ParticleCollider *B, PositionBasedDynamics &pbd)
{
    Particle * rb1 = dynamic_cast<Particle *>(B->rb);
    Cube * rb2 = dynamic_cast<Cube *>(this->rb);

    auto rect = CollisionDetectFunction(*rb1, *rb2);

    if (rect.has_value())
    {
        Vector3f contactNormal = rect.value().first;
        contactNormal.normalize();
        float penetration = rect.value().second;
        pbd.constraints.emplace_back(
            std::make_shared<ParticleCubeCollisionConstraint>(
                *rb1, *rb2, contactNormal, penetration));
    }
}

void CranePhysics::CollisionDetectDispatch(Rigidbody *a, Rigidbody *b, PositionBasedDynamics &pbd)
{
    Collider *colliderA = a->getCollider();
    Collider *colliderB = b->getCollider();
    colliderA->ColliderDispatch(colliderB, pbd);
}

optional<pair<Vector3f, float>> CranePhysics::CollisionDetectFunction(const Particle &a, const Cube &b)
{
    Vector3f bmax{b.width / 2, b.height / 2, b.depth / 2};
    Vector3f bmin{-b.width / 2, -b.height / 2, -b.depth / 2};

    return SphereCubeIntersect(bmin, bmax, b.position, b.rotation, a.position, a.radius);
}

std::optional<std::pair<Eigen::Vector3f, float>> CranePhysics::CollisionDetectFunction(const Particle &a, const Particle &b)
{
    return SphereSphereIntersect(a.position, a.radius, b.position, b.radius);
}
