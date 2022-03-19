#include "Utils.hpp"

using namespace std;
using namespace Eigen;
using namespace Crane;

#ifdef ANDROID
AAssetManager* Crane::mgr = nullptr;
std::vector<char> Crane::Loader::readFile(const std::string& filename)
{
	if(mgr == nullptr)
	{
		throw runtime_error{"AassetManager null"};
	}

	AAsset *file = AAssetManager_open(mgr, filename.c_str(), AASSET_MODE_BUFFER);
	if (!file)
	{
		throw runtime_error{"Unknown error. Couldn't open file."};
	}

	vector<char> file_contents(AAsset_getLength(file));

	if (AAsset_read(file, file_contents.data(), file_contents.size()) != file_contents.size())
	{
		AAsset_close(file);
		throw runtime_error{"Unknown error. Couldn't load file contents."};
	}
	AAsset_close(file);

	return file_contents;
}
#else
std::vector<char> Crane::Loader::readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("failed to open file!");

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}
#endif

Loader::ObjData Loader::loadObj(const vector<char>& data)
{
	ObjData obj;
	bool vt = false;
	bool vn = false;

	std::stringstream is;
	is.rdbuf()->pubsetbuf(const_cast<char*>(data.data()), data.size());
/*
	string line;
	while (getline(is, line))
	{
		stringstream lineStream(line);
		string type;
		lineStream >> type;

		switch (hash_(type.c_str()))
		{
		case hash_compile_time("v"):
		{
			Vector3f vertex;
			for (uint32_t i = 0; i < 3; ++i)
			{
				string v;
				lineStream >> v;
				float x = stof(v);
				vertex[i] = x;
			}
			obj.vertices.push_back(vertex);

			break;
		}
		case hash_compile_time("vt"):
		{
			Vector2f tex;
			for (uint32_t i = 0; i < 2; ++i)
			{
				string t;
				lineStream >> t;
				tex[i] = stof(t);
			}
			obj.texs.push_back(tex);
			vt = true;
			break;
		}
		case hash_compile_time("vn"):
		{
			Vector3f normal;
			for (uint32_t i = 0; i < 3; ++i)
			{
				string n;
				lineStream >> n;
				normal[i] = stof(n);
			}
			obj.texs.push_back(normal);
			vn = true;
			break;
		}
		case hash_compile_time("f"):
		{
			MeshFaceIndices faceIndex;
			for (uint32_t i = 0; i < 3; ++i)
			{
				string vertexIndex;
				lineStream >> vertexIndex;
				stringstream vertexIndexStream(vertexIndex);

				string pos;
				std::getline(vertexIndexStream, pos, '/');
				faceIndex.posIndices[i] = stoi(pos);

				if (vt)
				{
					string tex;
					std::getline(vertexIndexStream, tex, '/');
					faceIndex.texIndices[i] = stoi(tex);
				}
				if (vn)
				{
					string normal;
					std::getline(vertexIndexStream, normal, '/');
					faceIndex.normalIndices[i] = stoi(normal);
				}
			}

			obj.faces.push_back(faceIndex);
		}
		default:
			break;
		}
	}
*/
	return obj;
}
