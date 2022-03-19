#include "Actor.hpp"

using namespace std;
using namespace Eigen;
using namespace Crane;

void Actor::setPosition(Eigen::Vector3f p)
{
	position = p;

	transform = (Translation3f(position) * rotationQ).matrix();
}

void Actor::setRotation(Eigen::Vector3f r)
{
	rotation = r;
	auto aa = AngleAxisf(rotation[0], Vector3f::UnitX());
	rotationQ = Quaternionf(aa);
	auto bb = AngleAxisf(rotation[1], Vector3f::UnitY());
	rotationQ *= Quaternionf(bb);
	auto cc = AngleAxisf(rotation[2], Vector3f::UnitZ());
	rotationQ *= Quaternionf(cc);

	transform = (Translation3f(position) * rotationQ).matrix();
}

void Actor::computeAABB()
{
    extentMin = Vector3f(numeric_limits<float>::max(), numeric_limits<float>::max(), numeric_limits<float>::max());
    extentMax = Vector3f(-numeric_limits<float>::max(), -numeric_limits<float>::max(), -numeric_limits<float>::max());

    for(unsigned i = 0; i < mesh->data.size(); ++i)
    {
        const Vector3f &v = mesh->data[i].position;

        extentMin[0] = min(v[0], extentMin[0]);
        extentMin[1] = min(v[1], extentMin[1]);
        extentMin[2] = min(v[2], extentMin[2]);
        extentMax[0] = max(v[0], extentMax[0]);
        extentMax[1] = max(v[1], extentMax[1]);
        extentMax[2] = max(v[2], extentMax[2]);
    }
}
