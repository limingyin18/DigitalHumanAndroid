#include "BVH.hpp"

using namespace std;
using namespace CranePhysics;

BVH::BVH(const std::vector<std::shared_ptr<CranePhysics::Rigidbody>>& rbs)
{
	vector<uint32_t> index(rbs.size());
	iota(index.begin(), index.end(), 0);
	root = build(rbs, index, 0, rbs.size());
}

BVH::Node* BVH::build(const std::vector<std::shared_ptr<CranePhysics::Rigidbody>>& rbs, std::vector<uint32_t>& index, uint32_t start, uint32_t end)
{
	uint32_t n = end - start;
	Node *node = new Node();
	if (n == 1)
	{
		node->aa = rbs[start]->aa;
		node->bb = rbs[start]->bb;
		return node;
	}

	uint32_t axis = 0;
	sort(index.begin() + start, index.begin() + end, [&rbs, axis](uint32_t indA, uint32_t indB)
		{return rbs[indA]->aa[axis] < rbs[indA]->aa[axis]; });
	node->left = build(rbs, index, start, start + n / 2);
	node->right = build(rbs, index, start + n / 2, end);

	node->aa[0] = min(node->left->aa[0], node->right->bb[0]);
	node->aa[1] = min(node->left->aa[1], node->right->bb[1]);
	node->aa[2] = min(node->left->aa[2], node->right->bb[2]);
	node->bb[0] = max(node->left->bb[0], node->right->bb[0]);
	node->bb[1] = max(node->left->bb[1], node->right->bb[1]);
	node->bb[2] = max(node->left->bb[2], node->right->bb[2]);

	return node;
}