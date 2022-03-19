#include "Camera.hpp"

using namespace std;
using namespace Eigen;
using namespace Crane;

Camera::Camera()
{
    rotate(0, 0);
    computeTransformation();
}

void Camera::computeTransformation()
{
    assert(aspect != 0);
    assert(far_ != near_);

    projection << 1 / (aspect * std::tan(fov / 2)), 0, 0, 0,
        0, 1 / std::tan(fov / 2), 0, 0,
        0, 0, -(far_ + near_) / (far_ - near_), -2 * far_ * near_ / (far_ - near_),
        0, 0, -1, 0;

    /*
    view = (Translation<float, 3>(Vector3f{0.0f, 0.f, -targetDistance}) *
            AngleAxis<float>(-rotation[1], up) *
            AngleAxis<float>(-rotation[0], right) *
            Translation<float, 3>(-target) * 
            AngleAxis<float>(3.14f, Vector3f(0.0f, 0.0f, 1.0f)))
               .matrix();
    */

    view = (AngleAxis<float>(rotation[1], up) *
            AngleAxis<float>(rotation[0], right) * 
            Translation<float, 3>(-target)
            )
            .matrix();
}

Vector3f Camera::getCameraPos() const
{
    /*
    auto targetRot = AngleAxis<float>(rotation[1], up) *
                     AngleAxis<float>(rotation[0], right);
    auto targetDistRot = targetRot * Vector3f{0.0f, 0.f, targetDistance};
    */
    return target;
}

void Camera::rotate(float x, float y)
{
    rotation[0] += cameraRotateSpeed * x;
    rotation[1] += cameraRotateSpeed * y;
    right = AngleAxis<float>(-rotation[1], up).matrix()* Vector3f(1.f, 0.f, 0.f);
    right.normalize();
    front = up.cross(right);
    front.normalize();
}

void Camera::focus(float d)
{
    targetDistance += cameraZoomSpeed * d;
}