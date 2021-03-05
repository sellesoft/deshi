#pragma once
#include "../utils/defines.h"
#include "../animation/Model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../external/tinyobjloader/tiny_obj_loader.h"

#include <vector>
#include <iostream>
#include <fstream>

namespace deshi{
	//TODO(,delle) check which dir we are in to get the correct path
	inline static std::string getDataPath(){
		return "data/";
	}
	
	inline static std::string getModelsPath(){
		return getDataPath() + "models/";
	}
	
	inline static std::string getTexturesPath(){
		return getDataPath() + "textures/";
	}
	
	inline static std::string getSoundsPath(){
		return getDataPath() + "sounds/";
	}
	
	inline static std::string getShadersPath(){
		return "shaders/";
	}
	
	//TODO(,delle) fix error when not finding file
	//reads a files contents in binary and returns it as a char vector
	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		
		if (!file.is_open()) {
			ASSERT(false, "failed to open file");
		}
		
		size_t fileSize = (size_t) file.tellg();
		//std::cout << filename << ": " << fileSize << std::endl;
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();
		
		return buffer;
	}
	
	//TODO(r,delle) finish converting meshes from tinyOBJ
	//filename: filename and extension
	static Mesh CreateMeshFromOBJ(std::string filename){
		tinyobj::ObjReaderConfig reader_config;
		reader_config.mtl_search_path = "./"; // Path to material files
		
		tinyobj::ObjReader reader;
		if (!reader.ParseFromFile(getModelsPath() + filename, reader_config)) {
			if (!reader.Error().empty()) {
				std::cerr << "TinyObjReader: " << reader.Error();
			}
			ASSERT(false, "failed to read OBJ file");
		}
		
		if (!reader.Warning().empty()) {
			std::cout << "TinyObjReader: " << reader.Warning();
		}
		
		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		auto& materials = reader.GetMaterials();
		
		// Loop over shapes
		for (size_t s = 0; s < shapes.size(); s++) {
			ASSERT(shapes[s].mesh.num_face_vertices.size() == 3, "OBJ must be triangulated before importing");
			// Loop over faces(polygon)
			size_t index_offset = 0;
			for (size_t f = 0; f < 3; f++) {
				int fv = shapes[s].mesh.num_face_vertices[f];
				
				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					tinyobj::real_t vx = attrib.vertices[3*idx.vertex_index+0];
					tinyobj::real_t vy = attrib.vertices[3*idx.vertex_index+1];
					tinyobj::real_t vz = attrib.vertices[3*idx.vertex_index+2];
					tinyobj::real_t nx = attrib.normals[3*idx.normal_index+0];
					tinyobj::real_t ny = attrib.normals[3*idx.normal_index+1];
					tinyobj::real_t nz = attrib.normals[3*idx.normal_index+2];
					tinyobj::real_t tx = attrib.texcoords[2*idx.texcoord_index+0];
					tinyobj::real_t ty = attrib.texcoords[2*idx.texcoord_index+1];
					// Optional: vertex colors
					// tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
					// tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
					// tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];
				}
				index_offset += fv;
				
				// per-face material
				shapes[s].mesh.material_ids[f];
			}
		}
	}
}