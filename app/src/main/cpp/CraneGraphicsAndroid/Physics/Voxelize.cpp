#include "Voxelize.hpp"
#include "AABBTree.hpp"

using namespace std;
using namespace Eigen;
using namespace CranePhysics;

namespace CranePhysics
{
	void Voxelize(const Eigen::Vector3f *vertices, int numVertices,
				  const unsigned *indices, unsigned numFaces, unsigned width, unsigned height,
				  unsigned depth, vector<unsigned> &volume, Vector3f minExtents, Vector3f maxExtents)
	{
		volume.resize(width * height * depth);

		// build an aabb tree of the mesh
		AABBTree tree(vertices, numVertices, (const uint32_t *)indices, numFaces);

		// parity count method, single pass
		const Vector3f extents(maxExtents - minExtents);
		const Vector3f delta(extents[0] / width, extents[1] / height, extents[2] / depth);
		const Vector3f offset(0.5f * delta[0], 0.5f * delta[1], 0.5f * delta[2]);

		// this is the bias we apply to step 'off' a triangle we hit, not very robust
		const float eps = 0.00001f * extents[0];

		for (uint32_t x = 0; x < width; ++x)
		{
			for (uint32_t y = 0; y < height; ++y)
			{
				bool inside = false;

				Vector3f rayDir = Vector3f(0.0f, 0.0f, 1.0f);
				Vector3f rayStart = minExtents + Vector3f(x * delta[0] + offset[0], y * delta[1] + offset[1], 0.0f);

				uint32_t lastTri = uint32_t(-1);
				for (;;)
				{
					// calculate ray start
					float t, u, v, w, s;
					uint32_t tri;

					if (tree.TraceRay(rayStart, rayDir, t, u, v, w, s, tri))
					{
						// calculate cell in which intersection occurred
						const float zpos = rayStart[2] + t * rayDir[2];
						const float zhit = (zpos - minExtents[2]) / delta[2];

						uint32_t z = uint32_t(floorf((rayStart[2] - minExtents[2]) / delta[2] + 0.5f));
						uint32_t zend = std::min(uint32_t(floorf(zhit + 0.5f)), depth - 1);

						if (inside)
						{
							// march along column setting bits
							for (uint32_t k = z; k < zend; ++k)
							{
								//volume[k * width * height + y * width + x] = uint32_t(-1);
								volume[k * width * height + y * width + x] = uint32_t(1);
							}
						}

						inside = !inside;

						// we hit the tri we started from
						if (tri == lastTri)
							printf("Error self-intersect\n");
						lastTri = tri;

						rayStart += rayDir * (t + eps);
					}
					else
						break;
				}
			}
		}
	}
} // namespace PiratePhysics