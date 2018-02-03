#include <vector>
#include <thread>
#include <functional> // std::ref

#include <hppv/Scene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Texture.hpp>
#include <hppv/glad.h>
#include <hppv/imgui.h>

// ----- for d3_
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <hppv/Shader.hpp>
#include <hppv/GLobjects.hpp>
#include <hppv/Framebuffer.hpp>
// -----

#include "renderImage.hpp"
#include "../run.hpp"

// update (21-01-2018): the plan is to render the same scene with a ray-tracer, a software rasterizer
// and OpenGL (scene editor)

// todo:
// * better class and function names
// * move some parts into separate files
// * change the project name

class RayTracer: public hppv::Scene
{
public:
    RayTracer()
    {
        properties_.maximize = true;

        std::thread t1(renderImage1, image1_.buffer.data(), image1_.size, std::ref(image1_.renderProgress));
        t1.detach();

        std::thread t2(renderImage2, image2_.buffer.data(), image2_.size, std::ref(image2_.renderProgress));
        t2.detach();
    }

    void render(hppv::Renderer& renderer) override
    {
        // todo?: partial updates?
        if(!activeImage_->ready && (activeImage_->renderProgress == activeImage_->size.x * activeImage_->size.y))
        {
            activeImage_->ready = true;

            GLint unpackAlignment;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // 3 is not supported

            activeImage_->tex.bind();

            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, activeImage_->size.x, activeImage_->size.y, GL_RGB, GL_UNSIGNED_BYTE,
                            activeImage_->buffer.data());

            glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
        }

        if(activeImage_->ready)
        {
            const auto doesFit = activeImage_->size.x <= properties_.size.x &&
                                 activeImage_->size.y <= properties_.size.y;

            const auto proj = doesFit ? hppv::Space(0, 0, properties_.size) :
                                        hppv::expandToMatchAspectRatio(hppv::Space(0, 0, activeImage_->size), properties_.size);

            renderer.projection(proj);
            renderer.shader(hppv::Render::Tex);
            renderer.texture(activeImage_->tex);
            renderer.flipTextureY(true);

            hppv::Sprite sprite;
            sprite.size = activeImage_->size;
            sprite.pos = proj.pos + (proj.size - sprite.size) / 2.f;

            renderer.cache(sprite);
            renderer.flipTextureY(false);
        }
        else // progress bar
        {
            const hppv::Space space(0, 0, 100, 100);

            renderer.projection(hppv::expandToMatchAspectRatio(space, properties_.size));
            renderer.shader(hppv::Render::Color);

            hppv::Sprite sprite;
            sprite.size = {50.f, 3.f};
            sprite.pos = space.pos + (space.size - sprite.size) / 2.f;
            sprite.color = {0.2f, 0.2f, 0.2f, 1.f};

            renderer.cache(sprite);

            sprite.size.x *= static_cast<float>(activeImage_->renderProgress) / (activeImage_->size.x * activeImage_->size.y);
            sprite.color = {0.f, 1.f, 0.f, 1.f};

            renderer.cache(sprite);
        }

        // opengl 3d test window
        {
            renderer.flush();

            constexpr glm::vec2 size{200, 200};
            d3_.render(size, frame_.time);

            renderer.projection({0, 0, properties_.size});
            renderer.shader(hppv::Render::Tex);
            renderer.texture(d3_.fb.getTexture());

            hppv::Sprite s;
            s.pos = {0.f, 0.f};
            s.size = size;

            renderer.cache(s);
        }

        ImGui::Begin("image");
        {
            auto idx = (activeImage_ == &image1_) ? 0 : 1;
            const char* const ids[] = {"ray-tracer", "rasterizer"};
            ImGui::ListBox("technique", &idx, ids, hppv::size(ids));
            activeImage_ = (idx == 0) ? &image1_ : &image2_;
        }
        ImGui::End();
    }

private:
    struct Image
    {
        Image(): buffer(size.x * size.y), tex(GL_RGB8, size) {}

        static constexpr glm::ivec2 size{800, 600};
        std::vector<Pixel> buffer;
        hppv::Texture tex;
        std::atomic_int renderProgress = 0;
        bool ready = false;
    }
    image1_, image2_;

    Image* activeImage_ = &image2_;

    struct D3
    {
        D3(): fb(GL_RGBA8, 1)
        {
            glBindBuffer(GL_ARRAY_BUFFER, bo.getId());

            const glm::vec3 vertices[] =
            {
                {-1.f, -1.f, 0.f},
                {1.f, -1.f, 0.f},
                {0.f, 1.f, 0.f}
            };

            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            glBindVertexArray(vao.getId());

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(0);

            const char* const source = R"(

            #vertex
            #version 330

            layout(location = 0) in vec3 vertex;

            uniform mat4 projection;
            uniform mat4 view;
            uniform mat4 model;

            void main()
            {
                gl_Position = projection * view * model * vec4(vertex, 1);
            }

            #fragment
            #version 330

            out vec4 color;

            void main()
            {
                color = vec4(1, 1, 0, 1);
            }
            )";

            sh = hppv::Shader({source}, "d3");
        }

        void render(const glm::ivec2 size, float frameTime)
        {
            time += frameTime;

            // todo: depth testing

            glDisable(GL_BLEND);

            fb.bind();
            fb.setSize(size);
            fb.clear();

            glViewport(0, 0, size.x, size.y);

            sh.bind();
            {
                const auto projection = glm::perspective(camera.fovy, static_cast<float>(size.x) / size.y,
                                                         camera.zNear, camera.zFar);

                sh.uniformMat4f("projection", projection);

                const auto view = glm::lookAt(camera.eye, camera.eye + camera.dir, camera.up);
                sh.uniformMat4f("view", view);

                glm::mat4 model(1.f);

                constexpr auto angularVel = glm::radians(360.f / 10.f);
                //const auto x = glm::sin(angularVel * time);
                //const auto z = glm::cos(angularVel * time);
                //model = glm::translate(model, glm::vec3(x, 0.f, z));
                model = glm::rotate(model, angularVel * time, {0.f, 1.f, 0.f});

                sh.uniformMat4f("model", model);
            }

            glBindVertexArray(vao.getId());
            glDrawArrays(GL_TRIANGLES, 0, 3);

            fb.unbind();
        }

        struct
        {
            glm::vec3 eye = {0.f, 0.f, 3.f};

            // I find it more convenient than 'center'
            glm::vec3 dir = {0.f, 0.f, -1.f};

            glm::vec3 up = {0.f, 1.f, 0.f};
            float fovy = 90.f; // in degrees
            float zNear = 0.1f, zFar = 100.f;
        }
        const camera;

        hppv::Shader sh;
        hppv::GLvao vao;
        hppv::GLbo bo;
        hppv::Framebuffer fb;

        float time = 0.f;
    }
    d3_;
};

RUN(RayTracer)
