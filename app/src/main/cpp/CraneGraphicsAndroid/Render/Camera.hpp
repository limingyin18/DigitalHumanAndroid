#pragma once

#include <algorithm>
#include <iostream>

#include "Eigen/Eigen"
#include "Eigen/Geometry"

namespace Crane
{
class Camera
{
public:
    float fov = 3.14f / 4.0f;
    float aspect = 4.0f/3.0f;
    float near_ = 0.1f;
    float far_ = 10000.0f;

    float targetDistance = 0.0f;
    Eigen::Vector3f target = {0.0f, 0.0f, 0.0f};
    Eigen::Vector3f rotation ={0.f, 0.0f, 0.0f};
    Eigen::Vector3f up = {0.0f, 1.0f, 0.0f};
    Eigen::Vector3f right = {1.0f, 0.0f, 0.0f};
    Eigen::Vector3f front = {0.0f, 0.0f, 1.0f};

    float cameraMoveSpeed = 1.0f;
    float cameraRotateSpeed = 1.0f;
    float cameraZoomSpeed = 1.0f;

    Eigen::Matrix4f view;
    Eigen::Matrix4f projection;

    void computeTransformation();
public:
    explicit Camera();
    ~Camera() = default;

    void rotate(float x, float y);
    void focus(float d);

    Eigen::Vector3f getCameraPos() const;
};

}
