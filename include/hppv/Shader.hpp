// in the implementation file:

// #include "glad.h" or "glew.h" or ...
// #define SHADER_IMPLEMENTATION
// #include "Shader.h"

// shader source format

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

// directives might be placed anywhere
// #include only works when loading from file

// code is exception free

#pragma once

#include <string>
#include <set>
#include <map>
#include <initializer_list>
#include <experimental/filesystem>
#include <string_view>
#include <cassert>

namespace hppv
{

namespace fs = std::experimental::filesystem;

struct File {};

class Shader
{
public:
    using GLint = int;
    using GLuint = unsigned int;

    Shader(File, const std::string& filename, bool hotReload = false);
    Shader(std::initializer_list<std::string_view> sources, std::string_view id);
    Shader() = default;

    bool isValid() const {return program_.getId();}

    GLint getUniformLocation(const std::string& uniformName, bool printError = true) const;

    // after successful reload:
    // * shader must be rebound
    // * all uniform locations are invalidated
    //
    // on failure:
    // * previous state remains

    void reload(); // does not check if file was modified

    void bind(); // if hotReload is on and file was modified does reload

private:
    class Program
    {
    public:
        Program(): id_(0) {}
        Program(GLuint id): id_(id) {}
        ~Program();
        Program(const Program&) = delete;
        Program& operator=(const Program&) = delete;
        Program(Program&& rhs): id_(rhs.id_) {rhs.id_ = 0;}

        Program& operator=(Program&& rhs)
        {
            assert(this != &rhs);
            this->~Program();
            id_ = rhs.id_;
            rhs.id_ = 0;
            return *this;
        }

        GLuint getId() const {return id_;}

    private:
        GLuint id_;
    };

    std::string id_;
    bool hotReload_;
    Program program_;
    fs::file_time_type fileLastWriteTime_;
    std::map<std::string, GLint> uniformLocations_;
    mutable std::set<std::string> inactiveUniforms_;

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
    auto time = fs::last_write_time(filename, ec);

    if(ec)
    {
        std::cout << "Shader: last_write_time() failed, file = " << filename << std::endl;
    }

    return time;
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

    std::size_t start = 0;
    while((start = source.find(includeDirective, start)) != std::string::npos)
    {
        auto startQuot = source.find('"', start + includeDirective.size());
        assert(startQuot != std::string::npos);
        auto startPath = startQuot + 1;
        auto endPath = source.find('"', startPath);
        assert(endPath != std::string::npos);
        fs::path includePath(source.substr(startPath, endPath - startPath));
        std::string includedStr;

        if(includePath.is_absolute())
        {
            includedStr = loadSourceFromFile(includePath);
        }
        else
        {
            includedStr = loadSourceFromFile(path.parent_path() /= includePath);
        }

        auto addSize = includedStr.size();
        source.erase(start, endPath + 1 - start);
        source.insert(start, std::move(includedStr));
        start += addSize;
    }

    return source;
}

Shader::Shader(File, const std::string& filename, bool hotReload):
    id_(filename),
    hotReload_(hotReload)
{
    fileLastWriteTime_ = getFileLastWriteTime(filename);

    if(auto source = loadSourceFromFile(filename); source.size())
    {
        swapProgram({source});
    }
}

Shader::Shader(std::initializer_list<std::string_view> sources, std::string_view id):
    id_(id),
    hotReload_(false)
{
    swapProgram(sources);
}

GLint Shader::getUniformLocation(const std::string& uniformName, bool printError) const
{
    auto it = uniformLocations_.find(uniformName);

    if(it == uniformLocations_.end() &&
       inactiveUniforms_.find(uniformName) == inactiveUniforms_.end())
    {
        if(printError)
        {
            std::cout << "Shader, " << id_ << ": inactive uniform = " << uniformName << std::endl;
            inactiveUniforms_.insert(uniformName);
        }
        return 666;
    }

    return it->second;
}

void Shader::reload()
{
    if(auto time = getFileLastWriteTime(id_); time > fileLastWriteTime_)
    {
        fileLastWriteTime_ = time;
    }

    if(auto source = loadSourceFromFile(id_); source.size())
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
        if(auto time = getFileLastWriteTime(id_); time > fileLastWriteTime_)
        {
            fileLastWriteTime_ = time;

            if(auto source = loadSourceFromFile(id_); source.size())
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

Shader::Program::~Program() {if(id_) glDeleteProgram(id_);}

template<bool isProgram>
std::optional<std::string> getError(GLuint id, GLenum flag)
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

    --length;
    std::string log(length, '\0');

    if constexpr (isProgram)
    {
        glGetProgramInfoLog(id, length, nullptr, log.data());
    }
    else
    {
        glGetShaderInfoLog(id, length, nullptr, log.data());
    }

    return log;
}

// shader must be deleted by caller with glDeleteShader()
GLuint createAndCompileShader(GLenum type, const std::string& source)
{
    auto id = glCreateShader(type);
    auto* str = source.c_str();
    glShaderSource(id, 1, &str, nullptr);
    glCompileShader(id);
    return id;
}

// returns 0 on error
// if return value != 0 program must be deleted by caller with glDeleteProgram()
GLuint createProgram(std::initializer_list<std::string_view> sources, const std::string& id)
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
    for(auto source: sources)
    {
        shaderData.clear();

        for(auto& shaderType: shaderTypes)
        {
            if(auto pos = source.find(shaderType.name); pos != std::string::npos)
            {
                shaderData.push_back({pos + shaderType.name.size(), shaderType});
            }
        }

        std::sort(shaderData.begin(), shaderData.end(), [](ShaderData& l, ShaderData& r)
                                                        {return l.start < r.start;});

        for(auto it = shaderData.begin(); it != shaderData.end(); ++it)
        {
            std::size_t count;

            if(it == shaderData.end() - 1)
            {
                count = std::string::npos;
            }
            else
            {
                auto nextIt = it + 1;
                count = nextIt->start - nextIt->type.name.size() - it->start;
            }

            auto shaderSource = source.substr(it->start, count);
            shaders.push_back(createAndCompileShader(it->type.value, std::string(shaderSource)));

            if(auto error = getError<false>(shaders.back(), GL_COMPILE_STATUS))
            {
                std::cout << "Shader, " << id << ": " << it->type.name << " shader compilation failed\n"
                          << *error << '\n';

                {
                    std::cout.setf(std::ios::left);
                    std::size_t end = 0;
                    int line = 1;

                    for(;;)
                    {
                        auto start = end;

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
        for(auto shader: shaders)
        {
            glDeleteShader(shader);
        }

        return 0;
    }

    auto program = glCreateProgram();

    for(auto shader: shaders)
    {
        glAttachShader(program, shader);
    }

    glLinkProgram(program);

    for(auto shader: shaders)
    {
        glDetachShader(program, shader);
        glDeleteShader(shader);
    }

    if(auto error = getError<true>(program, GL_LINK_STATUS))
    {
        std::cout << "Shader, " << id << ": program linking failed\n" << *error << std::endl;
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

bool Shader::swapProgram(std::initializer_list<std::string_view> sources)
{
    auto newProgram = createProgram(sources, id_);

    if(!newProgram)
        return false;

    program_ = Program(newProgram);

    uniformLocations_.clear();
    inactiveUniforms_.clear();

    GLint numUniforms;
    glGetProgramiv(program_.getId(), GL_ACTIVE_UNIFORMS, &numUniforms);

    std::vector<char> uniformName(256);

    for(int i = 0; i < numUniforms; ++i)
    {
        GLint dum1;
        GLenum dum2;

        glGetActiveUniform(program_.getId(), i, uniformName.size(), nullptr, &dum1, &dum2,
                           uniformName.data());

        auto uniformLocation = glGetUniformLocation(program_.getId(), uniformName.data());
        uniformLocations_[uniformName.data()] = uniformLocation;
    }

    return true;
}

} // namespace hppv

#endif // SHADER_IMPLEMENTATION
