#include "AABBTree.hpp"

using namespace Eigen;
using namespace std;
using namespace CranePhysics;

static unsigned sDepth = 0;

AABBTree::AABBTree(const Vector3f *vertices, unsigned numVerts,
                   const unsigned *indices, unsigned numFaces) : mVertices(vertices), mNumVerts(numVerts),
                                                                              mIndices(indices), mNumFaces(numFaces)
{
    mFaces.reserve(mNumFaces);
    mFaceBounds.reserve(mNumFaces);

    for (unsigned i = 0; i < numFaces; ++i)
    {
        Bounds top;
        CalculateFaceBounds(&i, 1, top.mMin, top.mMax);
        mFaces.push_back(i);
        mFaceBounds.push_back(top);
    }

    mNodes.reserve(static_cast<unsigned>(numFaces*1.5f));
    mFreeNode = 1;

    BuildRecursive(0, &mFaces[0], numFaces);
    assert(sDepth == 0);
}

AABBTree::~AABBTree()
{
    mIndices = nullptr;
    mVertices = nullptr;
}

void AABBTree::CalculateFaceBounds(unsigned *faces, unsigned numFaces,
                                   Eigen::Vector3f &outMinExtents, Eigen::Vector3f &outMaxExtents)
{
    Vector3f minExtents(numeric_limits<float>::max(), numeric_limits<float>::max(), numeric_limits<float>::max());
    Vector3f maxExtents(-numeric_limits<float>::max(), -numeric_limits<float>::max(), -numeric_limits<float>::max());

    // calculate face bounds
    for(unsigned i = 0; i < numFaces; ++i)
    {
        Vector3f a = mVertices[mIndices[faces[i]*3+0]];
        Vector3f b = mVertices[mIndices[faces[i]*3+1]];
        Vector3f c = mVertices[mIndices[faces[i]*3+2]];

        minExtents[0] = min(a[0], minExtents[0]);
        minExtents[1] = min(a[1], minExtents[1]);
        minExtents[2] = min(a[2], minExtents[2]);
        maxExtents[0] = max(a[0], maxExtents[0]);
        maxExtents[1] = max(a[1], maxExtents[1]);
        maxExtents[2] = max(a[2], maxExtents[2]);

        minExtents[0] = min(b[0], minExtents[0]);
        minExtents[1] = min(b[1], minExtents[1]);
        minExtents[2] = min(b[2], minExtents[2]);
        maxExtents[0] = max(b[0], maxExtents[0]);
        maxExtents[1] = max(b[1], maxExtents[1]);
        maxExtents[2] = max(b[2], maxExtents[2]);

        minExtents[0] = min(c[0], minExtents[0]);
        minExtents[1] = min(c[1], minExtents[1]);
        minExtents[2] = min(c[2], minExtents[2]);
        maxExtents[0] = max(c[0], maxExtents[0]);
        maxExtents[1] = max(c[1], maxExtents[1]);
        maxExtents[2] = max(c[2], maxExtents[2]);
    }

    outMinExtents = minExtents;
    outMaxExtents = maxExtents;
}

void AABBTree::BuildRecursive(unsigned nodeIndex, unsigned *faces, unsigned numFaces)
{
    const unsigned kMaxFacesPerLeaf = 6;

    // allocate more memory
    if(nodeIndex >= mNodes.size())
    {
        unsigned s = max(static_cast<unsigned>(1.5f*mNodes.size()), 512U);
        mNodes.resize(s);
    }

    Node &n = mNodes[nodeIndex];
    ++sDepth;
    mTreeDepth = max(mTreeDepth, sDepth);

    CalculateFaceBounds(faces, numFaces, n.mMinExtents, n.mMaxExtents);

    // calculate bounds of faces and add node
    if(numFaces <= kMaxFacesPerLeaf)
    {
        n.mFaces = faces;
        n.mNumFaces = numFaces;
        ++mLeafNodes;
    }
    else
    {
        ++mInnerNodes;

        // face counts for each branch
        const unsigned leftCount = PartitionSAH(n, faces, numFaces);
        const unsigned rightCount = numFaces - leftCount;

        // alloc
        mNodes[nodeIndex].mChildren = mFreeNode;
        mFreeNode += 2;

        // split faces in half and build each side recursively
        BuildRecursive(mNodes[nodeIndex].mChildren+0, faces, leftCount);
        BuildRecursive(mNodes[nodeIndex].mChildren+1, faces+leftCount, rightCount);
    }

    --sDepth;
    
}

unsigned AABBTree::PartitionSAH(Node &n, unsigned *faces, unsigned numFaces)
{
    unsigned bestAxis = 0;
    unsigned bestIndex = 0;
    float bestCost = numeric_limits<float>::max();

    for(unsigned a = 0; a < 3; ++a)
    {
        // sort faces by centroids
        		// sort faces by centroids
		FaceSorter predicate(&mVertices[0], &mIndices[0], mNumFaces*3, a);
		std::sort(faces, faces+numFaces, predicate);

        		// two passes over data to calculate upper and lower bounds
		vector<float> cumulativeLower(numFaces);
		vector<float> cumulativeUpper(numFaces);

		Bounds lower;
		Bounds upper;

		for (uint32_t i=0; i < numFaces; ++i)
		{
			lower.Union(mFaceBounds[faces[i]]);
			upper.Union(mFaceBounds[faces[numFaces-i-1]]);

			cumulativeLower[i] = lower.GetSurfaceArea();        
			cumulativeUpper[numFaces-i-1] = upper.GetSurfaceArea();
		}

		float invTotalSA = 1.0f / cumulativeUpper[0];

		// test all split positions
		for (uint32_t i=0; i < numFaces-1; ++i)
		{
			float pBelow = cumulativeLower[i] * invTotalSA;
			float pAbove = cumulativeUpper[i] * invTotalSA;

			float cost = 0.125f + (pBelow*i + pAbove*(numFaces-i));
			if (cost <= bestCost)
			{
				bestCost = cost;
				bestIndex = i;
				bestAxis = a;
			}
		}
    }

    // re-sort by best axis
	FaceSorter predicate(&mVertices[0], &mIndices[0], mNumFaces*3, bestAxis);
	std::sort(faces, faces+numFaces, predicate);

	return bestIndex+1;
}

bool AABBTree::TraceRay(const Eigen::Vector3f& start, const Vector3f& dir, float& outT,
        float& u, float& v, float& w, float& faceSign, uint32_t& faceIndex) const
{
    Vector3f rcp_dir(1.0f/dir[0], 1.0f/dir[1], 1.0f/dir[2]);

    outT = numeric_limits<float>::max();

    TraceRecursive(0, start, dir, outT, u, v, w, faceSign, faceIndex);

    return outT != numeric_limits<float>::max();
}

void AABBTree::TraceRecursive(uint32_t nodeIndex,
    const Eigen::Vector3f& start, const Eigen::Vector3f& dir,
    float& outT, float& outU, float& outV, float& outW,
    float& faceSign, uint32_t& faceIndex) const
{
	const Node& node = mNodes[nodeIndex];

    if (node.mFaces == NULL)
    {
        // find closest node
        const Node& leftChild = mNodes[node.mChildren+0];
        const Node& rightChild = mNodes[node.mChildren+1];

        float dist[2] = {numeric_limits<float>::max(), numeric_limits<float>::max()};

        IntersectRayAABB(start, dir, leftChild.mMinExtents, leftChild.mMaxExtents, dist[0], NULL);
        IntersectRayAABB(start, dir, rightChild.mMinExtents, rightChild.mMaxExtents, dist[1], NULL);
        
        uint32_t closest = 0;
        uint32_t furthest = 1;
		
        if (dist[1] < dist[0])
        {
            closest = 1;
            furthest = 0;
        }		

        if (dist[closest] < outT)
            TraceRecursive(node.mChildren+closest, start, dir, outT, outU, outV, outW, faceSign, faceIndex);

        if (dist[furthest] < outT)
            TraceRecursive(node.mChildren+furthest, start, dir, outT, outU, outV, outW, faceSign, faceIndex);
    }
    else
    {
        Vector3f normal;
        float t, u, v, w, s;

        for (uint32_t i=0; i < node.mNumFaces; ++i)
        {
            uint32_t indexStart = node.mFaces[i]*3;

            const Vector3f& a = mVertices[mIndices[indexStart+0]];
            const Vector3f& b = mVertices[mIndices[indexStart+1]];
            const Vector3f& c = mVertices[mIndices[indexStart+2]];


            if (IntersectRayTriTwoSided(start, dir, a, b, c, t, u, v, w, s))
            {
                if (t < outT)
                {
                    outT = t;
					outU = u;
					outV = v;
					outW = w;
					faceSign = s;
					faceIndex = node.mFaces[i];
                }
            }
        }
    }
}

bool AABBTree::IntersectRayAABB(const Vector3f& start, const Vector3f& dir,
    const Vector3f& min, const Vector3f& max, float& t, Vector3f* normal) const
{
	//! calculate candidate plane on each axis
	float tx = -1.0f, ty = -1.0f, tz = -1.0f;
	bool inside = true;
			
	//! use unrolled loops

	//! x
	if (start[0] < min[0])
	{
		if (dir[0] != 0.0f)
			tx = (min[0]-start[0])/dir[0];
		inside = false;
	}
	else if (start[0] > max[0])
	{
		if (dir[0] != 0.0f)
			tx = (max[0]-start[0])/dir[0];
		inside = false;
	}

	//! y
	if (start[1] < min[1])
	{
		if (dir[1] != 0.0f)
			ty = (min[1]-start[1])/dir[1];
		inside = false;
	}
	else if (start[1] > max[1])
	{
		if (dir[1] != 0.0f)
			ty = (max[1]-start[1])/dir[1];
		inside = false;
	}

	//! z
	if (start[2] < min[2])
	{
		if (dir[2] != 0.0f)
			tz = (min[2]-start[2])/dir[2];
		inside = false;
	}
	else if (start[2] > max[2])
	{
		if (dir[2] != 0.0f)
			tz = (max[2]-start[2])/dir[2];
		inside = false;
	}

	//! if point inside all planes
	if (inside)
    {
        t = 0.0f;
		return true;
    }

	//! we now have t values for each of possible intersection planes
	//! find the maximum to get the intersection point
	float tmax = tx;
	int taxis = 0;

	if (ty > tmax)
	{
		tmax = ty;
		taxis = 1;
	}
	if (tz > tmax)
	{
		tmax = tz;
		taxis = 2;
	}

	if (tmax < 0.0f)
		return false;

	//! check that the intersection point lies on the plane we picked
	//! we don't test the axis of closest intersection for precision reasons

	//! no eps for now
	float eps = 0.0f;

	Vector3f hit = start + dir*tmax;

	if ((hit[0] < min[0]-eps || hit[0] > max[0]+eps) && taxis != 0)
		return false;
	if ((hit[1] < min[1]-eps || hit[1] > max[1]+eps) && taxis != 1)
		return false;
	if ((hit[2] < min[2]-eps || hit[2] > max[2]+eps) && taxis != 2)
		return false;

	//! output results
	t = tmax;
			
	return true;
}


bool AABBTree::IntersectRayTriTwoSided(const Vector3f& p, const Vector3f& dir, const Vector3f& a,
        const Vector3f& b, const Vector3f& c, float& t, float& u, float& v, float& w,
        float& sign) const
{
    Vector3f ab = b - a;
    Vector3f ac = c - a;
    Vector3f n = ab.cross(ac);

    float d = n.dot(-dir);
    float ood = 1.0f / d; // No need to check for division by zero here as infinity aritmetic will save us...
    Vector3f ap = p - a;

    t = ap.dot(n) * ood;
    if (t < 0.0f)
        return false;

    Vector3f e = (-dir).cross(ap);
    v = ac.dot(e) * ood;
    if (v < 0.0f || v > 1.0f) // ...here...
        return false;
    w = -ab.dot(e) * ood;
    if (w < 0.0f || v + w > 1.0f) // ...and here
        return false;

    u = 1.0f - v - w;
    //if (normal)
        //*normal = n;
	sign = d;

    return true;
}