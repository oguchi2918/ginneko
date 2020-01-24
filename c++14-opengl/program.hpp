#ifndef INCLUDED_PROGRAM_HPP
#define INCLUDED_PROGRAM_HPP

#include <string>
#include <vector>
#include <utility>

#include <glm/glm.hpp>

namespace nekolib {
  namespace renderer {
    enum class ShaderType { VERTEX = GL_VERTEX_SHADER,
			    FRAGMENT = GL_FRAGMENT_SHADER,
			    GEOMETRY = GL_GEOMETRY_SHADER,
			    TESS_CONTROL = GL_TESS_CONTROL_SHADER,
			    TESS_EVALUATION = GL_TESS_EVALUATION_SHADER,
			    COMPUTE = GL_COMPUTE_SHADER, };

    class Program {
    private:
      int handle_;
      bool linked_;
      std::string log_string_;

      // ユニフォーム変数名とlocの対応表
      // cache効果目当てというよりは
      // 「このユニフォーム変数は存在しない」エラーを
      // 最初の一回だけ表示する為に使用している
      std::vector<std::pair<char*, int>> uniforms_;

      int get_uniform_location(const char* name);
      bool file_exists(const char* filename);

    public:
      Program() : handle_(0), linked_(false), log_string_(""), uniforms_() { uniforms_.reserve(16); }
      ~Program();

      // 複数のファイルからProgramを自動生成
      // compile_shader_from_file() -> link() -> valid()の順で実行される
      // ShaderTypeは以下のようにファイル名の拡張子から自動判別
      // "*.vs" or "*.vert" -> VERTEX
      // "*.fs" or "*.frag" -> FRAGMENT
      // "*.gs" or "*.geom" -> GEOMETRY
      // "*.cs" or "*.comp" -> COMPUTE
      bool build_program_from_files(std::vector<std::string> filenames);

      bool compile_shader_from_file(const char* filename, ShaderType type);
      bool compile_shader_from_string(const std::string& source, ShaderType type);
      bool link();
      void use();
      bool valid();

      std::string log() const noexcept { return log_string_; }
      int handle() const noexcept { return handle_; }
      bool is_linked() const noexcept { return linked_; }

      // シェーダの方でlocation指定を使うなら以下の二関数はあまり使わない
      void bind_attrib_location(GLuint location, const char* name);
      void bind_frag_data_location(GLuint location, const char* name);
    
      void set_uniform(const char* name, float x, float y, float z);
      void set_uniform(const char* name, const glm::vec2& v);
      void set_uniform(const char* name, const glm::vec3& v);
      void set_uniform(const char* name, const glm::vec4& v);
      void set_uniform(const char* name, const glm::mat4& m);
      void set_uniform(const char* name, const glm::mat3& m);
      void set_uniform(const char* name, float val);
      void set_uniform(const char* name, int val);
      void set_uniform(const char* name, unsigned int val);
      void set_uniform(const char* name, bool val);
      void set_uniform(const char* name, const float* a, size_t count);

      void set_subroutines(std::vector<const char*> names, ShaderType type);

      void print_active_uniforms();
      void print_active_attribs();
    };
  }
}

#endif // INCLUDED_PROGRAM_HPP
