#pragma once

#include "numeric"
#include "Rigidbody.hpp"


namespace CranePhysics
{
	class BVH
	{
		struct Node
		{
			Eigen::Vector3f aa;
			Eigen::Vector3f bb;

			Node* left = nullptr;
			Node* right = nullptr;
		};

	public:
		BVH() {};
		BVH(const std::vector<std::shared_ptr<CranePhysics::Rigidbody>>& rbs);

		Node* build(const std::vector<std::shared_ptr<CranePhysics::Rigidbody>>& rbs, std::vector<uint32_t>& index, uint32_t start, uint32_t end);

		Node* root = nullptr;
	};
}