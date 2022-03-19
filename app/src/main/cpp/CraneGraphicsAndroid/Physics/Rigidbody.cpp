#include "Rigidbody.hpp"

using namespace CranePhysics;

Rigidbody::Rigidbody(float inv) : invMass{inv}
{
}

Particle::Particle(float inv) : Rigidbody(inv), collider(this)
{
	if(invMass != 0.f)
		inertiaMoment = 2.f/ 5.f /invMass * radius * radius * Matrix3f::Ones();
}

Cube::Cube(float inv) : Rigidbody(inv), collider(this)
{
	if (invMass != 0.f)
		inertiaMoment = (1.f/invMass/12.f * 
			Vector3f{ depth * depth + height * height, 
					  width * width + depth * depth, 
					  width * width + height * height }).asDiagonal();
}