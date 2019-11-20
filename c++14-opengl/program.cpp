#include <cassert>
#include <vector>
#include <string>
#include <algorithm>
#include <utility>
#include <istream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include <glad/glad.h>

#include "program.hpp"

namespace nekolib {
  namespace renderer {
    using KeyValue = std::pair<char*, int>;

    Program::~Program() noexcept
    {
      glUseProgram(0);
      if (handle_ > 0) {
	glDeleteProgram(handle_);
      }

      // free uniform names
      for (auto& kv : uniforms_) {
	free(kv.first);
      }
    }
  
    bool Program::compile_shader_from_file(const char* filename, ShaderType type)
    {
      if (!file_exists(filename)) {
	log_string_ = "File not found.";
	return false;
      }

      if (handle_ <= 0) {
	handle_ = glCreateProgram();
	if (handle_ == 0) {
	  log_string_ = "Unable to create shader program.";
	  return false;
	}
      }

      std::ifstream in_file(filename);
      if (!in_file) {
	return false;
      }
      std::istreambuf_iterator<char> begin(in_file);
      std::istreambuf_iterator<char> end;
      std::string code(begin, end);
    
      return compile_shader_from_string(code, type);
    }

    bool Program::compile_shader_from_string(const std::string& source, ShaderType type)
    {
      if (handle_ <= 0) {
	handle_ = glCreateProgram();
	if (handle_ == 0) {
	  log_string_ = "Unable to create shader program.";
	  return false;
	}
      }

      GLuint shader_handle = 0;

      switch(type) {
      case ShaderType::VERTEX:
	shader_handle = glCreateShader(GL_VERTEX_SHADER);
	break;
      case ShaderType::FRAGMENT:
	shader_handle = glCreateShader(GL_FRAGMENT_SHADER);
	break;
      case ShaderType::GEOMETRY:
	shader_handle = glCreateShader(GL_GEOMETRY_SHADER);
	break;
      case ShaderType::TESS_CONTROL:
	shader_handle = glCreateShader(GL_TESS_CONTROL_SHADER);
	break;
      case ShaderType::TESS_EVALUATION:
	shader_handle = glCreateShader(GL_TESS_EVALUATION_SHADER);
	break;
      case ShaderType::COMPUTE:
	shader_handle = glCreateShader(GL_COMPUTE_SHADER);
	break;
      default:
	assert(!"This must not be happend!");
	break;
      }

      const char* c_code = source.c_str();
      glShaderSource(shader_handle, 1, &c_code, nullptr);

      glCompileShader(shader_handle);
    
      int result;
      glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &result);
      if (result == GL_FALSE) {
	// compile fail
	int length = 0;
	log_string_ = "";
	glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &length);
	if (length > 0) {
	  std::vector<GLchar> info_log(length + 1);
	  glGetShaderInfoLog(shader_handle, length, nullptr, &info_log[0]);
	  log_string_ = std::string(&info_log[0]);
	}

	return false;
      } else {
	// compile success
	glAttachShader(handle_, shader_handle);
	glDeleteShader(shader_handle);
	return true;
      }
    }

    bool Program::link()
    {
      if (linked_) {
	return true;
      }
      if (handle_ <= 0) {
	return false;
      }

      glLinkProgram(handle_);

      int status = 0;
      glGetProgramiv(handle_, GL_LINK_STATUS, &status);
      if (status == GL_FALSE) {
	int length = 0;
	log_string_ = "";
	glGetProgramiv(handle_, GL_INFO_LOG_LENGTH, &length);

	if (length > 0) {
	  std::vector<GLchar> c_log;
	  glGetProgramInfoLog(handle_, length, nullptr, &c_log[0]);
	  log_string_ = std::string(&c_log[0]);
	}

	return false;
      } else {
	linked_ = true;
	return true;
      }
    }

    void Program::use()
    {
      if (handle_ <= 0 || !linked_) {
	return;
      }
      glUseProgram(handle_);
    }

    bool Program::valid()
    {
      if (!is_linked()) {
	return false;
      }

      GLint status;
      glValidateProgram(handle_);
      glGetProgramiv(handle_, GL_VALIDATE_STATUS, &status);

      if (status == GL_FALSE) {
	int length = 0;
	log_string_ = "";

	glGetProgramiv(handle_, GL_INFO_LOG_LENGTH, &length);
	if (length > 0) {
	  std::vector<GLchar> log(length + 1);
	  glGetProgramInfoLog(handle_, length, nullptr, &log[0]);
	  log_string_ = std::string(&log[0]);
	}

	return false;
      } else {
	return true;
      }
    }

    void Program::bind_attrib_location(GLuint location, const char* name)
    {
      glBindAttribLocation(handle_, location, name);
    }

    void Program::bind_frag_data_location(GLuint location, const char* name)
    {
      glBindFragDataLocation(handle_, location, name);
    }

    void Program::set_uniform(const char* name, float x, float y, float z)
    {
      int loc = get_uniform_location(name);
      if (loc >= 0) {
	glUniform3f(loc, x, y, z);
      }
    }

    void Program::set_uniform(const char* name, const glm::vec2& v)
    {
      int loc = get_uniform_location(name);
      if (loc >= 0) {
	glUniform2f(loc, v.x, v.y);
      }
    }

    void Program::set_uniform(const char* name, const glm::vec3& v)
    {
      set_uniform(name, v.x, v.y, v.z);
    }

    void Program::set_uniform(const char* name, const glm::vec4& v)
    {
      int loc = get_uniform_location(name);
      if (loc >= 0) {
	glUniform4f(loc, v.x, v.y, v.z, v.w);
      }
    }

    void Program::set_uniform(const char* name, const glm::mat3& m)
    {
      int loc = get_uniform_location(name);
      if (loc >= 0) {
	glUniformMatrix3fv(loc, 1, GL_FALSE, &m[0][0]);
      }
    }

    void Program::set_uniform(const char* name, const glm::mat4& m)
    {
      int loc = get_uniform_location(name);
      if (loc >= 0) {
	glUniformMatrix4fv(loc, 1, GL_FALSE, &m[0][0]);
      }
    }

    void Program::set_uniform(const char* name, const float* a, size_t count)
    {
      int loc = get_uniform_location(name);
      if (loc >= 0) {
	glUniform1fv(loc, count, a);
      }
    }

    void Program::set_uniform(const char* name, float val)
    {
      int loc = get_uniform_location(name);
      if (loc >= 0) {
	glUniform1f(loc, val);
      }
    }

    void Program::set_uniform(const char* name, int val)
    {
      int loc = get_uniform_location(name);
      if (loc >= 0) {
	glUniform1i(loc, val);
      }
    }

    void Program::set_uniform(const char* name, unsigned int val)
    {
      int loc = get_uniform_location(name);
      if (loc >= 0) {
	glUniform1ui(loc, val);
      }
    }
    void Program::set_uniform(const char* name, bool val)
    {
      int loc = get_uniform_location(name);
      if (loc >= 0) {
	glUniform1i(loc, val);
      }
    }

    void Program::set_subroutines(std::vector<const char*>names, ShaderType shader)
    {
      std::vector<GLuint> locs;
      for (auto it(names.begin()); it != names.end(); ++it) {
	GLuint loc = glGetSubroutineIndex(handle_, static_cast<GLenum>(shader), *it);
	locs.push_back(loc);
      }
      glUniformSubroutinesuiv(static_cast<GLenum>(shader), locs.size(), &locs[0]);
    }

    void Program::print_active_uniforms()
    {
      GLint uniforms_num, size, location, max_len;
      GLsizei written;
      GLenum type;

      glGetProgramiv(handle_, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_len);
      glGetProgramiv(handle_, GL_ACTIVE_UNIFORMS, &uniforms_num);
      std::vector<GLchar> name(max_len + 1);
    
      printf(" Location | Name\n");
      printf("--------------------------------------------------\n");
      for (int i = 0; i < uniforms_num; ++i) {
	glGetActiveUniform(handle_, i, max_len, &written, &size, &type, &name[0]);
	location = glGetUniformLocation(handle_, &name[0]);
	printf(" %-8d | %s\n", location, &name[0]);
      }
    }

    void Program::print_active_attribs()
    {
      GLint attribs_num, size, location, max_len;
      GLsizei written;
      GLenum type;

      glGetProgramiv(handle_, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_len);
      glGetProgramiv(handle_, GL_ACTIVE_ATTRIBUTES, &attribs_num);
      std::vector<GLchar> name(max_len + 1);

      printf(" Index | Name\n");
      printf("------------------------------------------------\n");
      for (int i = 0; i < attribs_num; ++i) {
	glGetActiveAttrib(handle_, i, max_len, &written, &size, &type, &name[0]);
	location = glGetAttribLocation(handle_, &name[0]);
	printf(" %-5d | %s\n", location, &name[0]);
      }
    }

    int Program::get_uniform_location(const char* name)
    {
      // search uniform name
      auto iter = lower_bound(uniforms_.begin(), uniforms_.end(), name,
			      [](const KeyValue& p, const char* s) {
				return strcmp(p.first, s) < 0;
			      });
      if (iter == uniforms_.end() || strcmp((*iter).first, name) != 0) { // not found
	char* key = strdup(name);
	int loc = glGetUniformLocation(handle_, name);
	uniforms_.insert(iter, std::make_pair(key, loc));

	if (loc < 0) { // uniform variable doesn't exist
	  fprintf(stderr, "uniform %s can't found\n", name);
	}
	return loc;
      } else { // found
	return (*iter).second;
      }
    }

    bool Program::file_exists(const char* filename)
    {
      struct stat info;
      int ret = -1;

      ret = stat(filename, &info);
      return ret == 0;
    }
  }
}
