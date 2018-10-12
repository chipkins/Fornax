#include "VulkanModel.h"

#include <iostream>
#include <algorithm>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

const std::string MODEL_PATH = "../source/assets/models/chalet.obj";
const std::string TEXTURE_PATH = "../source/assets/textures/chalet.jpg";

namespace vk
{
	void Model::LoadModel(const char* modelFile)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelFile))
		{
			throw std::runtime_error(err);
		}

		std::map<int, Vertex> uniqueVertices = {};

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex = {};
				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};
				vertex.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
				vertex.color = {1.0f, 1.0f, 1.0f};
				
				if (uniqueVertices.count(index.vertex_index) == 0)
				{
					uniqueVertices[index.vertex_index] = vertex;
				}

				indices.push_back(index.vertex_index);
			}
		}
		for (int i = 0; i < uniqueVertices.size(); ++i)
		{
			vertices.push_back(uniqueVertices[i]);
		}
	}
}