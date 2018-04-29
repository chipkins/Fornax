#include "../PrecompiledHeader.h"

struct Vertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 color;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, uv);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}

	bool operator ==(const Vertex& other) const
	{
		return pos == other.pos && uv == other.uv && color == other.color;
	}
};

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
					(hash<glm::vec2>()(vertex.uv) << 1)) >> 1) ^
					(hash<glm::vec3>()(vertex.color) << 1);
		}
	};
}

const std::string MODEL_PATH = "../source/assets/models/chalet.obj";
const std::string TEXTURE_PATH = "../source/assets/textures/chalet.jpg";

class Model
{
public:
	void LoadModel(const char* modelFile);

	Vertex*   getVertices() { return vertices.data(); }
	size_t    getNumVertices() { return vertices.size(); }
	uint32_t* getIndices() { return indices.data(); }
	size_t    getNumIndices() { return indices.size(); }

private:
	std::vector<Vertex>   vertices;
	std::vector<uint32_t> indices;
};