#pragma once

#include <array>
#include <vector>
#include <limits>
#include <Eigen/Eigen>

namespace CranePhysics
{
    class AABBTree
    {
        struct Node
        {
            Node() : mNumFaces(0), mFaces(nullptr), mMinExtents(0.f, 0.f, 0.f), mMaxExtents(0.f, 0.f, 0.f)
            {
            }

            union
            {
                unsigned mChildren;
                unsigned mNumFaces;
            };

            unsigned *mFaces;
            Eigen::Vector3f mMinExtents;
            Eigen::Vector3f mMaxExtents;
        };

        struct Bounds
        {
            Bounds() : mMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
                       mMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max())
            {
            }

            Bounds(const Bounds &rhs) : mMin(rhs.mMin), mMax(rhs.mMax)
            {
            }

            Bounds(const Eigen::Vector3f &min, const Eigen::Vector3f &max) : mMin(min), mMax(max)
            {
            }

            float GetVolume() const
            {
                Eigen::Vector3f e = mMax - mMin;
                return e.prod();
            }

            float GetSurfaceArea() const
            {
                Eigen::Vector3f e = mMax - mMin;
                return 2.0f * (e[0] * e[1] + e[0] * e[2] + e[1] * e[2]);
            }

            void Union(const Bounds &b)
            {
                mMin[0] = std::min(mMin[0], b.mMin[0]);
                mMin[1] = std::min(mMin[1], b.mMin[1]);
                mMin[2] = std::min(mMin[2], b.mMin[2]);

                mMax[0] = std::max(mMax[0], b.mMax[0]);
                mMax[1] = std::max(mMax[1], b.mMax[1]);
                mMax[2] = std::max(mMax[2], b.mMax[2]);
            }

            Eigen::Vector3f mMin;
            Eigen::Vector3f mMax;
        };

        struct FaceSorter
        {
            FaceSorter(const Eigen::Vector3f *positions, const unsigned *indices, unsigned n, unsigned axis)
                : mVertices(positions), mIndices(indices), mNumIndices(n), mAxis(axis)
            {
            }

            bool operator()(unsigned lhs, unsigned rhs) const
            {
                float a = GetCentroid(lhs);
                float b = GetCentroid(rhs);

                if (a == b)
                    return lhs < rhs;
                else
                    return a < b;
            }

            float GetCentroid(unsigned face) const
            {
                const Eigen::Vector3f &a = mVertices[mIndices[face * 3 + 0]];
                const Eigen::Vector3f &b = mVertices[mIndices[face * 3 + 1]];
                const Eigen::Vector3f &c = mVertices[mIndices[face * 3 + 2]];

                return (a[mAxis] + b[mAxis] + c[mAxis]) / 3.0f;
            }

            const Eigen::Vector3f *mVertices;
            const unsigned *mIndices;
            unsigned mNumIndices;
            unsigned mAxis;
        };

    private:
        const Eigen::Vector3f *mVertices = nullptr;
        unsigned mNumVerts = 0;
        const unsigned *mIndices = nullptr;
        unsigned mNumFaces = 0;

        std::vector<unsigned> mFaces;
        std::vector<Node> mNodes;
        std::vector<Bounds> mFaceBounds;

        // track the next free node
        unsigned mFreeNode;

        // stats
        unsigned mTreeDepth = 0;
        unsigned mInnerNodes = 0;
        unsigned mLeafNodes = 0;

        size_t GetNumFaces() const { return mNumFaces; }
        size_t GetNumNodes() const { return mNodes.size(); }

        void CalculateFaceBounds(unsigned *faces, unsigned numFaces,
                                 Eigen::Vector3f &outMinExtents, Eigen::Vector3f &outMaxExtents);

        void BuildRecursive(unsigned nodeIndex, unsigned *faces, unsigned numFaces);
        unsigned PartitionSAH(Node &n, unsigned *faces, unsigned numFaces);

        /**
     * Moller and Trumbore's method
     */
        bool IntersectRayTriTwoSided(const Eigen::Vector3f &p, const Eigen::Vector3f &dir, const Eigen::Vector3f &a,
                                     const Eigen::Vector3f &b, const Eigen::Vector3f &c, float &t, float &u, float &v, float &w,
                                     float &sign) const;
        bool IntersectRayAABB(const Eigen::Vector3f &start, const Eigen::Vector3f &dir,
                              const Eigen::Vector3f &min, const Eigen::Vector3f &max, float &t,
                              Eigen::Vector3f *normal) const;
        void TraceRecursive(uint32_t nodeIndex,
                            const Eigen::Vector3f &start, const Eigen::Vector3f &dir,
                            float &outT, float &u, float &v, float &w,
                            float &faceSign, uint32_t &faceIndex) const;

    public:
        AABBTree(const Eigen::Vector3f *vertices, unsigned numVerts,
                 const unsigned *indices, unsigned numFaces);
        ~AABBTree();

        bool TraceRay(const Eigen::Vector3f &start, const Eigen::Vector3f &dir, float &outT,
                      float &u, float &v, float &w, float &faceSign, uint32_t &faceIndex) const;
    };
} // namespace PiratePhysics