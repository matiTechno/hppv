// in the implementation file:

// #include "glad.h" or "glew.h" or ...
// #define SHADER_IMPLEMENTATION
// #define SHADER_GLM (for more convenient uniform functions)
// #include "Shader.h"

// shader source format (order of the shader type directives does not matter)

// #vertex
// ...
// #geometry (optional)
// ...
// #fragment
// ...

// or

// #compute
// ...

// or
// #include "vertex.glsl"
// #fragment
// ...

// #include only works when a shader is loaded from a file,
// might be placed anywhere

// code is exception free

#pragma once

#include <string>
#include <set>
#include <map>
#include <initializer_list>
#include <experimental/filesystem>
#include <string_view>
#include <cassert>

#ifdef SHADER_GLM
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#endif

using GLint = int;
using GLuint = unsigned int;

namespace hppv
{

namespace fs = std::experimental::filesystem;

class Shader
{
public:
    struct File {};

    Shader() = default;
    Shader(File, const std::string& filename, bool hotReload = false);
    Shader(std::initializer_list<std::string_view> sources, std::string_view id);

    const std::string& getId() const {return id_;}

    bool isValid() const {return program_.getId();}

    GLint getUniformLocation(std::string_view name);

    // after successful reload:
    // * shader must be rebound
    // * all uniform locations are invalidated
    //
    // on failure:
    // * previous state remains

    void reload(); // does not check if file was modified

    void bind(); // if hotReload is on and file was modified, reloads the shader

    // call bind before these

    void uniform1i(std::string_view name, int value);
    void uniform1f(std::string_view name, float value);
    void uniform2f(std::string_view name, const float* value);
    void uniform3f(std::string_view name, const float* value);
    void uniform4f(std::string_view name, const float* value);
    void uniformMat4f(std::string_view name, const float* value);

#ifdef SHADER_GLM
    void uniform2f(std::string_view name, glm::vec2 value) {uniform2f(name, &value.x);}
    void uniform3f(std::string_view name, glm::vec3 value) {uniform3f(name, &value.x);}
    void uniform4f(std::string_view name, glm::vec4 value) {uniform4f(name, &value.x);}
    void uniformMat4f(std::string_view name, const glm::mat4& value) {uniformMat4f(name, &value[0][0]);}
#endif

private:
    enum {MaxUniforms = 256, InactiveUniform = 666};

    class Program
    {
    public:
        Program() = default;
        Program(GLuint id): id_(id) {}
        ~Program() {clean();}
        Program(const Program&) = delete;
        Program& operator=(const Program&) = delete;
        Program(Program&& rhs) {*this = std::move(rhs);}

        Program& operator=(Program&& rhs)
        {
            assert(this != &rhs);
            clean();
            id_ = rhs.id_;
            rhs.id_ = 0;
            return *this;
        }

        GLuint getId() const {return id_;}

    private:
        GLuint id_ = 0;

        void clean();
    };

    std::string id_;
    bool hotReload_ = false;
    Program program_;
    fs::file_time_type fileLastWriteTime_;
    std::map<std::string, GLint, std::less<>> uniformLocations_;
    std::set<std::string, std::less<>> inactiveUniforms_;

    // returns true on success
    bool swapProgram(std::initializer_list<std::string_view> sources);
};

} // namespace hppv

#ifdef SHADER_IMPLEMENTATION

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <optional>

namespace hppv
{

fs::file_time_type getFileLastWriteTime(const std::string& filename)
{
    std::error_code ec;
    const auto time = fs::last_write_time(filename, ec);

    if(ec)
    {
        std::cout << "Shader: last_write_time() failed, file = " << filename << std::endl;
    }

    return time;
}

void printMissingQuoteError(const fs::path& path)
{
    std::cout << "Shader: include directive - \" missing, file = " << path << std::endl;
}

std::string loadSourceFromFile(const fs::path& path)
{
    std::ifstream file(path);

    if(!file.is_open())
    {
        std::cout << "Shader: could not open file = " << path << std::endl;
        return {};
    }

    std::stringstream stringstream;
    stringstream << file.rdbuf();
    auto source = stringstream.str();

    const std::string_view includeDirective = "#include";

    std::size_t includeStart = 0;
    while((includeStart = source.find(includeDirective, includeStart)) != std::string::npos)
    {
        const auto quoteStart = source.find('"', includeStart + includeDirective.size());

        if(quoteStart == std::string::npos)
        {
            printMissingQuoteError(path);
            return {};
        }

        const auto pathStart = quoteStart + 1;
        const auto pathEnd = source.find('"', pathStart);

        if(pathEnd == std::string::npos)
        {
            printMissingQuoteError(path);
            return {};
        }

        const fs::path includePath(source.substr(pathStart, pathEnd - pathStart));
        std::string includedStr;

        if(includePath.is_absolute())
        {
            includedStr = loadSourceFromFile(includePath);
        }
        else
        {
            includedStr = loadSourceFromFile(path.parent_path() /= includePath);
        }

        const auto addSize = includedStr.size();
        source.erase(includeStart, pathEnd + 1 - includeStart);
        source.insert(includeStart, std::move(includedStr));
        includeStart += addSize;
    }

    return source;
}

Shader::Shader(File, const std::string& filename, const bool hotReload):
    id_(filename),
    hotReload_(hotReload)
{
    fileLastWriteTime_ = getFileLastWriteTime(filename);

    if(const auto source = loadSourceFromFile(filename); source.size())
    {
        swapProgram({source});
    }
}

Shader::Shader(const std::initializer_list<std::string_view> sources, const std::string_view id):
    id_(id)
{
    swapProgram(sources);
}

GLint Shader::getUniformLocation(const std::string_view name)
{
    const auto it = uniformLocations_.find(name);

    if(it == uniformLocations_.end() &&
       inactiveUniforms_.find(name) == inactiveUniforms_.end())
    {
        std::cout << "Shader, " << id_ << ": inactive uniform = " << name << std::endl;
        inactiveUniforms_.emplace(name);
        return InactiveUniform;
    }

    return it->second;
}

void Shader::reload()
{
    if(const auto time = getFileLastWriteTime(id_); time > fileLastWriteTime_)
    {
        fileLastWriteTime_ = time;
    }

    if(const auto source = loadSourceFromFile(id_); source.size())
    {
        if(swapProgram({source}))
        {
            std::cout << "Shader, " << id_ << ": reload succeeded" << std::endl;
        }
    }
}

void Shader::bind()
{
    if(hotReload_)
    {
        if(const auto time = getFileLastWriteTime(id_); time > fileLastWriteTime_)
        {
            fileLastWriteTime_ = time;

            if(const auto source = loadSourceFromFile(id_); source.size())
            {
                if(swapProgram({source}))
                {
                    std::cout << "Shader, " << id_ << ": hot reload succeeded" << std::endl;
                }
            }
        }
    }

    glUseProgram(program_.getId());
}

void Shader::uniform1i(const std::string_view name, const int value) {glUniform1i(getUniformLocation(name), value);}
void Shader::uniform1f(const std::string_view name, const float value) {glUniform1f(getUniformLocation(name), value);}
void Shader::uniform2f(const std::string_view name, const float* const value) {glUniform2fv(getUniformLocation(name), 1, value);}
void Shader::uniform3f(const std::string_view name, const float* const value) {glUniform3fv(getUniformLocation(name), 1, value);}
void Shader::uniform4f(const std::string_view name, const float* const value) {glUniform4fv(getUniformLocation(name), 1, value);}
void Shader::uniformMat4f(const std::string_view name, const float* const value) {glUniformMatrix4fv(getUniformLocation(name),
                                                                                                     1, GL_FALSE, value);}

void Shader::Program::clean() {if(id_) glDeleteProgram(id_);}

template<bool isProgram>
std::optional<std::string> getError(const GLuint id, const GLenum flag)
{
    GLint success;

    if constexpr (isProgram)
    {
        glGetProgramiv(id, flag, &success);
    }
    else
    {
        glGetShaderiv(id, flag, &success);
    }

    if(success == GL_TRUE)
        return {};

    GLint length;

    if constexpr (isProgram)
    {
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);
    }
    else
    {
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
    }

    if(!length)
        return "";

    std::string log(length, '\0');

    if constexpr (isProgram)
    {
        glGetProgramInfoLog(id, length, nullptr, log.data());
    }
    else
    {
        glGetShaderInfoLog(id, length, nullptr, log.data());
    }

    log.pop_back(); // pop the '\0'
    return log;
}

// shader must be deleted with glDeleteShader()
GLuint createAndCompileShader(const GLenum type, const std::string_view source)
{
    const auto id = glCreateShader(type);
    const auto* const str = source.data();
    const GLint length = source.length();
    glShaderSource(id, 1, &str, &length);
    glCompileShader(id);
    return id;
}

// returns 0 on error
// if return value != 0 program must be deleted with glDeleteProgram()
GLuint createProgram(const std::initializer_list<std::string_view> sources, const std::string_view id)
{
    struct ShaderType
    {
        GLenum value;
        std::string_view name;
    };

    const ShaderType shaderTypes[] =
    {{GL_VERTEX_SHADER, "#vertex"},
    {GL_GEOMETRY_SHADER, "#geometry"},
    {GL_FRAGMENT_SHADER, "#fragment"},
    {GL_COMPUTE_SHADER, "#compute"}};

    struct ShaderData
    {
        std::size_t start;
        ShaderType type;
    };

    std::vector<GLuint> shaders;
    bool compilationError = false;

    std::vector<ShaderData> shaderData;

    for(const auto source: sources)
    {
        shaderData.clear();

        for(const auto shaderType: shaderTypes)
        {
            if(const auto pos = source.find(shaderType.name); pos != std::string::npos)
            {
                shaderData.push_back({pos + shaderType.name.size(), shaderType});
            }
        }

        std::sort(shaderData.begin(), shaderData.end(), [](const ShaderData& l, const ShaderData& r)
                                                        {return l.start < r.start;});

        for(auto it = shaderData.cbegin(); it != shaderData.cend(); ++it)
        {
            std::size_t count;

            if(it == shaderData.end() - 1)
            {
                count = std::string::npos;
            }
            else
            {
                const auto nextIt = it + 1;
                count = nextIt->start - nextIt->type.name.size() - it->start;
            }

            const auto shaderSource = source.substr(it->start, count);
            shaders.push_back(createAndCompileShader(it->type.value, shaderSource));

            if(const auto error = getError<false>(shaders.back(), GL_COMPILE_STATUS))
            {
                std::cout << "Shader, " << id << ": " << it->type.name << " shader compilation failed\n"
                          << *error << '\n';

                {
                    std::cout.setf(std::ios::left);
                    std::size_t end = 0;
                    int line = 1;

                    for(;;)
                    {
                        const auto start = end;

                        if(end = shaderSource.find('\n', end); end != std::string::npos)
                        {
                            ++end;
                        }
                        else
                        {
                            std::cout << std::setw(5) << line << shaderSource.substr(start, end) << '\n';
                            break;
                        }

                        std::cout << std::setw(5) << line << shaderSource.substr(start, end - start);
                        ++line;
                    }

                    std::cout.flush();
                }

                compilationError = true;
            }
        }
    }

    if(compilationError)
    {
        for(const auto shader: shaders)
        {
            glDeleteShader(shader);
        }

        return 0;
    }

    const auto program = glCreateProgram();

    for(const auto shader: shaders)
    {
        glAttachShader(program, shader);
    }

    glLinkProgram(program);

    for(const auto shader: shaders)
    {
        glDetachShader(program, shader);
        glDeleteShader(shader);
    }

    if(const auto error = getError<true>(program, GL_LINK_STATUS))
    {
        std::cout << "Shader, " << id << ": program linking failed\n" << *error << std::endl;
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

bool Shader::swapProgram(const std::initializer_list<std::string_view> sources)
{
    const auto newProgram = createProgram(sources, id_);

    if(!newProgram)
        return false;

    program_ = Program(newProgram);

    uniformLocations_.clear();
    inactiveUniforms_.clear();

    GLint numUniforms;
    glGetProgramiv(program_.getId(), GL_ACTIVE_UNIFORMS, &numUniforms);

    std::vector<char> uniformName(MaxUniforms);

    for(auto i = 0; i < numUniforms; ++i)
    {
        GLint dum1;
        GLenum dum2;

        glGetActiveUniform(program_.getId(), i, uniformName.size(), nullptr, &dum1, &dum2,
                           uniformName.data());

        const auto uniformLocation = glGetUniformLocation(program_.getId(), uniformName.data());
        uniformLocations_[uniformName.data()] = uniformLocation;
    }

    return true;
}

} // namespace hppv

#endif // SHADER_IMPLEMENTATION
