#include <cassert>
#include <algorithm> // std::find
#include <iostream>
#include <vector>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/vec3.hpp>
#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <hppv/glad.h>
#include <GLFW/glfw3.h>

#include <hppv/Scene.hpp>
#include <hppv/imgui.h>
#include <hppv/Shader.hpp>
#include <hppv/GLobjects.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Font.hpp>
#include <hppv/widgets.hpp>

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
    // todo: vertices -> vertexData (store here all vertex components interleaved)
    std::vector<glm::vec3> vertices;
    std::vector<unsigned> indices;
    Model model;

    for(unsigned k = 0; k < scene->mNumMeshes; ++k)
    {
        {
            const auto& mesh = *scene->mMeshes[k];
            // todo?: do all models have faces?
            assert(mesh.HasFaces());
            assert(mesh.mFaces[0].mNumIndices == 3);

            vertices.reserve(mesh.mNumVertices);
            vertices.clear();
            for(unsigned i = 0; i < mesh.mNumVertices; ++i)
            {
                const auto v = mesh.mVertices[i];
                vertices.emplace_back(v.x, v.y, v.z);
            }

            indices.reserve(mesh.mNumFaces * 3);
            indices.clear();
            for(unsigned i = 0; i < mesh.mNumFaces; ++i)
            {
                for(auto j = 0; j < 3; ++j)
                {
                    indices.push_back(mesh.mFaces[i].mIndices[j]);
                }
            }
        }

        auto& mesh = model.meshes.emplace_back();
        mesh.numIndices = indices.size();

        const GLsizeiptr verticesBytes = sizeof(glm::vec3) * vertices.size();
        const GLsizeiptr indicesBytes = sizeof(unsigned) * indices.size();

        mesh.indicesOffset = verticesBytes;

        glBindBuffer(GL_ARRAY_BUFFER, mesh.bo.getId());
        glBufferData(GL_ARRAY_BUFFER, verticesBytes + indicesBytes, nullptr, GL_STATIC_DRAW);

        glBufferSubData(GL_ARRAY_BUFFER, 0, verticesBytes, vertices.data());
        glBufferSubData(GL_ARRAY_BUFFER, mesh.indicesOffset, indicesBytes, indices.data());

        glBindVertexArray(mesh.vao.getId());

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.bo.getId());
    }

    return model;
}

void tryInsert(std::vector<int>& vec, const int key)
{
    if(std::find(vec.cbegin(), vec.cend(), key) == vec.cend())
    {
        vec.push_back(key);
    }
}

void tryErase(std::vector<int>& vec, const int key)
{
    if(const auto it = std::find(vec.cbegin(), vec.cend(), key); it != vec.cend())
    {
        vec.erase(it);
    }
}

bool find(const std::vector<int>& vec, const int key)
{
    return std::find(vec.cbegin(), vec.cend(), key) != vec.cend();
}

class Scene3D: public hppv::Scene
{
public:
    Scene3D():
        font_(hppv::Font::Default(), 13),
        sh_(hppv::Shader::File(), "res/test.sh")
    {
        {
            // todo: camera jump (also when resizing the window)
            hppv::Request r(hppv::Request::Window);
            r.window.state = hppv::Window::Fullscreen;
            hppv::App::request(r);
        }

        properties_.maximize = true;

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

        toggleCameraMode();
    }

    void processInput(const std::vector<hppv::Event>& events) override
    {
        keys_.pressed.clear();
        keys_.released.clear();

        for(const auto& event: events)
        {
            if(event.type == hppv::Event::Key)
            {
                if(event.key.action == GLFW_PRESS)
                {
                    tryInsert(keys_.pressed, event.key.key);
                    tryInsert(keys_.held, event.key.key);
                }
                else if(event.key.action == GLFW_RELEASE)
                {
                    tryInsert(keys_.released, event.key.key);
                    tryErase(keys_.held, event.key.key);
                }
            }
            else if(event.type == hppv::Event::Cursor && cameraMode_)
            {
                const auto offset = event.cursor.pos - cursorPos_;
                cursorPos_ = event.cursor.pos;
                constexpr auto coeff = 0.1f;
                camera_.pitch -= offset.y * coeff;
                // todo: why 89 and not 90? (learnopengl.com/Getting-started/Camera)
                camera_.pitch = glm::min(89.f, camera_.pitch);
                camera_.pitch = glm::max(-89.f, camera_.pitch);
                camera_.yaw = glm::mod(camera_.yaw + offset.x * coeff, 360.f);
            }
        }

        if(find(keys_.pressed, GLFW_KEY_ESCAPE))
        {
            toggleCameraMode();
        }

        // update the camera direction
        camera_.dir.x = glm::cos(glm::radians(camera_.pitch)) * glm::cos(glm::radians(camera_.yaw));
        camera_.dir.y = glm::sin(glm::radians(camera_.pitch));
        camera_.dir.z = glm::cos(glm::radians(camera_.pitch)) * glm::sin(glm::radians(camera_.yaw));
        camera_.dir = glm::normalize(camera_.dir);

        // update the camera position

        if(keyActive(GLFW_KEY_W))
        {
            camera_.eye += camera_.dir * camera_.speed * frame_.time;
        }
        if(keyActive(GLFW_KEY_S))
        {
            camera_.eye -= camera_.dir * camera_.speed * frame_.time;
        }
        {
            const auto right = glm::normalize(glm::cross(camera_.dir, camera_.up));

            if(keyActive(GLFW_KEY_A))
            {
                camera_.eye -= right * camera_.speed * frame_.time;
            }
            if(keyActive(GLFW_KEY_D))
            {
                camera_.eye += right * camera_.speed * frame_.time;
            }
        }
        if(keyActive(GLFW_KEY_SPACE))
        {
            camera_.eye += camera_.up * camera_.speed * frame_.time;
        }
        if(keyActive(GLFW_KEY_LEFT_SHIFT))
        {
            camera_.eye -= camera_.up * camera_.speed * frame_.time;
        }
    }

    void render(hppv::Renderer& renderer) override
    {
        // hppv::App already cleared the color buffer
        glClear(GL_DEPTH_BUFFER_BIT);

        assert(properties_.maximize);
        const auto size = properties_.size;
        glViewport(0, 0, size.x, size.y);

        sh_.bind();
        {
            const auto projection = glm::perspective(camera_.fovy, static_cast<float>(size.x) / size.y,
                                                     camera_.zNear, camera_.zFar);

            sh_.uniformMat4f("projection", projection);

            const auto view = glm::lookAt(camera_.eye, camera_.eye + camera_.dir, camera_.up);
            sh_.uniformMat4f("view", view);
        }
        // todo: render commands
        {
            glm::mat4 model(1.f);
            constexpr auto angularVel = glm::radians(360.f / 10.f);
            model = glm::translate(model, glm::vec3(0.f, 0.f, -5.f));
            time_ += frame_.time;
            model = glm::rotate(model, angularVel * time_, {0.f, 1.f, 0.f});
            sh_.uniformMat4f("model", model);
            sh_.uniform4f("color", {1.f, 0.5f, 0.f, 1.f});
            glBindVertexArray(vao_.getId());
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            sh_.uniformMat4f("model", glm::mat4(1.f));
            sh_.uniform4f("color", {0.1f, 0.1f, 0.f, 0.f});

            for(auto& mesh: model_.meshes)
            {
                glBindVertexArray(mesh.vao.getId());
                glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT,
                               reinterpret_cast<const void*>(mesh.indicesOffset));
            }

            glDisable(GL_BLEND);
        }

        appWidget_.update(frame_);

        ImGui::Begin("Scene3D");
        appWidget_.imgui(frame_);
        ImGui::Text("toggle the camera mode - Esc");
        ImGui::Text("pitch / yaw - mouse");
        ImGui::Text("move - wsad, space (up), lshift (down)");
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
    bool cameraMode_ = false;
    glm::vec2 cursorPos_;
    float time_ = 0.f;
    hppv::AppWidget appWidget_;
    hppv::Font font_;
    hppv::Shader sh_;
    hppv::GLvao vao_;
    hppv::GLbo bo_;
    Model model_;

    struct
    {
        std::vector<int> pressed;
        std::vector<int> released;
        std::vector<int> held;
    }
    keys_;

    struct
    {
        glm::vec3 eye = {0.f, 0.f, 3.f};
        glm::vec3 dir;
        const glm::vec3 up = {0.f, 1.f, 0.f};
        const float fovy = 90.f; // degrees
        const float zNear = 0.1f;
        const float zFar = 10000.f;
        float speed = 4.f;

        // degrees
        float pitch = 0.f;
        float yaw = -90.f; // -z direction
    }
    camera_;

    bool keyActive(int key) const {return find(keys_.held, key) || find(keys_.pressed, key);}

    void toggleCameraMode()
    {
        cameraMode_ = !cameraMode_;
        hppv::Request r(hppv::Request::Cursor);
        r.cursor.mode = cameraMode_ ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
        hppv::App::request(r);
        cursorPos_ = hppv::App::getCursorPos();
    }
};

RUN(Scene3D)
