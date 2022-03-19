#pragma once

#include <array>
#include <vector>
#include <Eigen/Eigen>

namespace CranePhysics
{
    // voxelizes a mesh using a single pass parity algorithm
    void Voxelize(const Eigen::Vector3f *vertices, int numVertices,
                  const unsigned *indices, unsigned numFaces, unsigned width, unsigned height,
                  unsigned depth, std::vector<unsigned> &volume, Eigen::Vector3f minExtents, Eigen::Vector3f maxExtents);
} // namespace PiratePhysics