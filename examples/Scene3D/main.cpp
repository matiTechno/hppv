#include <cassert>
#include <algorithm>

#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <hppv/glad.h>
#include <GLFW/glfw3.h>

#include <hppv/Scene.hpp>
#include <hppv/imgui.h>
#include <hppv/Shader.hpp>
#include <hppv/GLobjects.hpp>

#include "../run.hpp"

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
        sh_(hppv::Shader::File(), "res/test.glsl")
    {
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
        {
            const auto xzDir = glm::normalize(glm::vec3(camera_.dir.x, 0.f, camera_.dir.z));

            if(keyActive(GLFW_KEY_W))
            {
                camera_.eye += xzDir * camera_.vel * frame_.time;
            }
            if(keyActive(GLFW_KEY_S))
            {
                camera_.eye -= xzDir * camera_.vel * frame_.time;
            }
        }
        {
            const auto right = glm::normalize(glm::cross(camera_.dir, camera_.up));

            if(keyActive(GLFW_KEY_A))
            {
                camera_.eye -= right * camera_.vel * frame_.time;
            }
            if(keyActive(GLFW_KEY_D))
            {
                camera_.eye += right * camera_.vel * frame_.time;
            }
        }
        if(keyActive(GLFW_KEY_SPACE))
        {
            camera_.eye += camera_.up * camera_.vel * frame_.time;
        }
        if(keyActive(GLFW_KEY_LEFT_SHIFT))
        {
            camera_.eye -= camera_.up * camera_.vel * frame_.time;
        }
    }

    void render(hppv::Renderer&) override
    {
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

            glm::mat4 model(1.f);
            constexpr auto angularVel = glm::radians(360.f / 10.f);
            //const auto x = glm::sin(angularVel * time);
            //const auto z = glm::cos(angularVel * time);
            //model = glm::translate(model, glm::vec3(x, 0.f, z));
            time_ += frame_.time;
            model = glm::rotate(model, angularVel * time_, {0.f, 1.f, 0.f});
            sh_.uniformMat4f("model", model);
        }

        glBindVertexArray(vao_.getId());
        glDrawArrays(GL_TRIANGLES, 0, 3);

        ImGui::Begin("Scene3D");
        ImGui::Text("Esc - toggle the camera mode");
        ImGui::Text("pitch / yaw - mouse");
        ImGui::Text("move - wsad (xz plane), space (up), lshift (down)");
        ImGui::End();
    }

private:
    bool cameraMode_ = false;
    glm::vec2 cursorPos_;
    float time_ = 0.f;
    hppv::Shader sh_;
    hppv::GLvao vao_;
    hppv::GLbo bo_;

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
        const float zFar = 100.f;
        const float vel = 4.f;

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
