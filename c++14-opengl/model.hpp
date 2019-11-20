#ifndef INCLUDED_MODEL_HPP
#define INCLUDED_MODEL_HPP

#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <glad/glad.h>

#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "texture.hpp"
#include "program.hpp"
#include "globject.hpp"
#include "defines.hpp"

namespace nekolib {
  namespace assimp {
    enum class TextureType { DIFFUSE, SPECULAR, NORMAL, REFLECTION };
    struct Vertex {
      glm::vec3 position;
      glm::vec3 normal;
      glm::vec2 tex_coords;
      glm::vec3 tangent;
      glm::vec3 bitangent;
    };

    struct Texture {
      nekolib::renderer::Texture texture;
      TextureType type;
      std::string path;
    };

    class Mesh {
    public:
      std::vector<Vertex> vertices_;
      std::vector<unsigned int> indices_;
      std::vector<Texture> textures_;

      Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
	: vertices_(vertices), indices_(indices), textures_(textures) {
	setup();
      }

      // render the mesh
      void render(nekolib::renderer::Program& program) const {
	// bind appropriate textures
	unsigned int diffuse_id(1);
	unsigned int specular_id(1);
	unsigned int normal_id(1);
	unsigned int reflection_id(1);

	for (size_t i = 0; i < textures_.size(); ++i) {
	  char buf[32];

	  // set proper sampler in shader program to texture unit i
	  if (textures_[i].type == TextureType::DIFFUSE) {
	    snprintf(buf, 32, "material.%s%d", "texture_diffuse", diffuse_id++);
	  } else if (textures_[i].type == TextureType::SPECULAR) {
	    snprintf(buf, 32, "material.%s%d", "texture_specular", specular_id++);
	  } else if (textures_[i].type == TextureType::NORMAL) {
	    snprintf(buf, 32, "material.%s%d", "texture_normal", normal_id++);
	  } else if (textures_[i].type == TextureType::REFLECTION) {
	    snprintf(buf, 32, "material.%s%d", "texture_reflection", reflection_id++);
	  } else {
	    assert(!"This must not happen to be!\n");
	    return;
	  }
	  program.set_uniform(buf, static_cast<int>(i));
	  // bind the texture to texture unit i
	  textures_[i].texture.bind(i);
	}
	// draw mesh
	vao_.bind();
	glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, 0);
	vao_.bind(false);

	// like manner
	glActiveTexture(GL_TEXTURE0);
      }
      
    private:
      nekolib::renderer::gl::Vao vao_;
      nekolib::renderer::gl::VertexBuffer vbo_;
      nekolib::renderer::gl::IndexBuffer ebo_;

      void setup() noexcept {
	vao_.bind();
	vbo_.bind();
	ebo_.bind();

	glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), &vertices_[0], GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int), &indices_[0], GL_STATIC_DRAW);

	// vertex position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(0));
	// vertex normal
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, normal)));
	// vertex texture coords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, tex_coords)));
	// vertex tangent
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, tangent)));
	// vertex bitangent
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, bitangent)));

	vao_.bind(false);
      }
    };

    class Model {
    public:
      std::vector<Mesh> meshes_;
      std::string dir_;
      using KV = std::pair<std::string, nekolib::renderer::Texture>;
      std::vector<KV> textures_cache_; // alyways sorted vector
                                       // record path & texture which has already loaded.
      bool gamma_; // gamma correction

      // constructor
      Model() noexcept : dir_(""), gamma_(false) {}

      // draw
      void render(nekolib::renderer::Program& prog) const {
	for (const auto& mesh : meshes_) {
	  mesh.render(prog);
	}
      }
      
      // loads a model with supported Assimp and stores meshes in vector
      bool load_model(const std::string& path) {
	fprintf(stderr, "model path: %s\n", path.c_str());
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);

	// check error
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
	  fprintf(stderr, "Assimp error : %s\n", importer.GetErrorString());
	  return false;
	}
	dir_ = path.substr(0, path.find_last_of('/'));

	// process Assimp's node from root node recusively
	return process_node(scene->mRootNode, scene);
      }

    private:
      bool process_node(aiNode* node, const aiScene* scene) {
	// process each mesh in current node
	for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
	  // node's mMeshes is array of index to scene's mMeshes
	  aiMesh* aimesh = scene->mMeshes[node->mMeshes[i]];
	  if (!process_mesh(aimesh, scene)) {
	    return false;
	  }
	}
	
	// recursive call for children
	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
	  if (!process_node(node->mChildren[i], scene)) {
	    return false;
	  }
	}

	return true;
      }

      bool process_mesh(aiMesh* mesh, const aiScene* scene) {
	// datas to fill
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;
	fprintf(stderr, "vertices: %d, faces: %d\n", mesh->mNumVertices, mesh->mNumFaces);
	
	// walk through each of mesh's vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
	  Vertex vertex;
	  vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
	  vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
	  if (mesh->mTextureCoords[0]) { // does this mesh contain texture coordinates?
	    vertex.tex_coords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
	  } else {
	    vertex.tex_coords = glm::vec2(0.f, 0.f);
	  }

	  if (mesh->mTangents) {
	  vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
	  vertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
	  } else {
	    vertex.tangent = vertex.bitangent = glm::vec3(0.f, 0.f, 0.f);
	  }
	  
	  vertices.push_back(vertex);
	}

	// walk through each of mesh's faces and make vertex indices
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
	  aiFace face = mesh->mFaces[i];
	  for (unsigned int j = 0; j < face.mNumIndices; ++j) {
	    indices.push_back(face.mIndices[j]);
	  }
	}

	// process Materials
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	// in our convention, sampler names in shader are
	// "texture_dissuseN", "texture_specularN", "texture_normalN", texture_reflectionN"

	// diffuse maps
	if (!append_material_textures(material, TextureType::DIFFUSE, textures)) {
	  return false;
	}
	// specular maps
	if (!append_material_textures(material, TextureType::SPECULAR, textures)) {
	  return false;
	}
	// normal maps
	if (!append_material_textures(material, TextureType::NORMAL, textures)) {
	  return false;
	}
	// reflection maps
	if (!append_material_textures(material, TextureType::REFLECTION, textures)) {
	  return false;
	}

	// add new mesh
	meshes_.emplace_back(vertices, indices, textures);

	return true;
      }

      bool append_material_textures(aiMaterial* mat, TextureType type,
				    std::vector<Texture>& textures) {
	aiTextureType type_table[] = { aiTextureType_DIFFUSE,
				       aiTextureType_SPECULAR,
				       // Hack for assimp probrem when loading wavefront .obj
				       aiTextureType_HEIGHT,
				       // Hack for assimp doesn't support reflection map
				       aiTextureType_AMBIENT, };
	aiTextureType aitype = type_table[static_cast<size_t>(type)];

	for (unsigned i = 0; i < mat->GetTextureCount(aitype); ++i) {
	  aiString aipath;
	  mat->GetTexture(aitype, i, &aipath);
	  std::string path(aipath.C_Str());
	  for (size_t i = 0; i < path.length(); ++i) {
	    if (path[i] == '\\') {
	      path[i] = '/';
	    }
	  }
	  path = dir_ + '/' + path;
	    
	  
	  // check same path texture has already loaded ?
	  auto it = lower_bound(textures_cache_.begin(), textures_cache_.end(), path,
				[](const KV& p, const std::string& path) {
				  return strcmp(p.first.c_str(), path.c_str()) < 0;
				});
	  if (it != textures_cache_.end() && strcmp((*it).first.c_str(), path.c_str()) == 0) {
	    // texture with this path has already loaded
	    Texture texture;
	    texture.texture = (*it).second;
	    texture.type = type; // different texturetype may use same file...?
	    texture.path = path;
	    textures.push_back(texture);
	  } else {
	    // texture with this path has not loaded
	    Texture texture;
	    fprintf(stderr, "loading texture from %s\n", path.c_str());
	    texture.texture = nekolib::renderer::Texture::create(path.c_str(), false);
	    if (!texture.texture) {
	      fprintf(stderr, "loading texture from %s, falied\n", path.c_str());
	      return false;
	    }
	    texture.type = type;
	    texture.path = path;
	    textures.push_back(texture);
	    
	    textures_cache_.insert(it, make_pair(texture.path, texture.texture));
	  }
	}

	return true;
      }
    };
  }
}

#endif // INCLUDED_MODEL_HPP
