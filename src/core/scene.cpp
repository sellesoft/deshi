#define TINYOBJLOADER_IMPLEMENTATION
#include "../external/tinyobjloader/tiny_obj_loader.h"

void Scene::
Init(){
	g_scene = this;
	CreateBox(1.0f, 1.0f, 1.0f);
	CreateMeshFromOBJ("sphere.obj");
	CreateMeshFromOBJ("arrow.obj");
}

void Scene::
Reset(){
	for(int i=3; i < meshes.size(); ++i) DeleteMesh(&meshes[i]); //NOTE(delle) this is dumb
}

//TODO(delle,Op) speed this up with tinyobj::LoadOBJWithCallback to not parse twice
Mesh* Scene::
CreateMeshFromOBJ(const char* filename, Shader shader, Color color, bool planarize){
	//setup tinyobj and parse the OBJ file
	tinyobj::ObjReaderConfig reader_config;
	reader_config.triangulate     = true;
	reader_config.vertex_color    = true;
	reader_config.mtl_search_path = Assets::dirModels(); // Path to material files
	tinyobj::ObjReader reader;
	if(!reader.ParseFromFile(Assets::dirModels() + filename, reader_config)) {
		ERROR("Failed to read OBJ file: ", filename);
		if (!reader.Error().empty()) ERROR("TinyObjReader: ", reader.Error());
		return 0;
	}
	if(!reader.Warning().empty()) WARNING("TinyObjReader: " , reader.Warning());
	
	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();
	Assert(shapes[0].mesh.num_face_vertices[0] == 3, "OBJ must be triangulated");
	
	//check which features it has
	bool hasMaterials = materials.size() > 0;
	bool hasNormals = attrib.normals.size() > 0;
	bool hasUVs = attrib.texcoords.size() > 0;
	bool hasColors = attrib.colors.size() > 0;
	
	//create mesh header on scene
	meshes.push_back(Mesh{});
	
	std::unordered_map<Vertex, u32> uniqueVertexes{};
	
	//if planarize, change all vertex normals to equal their face normal
	if(planarize){
		
	}
	
	//return mesh pointer
	return &meshes[meshes.size()-1];
}

Mesh* Scene::
CreateBox(f32 width, f32 height, f32 depth, Shader shader, Color color, bool planarize){
	//@Incomplete
}

void Scene::
DeleteMesh(Mesh* mesh){
	forI(meshes.size()){
		if(mesh == &meshes[i]){
			//@Incomplete
		}
	}
}