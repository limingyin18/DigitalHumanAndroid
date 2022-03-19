#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <Eigen/Eigen>

#ifdef ANDROID
#include <android/asset_manager.h>
#endif

namespace Crane
{
#ifdef ANDROID
	extern AAssetManager *mgr;
#endif

	/*
	constexpr uint64_t prime = 0x100000001B3ull;
	constexpr uint64_t basis = 0xCBF29CE484222325ull;
	constexpr uint64_t hash_compile_time(char const* str, uint64_t last_value = basis)
	{
		return *str ? hash_compile_time(str + 1, (*str ^ last_value) * prime) : last_value;
	}

	uint64_t hash_(char const* str, uint64_t last_value = basis)
	{
		return *str ? hash_compile_time(str + 1, (*str ^ last_value) * prime) : last_value;
	}*/

	class Loader
	{
	public:

		struct MeshFaceIndices
		{
			int posIndices[3];
			int texIndices[3];
			int normalIndices[3];
		};

		struct ObjData
		{
			std::vector<Eigen::Vector3f> vertices;
			std::vector<Eigen::Vector3f> normals;
			std::vector<Eigen::Vector2f> texs;
			std::vector<MeshFaceIndices> faces;
		};


		static std::vector<char> readFile(const std::string& filename);

		static ObjData loadObj(const std::vector<char>& data);
	};
}
