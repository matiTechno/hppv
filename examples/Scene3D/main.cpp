#include <cassert>
#include <iostream>
#include <vector>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp> // glm::radians
#include <glm/gtc/matrix_transform.hpp>

#include <hppv/glad.h>
#include <GLFW/glfw3.h>

#include <hppv/imgui.h>
#include <hppv/Shader.hpp>
#include <hppv/GLobjects.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Font.hpp>
#include <hppv/widgets.hpp>
#include <hppv/Texture.hpp>

#include "../run.hpp"

struct Mesh
{
    hppv::GLvao vao;
    // note: we are storing vertices and indices in one buffer
    hppv::GLbo bo;
    int numIndices = 0; // used for both glDrawArrays() and glDrawElements()
    GLsizeiptr indicesOffset = 0; // if 0 glDrawArrays() will be used
};

struct Model
{
    std::vector<Mesh> meshes;
};

Model loadModel(const std::string& filename)
{
    Assimp::Importer importer;
    const aiScene* const scene = importer.ReadFile(filename, aiProcess_Triangulate);

    if(!scene)
    {
        std::cout << "Assimp::Importer::ReadFile() failed: " << filename << std::endl;
        return {};
    }

    assert(scene->HasMeshes());
    // todo?: use something smaller than float for texCoords and normals?
    std::vector<float> vertexData;
    std::vector<unsigned> indices;
    Model model;

    for(unsigned k = 0; k < scene->mNumMeshes; ++k)
    {
        const auto& aimesh = *scene->mMeshes[k];
        // todo?: do all models have faces?
        assert(aimesh.HasFaces());
        assert(aimesh.mFaces[0].mNumIndices == 3);
        assert(aimesh.HasPositions());
        const auto hasTexCoords = aimesh.HasTextureCoords(0);
        const auto hasNormals = aimesh.HasNormals();
        const auto floatsPerVertex = 3 + hasTexCoords * 2 + hasNormals * 3;

        vertexData.reserve(aimesh.mNumVertices * floatsPerVertex);
        vertexData.clear(); // leaves the capacity() of the vector unchanged
        for(unsigned i = 0; i < aimesh.mNumVertices; ++i)
        {
            const auto v = aimesh.mVertices[i];
            vertexData.push_back(v.x);
            vertexData.push_back(v.y);
            vertexData.push_back(v.z);

            if(hasTexCoords)
            {
                const auto t = aimesh.mTextureCoords[0][i];
                vertexData.push_back(t.x);
                vertexData.push_back(t.y);
            }

            if(hasNormals)
            {
                const auto n = aimesh.mNormals[i];
                vertexData.push_back(n.x);
                vertexData.push_back(n.y);
                vertexData.push_back(n.z);
            }
        }

        indices.reserve(aimesh.mNumFaces * 3);
        indices.clear();
        for(unsigned i = 0; i < aimesh.mNumFaces; ++i)
        {
            for(auto j = 0; j < 3; ++j)
            {
                indices.push_back(aimesh.mFaces[i].mIndices[j]);
            }
        }

        auto& mesh = model.meshes.emplace_back();
        mesh.numIndices = indices.size();

        const GLsizeiptr verticesBytes = sizeof(float) * vertexData.size();
        const GLsizeiptr indicesBytes = sizeof(unsigned) * indices.size();

        mesh.indicesOffset = verticesBytes;

        glBindBuffer(GL_ARRAY_BUFFER, mesh.bo.getId());
        glBufferData(GL_ARRAY_BUFFER, verticesBytes + indicesBytes, nullptr, GL_STATIC_DRAW);

        glBufferSubData(GL_ARRAY_BUFFER, 0, verticesBytes, vertexData.data());
        glBufferSubData(GL_ARRAY_BUFFER, mesh.indicesOffset, indicesBytes, indices.data());

        glBindVertexArray(mesh.vao.getId());

        const GLsizei stride = floatsPerVertex * sizeof(float);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
        glEnableVertexAttribArray(0);

        if(hasTexCoords)
        {
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                                  reinterpret_cast<const void*>(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
        }

        if(hasNormals)
        {
            const auto offset = 3 * sizeof(float) + hasTexCoords * 2 * sizeof(float);

            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride,
                                  reinterpret_cast<const void*>(offset));
            glEnableVertexAttribArray(2);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.bo.getId());
    }

    return model;
}

class Scene3D: public hppv::Scene
{
public:
    Scene3D():
        font_(hppv::Font::Default(), 13),
        sh_(hppv::Shader::File(), "res/test.sh"),
        texDiffuse_("../RayTracer/res/african_head_diffuse.tga")
    {
        properties_.maximize = true;
        camera_.enterCameraMode();

        const glm::vec3 vertices[] =
        {
            {-1.f, -1.f, 0.f},
            {1.f, -1.f, 0.f},
            {0.f, 1.f, 0.f}
        };

        glBindBuffer(GL_ARRAY_BUFFER, bo_.getId());
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindVertexArray(vao_.getId());
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        model_ = loadModel("../RayTracer/res/african_head.obj");
    }

    void processInput(const std::vector<hppv::Event>& events) override
    {
        for(const auto& event: events)
        {
            camera_.processEvent(event);
        }
    }

    void render(hppv::Renderer& renderer) override
    {
        time_ += frame_.time;

        // hppv::App already cleared the color buffer
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        assert(properties_.maximize);
        glViewport(0, 0, properties_.size.x, properties_.size.y);

        const glm::vec3 lightPos(4.f, 4.f, 8.f);
        const glm::vec3 lightColor(1.f, 1.f, 1.f);

        camera_.update(frame_);
        sh_.bind();
        sh_.uniformMat4f("projection", camera_.projection);
        sh_.uniformMat4f("view", camera_.view);
        sh_.uniform3f("lightPos", lightPos);
        sh_.uniform3f("lightColor", lightColor);

        // todo: render commands
        {
            glm::mat4 model(1.f);
            // our little triangle will represent the light
            model = glm::translate(model, lightPos);
            const auto angularVel = glm::radians(360.f / 10.f);
            model = glm::rotate(model, angularVel * time_, {0.f, 1.f, 0.f});

            sh_.uniformMat4f("model", model);
            sh_.uniform4f("color", glm::vec4(lightColor, 1.f));
            sh_.uniform1i("useTexture", false);
            sh_.uniform1i("doLighting", false);

            glBindVertexArray(vao_.getId());
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);

            sh_.uniformMat4f("model", glm::mat4(1.f));
            sh_.uniform4f("color", {1.f, 1.f, 1.f, 1.f});
            sh_.uniform1i("useTexture", true);
            sh_.uniform1i("doLighting", true);
            texDiffuse_.bind();

            for(auto& mesh: model_.meshes)
            {
                glBindVertexArray(mesh.vao.getId());
                glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT,
                               reinterpret_cast<const void*>(mesh.indicesOffset));
            }

            glDisable(GL_CULL_FACE);
        }

        appWidget_.update(frame_);

        ImGui::Begin("Scene3D");
        appWidget_.imgui(frame_);
        camera_.imgui();
        ImGui::End();

        renderer.projection({0.f, 0.f, properties_.size});
        renderer.shader(hppv::Render::Font);
        renderer.texture(font_.getTexture());
        hppv::Text text(font_);
        text.text = "hppv::Renderer test";
        text.pos = glm::vec2(20.f);
        text.color = {0.f, 0.7f, 0.f, 1.f};
        renderer.cache(text);
        renderer.flush();
    }

private:
    float time_ = 0.f;
    hppv::AppWidget appWidget_;
    hppv::Camera camera_;
    hppv::Font font_;
    hppv::Shader sh_;
    hppv::GLvao vao_;
    hppv::GLbo bo_;
    Model model_;
    // todo: mipmapping, gamma correction, sampler
    // todo?: Sampler class?
    hppv::Texture texDiffuse_;
};

RUN(Scene3D)
