namespace Crane
{
	struct SceneParameters
	{
		Eigen::Vector4f fogColor;     // w is for exponent
		Eigen::Vector4f fogDistances; //x for min, y for max, zw unused.
		Eigen::Vector4f ambientColor;
		Eigen::Vector4f sunlightDirection; //w for sun power
		Eigen::Vector4f sunlightColor;
	};

	struct CameraPushConstant
	{
		Eigen::Vector4f position;
		Eigen::Matrix4f projView;
	};

	struct IndirectBatch
	{
		RenderableBase* renderable;
		uint32_t first;
		uint32_t count;
	};
	struct FlatBatch
	{
		uint32_t batchID;
		uint32_t objectID;
	};

	struct ObjectData
	{
		Eigen::Matrix4f model;
		Eigen::Vector4f spherebound;
		bool cullFlag;
	};

	struct DrawCullData
	{
		Eigen::Matrix4f view;
		float fov;
		float aspect;
		float znear, zfar;

		uint32_t drawCount;
	};
}
