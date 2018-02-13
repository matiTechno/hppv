// inspired by FEZ

#include <vector>
#include <algorithm> // std::max, std::swap
#include <random>
#include <fstream>
#include <string>

#include <hppv/glad.h>
#include <GLFW/glfw3.h>

#include <hppv/App.hpp>
#include <hppv/Scene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>
#include <hppv/Shader.hpp>
#include <hppv/Font.hpp>

#include "../run.hpp"

const char* const gradientSource = R"(

#fragment
#version 330

in vec2 vPos;

uniform float time;
uniform float duration;
uniform vec4 colorTopCurrent;
uniform vec4 colorBotCurrent;
uniform vec4 colorTopNext;
uniform vec4 colorBotNext;

const float edge = 0.666;

float map(float value, float inStart, float inEnd, float outStart, float outEnd)
{
    return outStart + (outEnd - outStart) / (inEnd - inStart) * (value - inStart);
}

out vec4 color;

void main()
{
    float y;

    if(vPos.y <= edge)
    {
        y = map(vPos.y, 0.0, edge, 0.0, 0.5);
    }
    else
    {
        y = map(vPos.y, edge, 1.0, 0.5, 1.0);
    }

    float lerpCoeff = time / duration;
    vec4 colorTop = mix(colorTopCurrent, colorTopNext, lerpCoeff);
    vec4 colorBot = mix(colorBotCurrent, colorBotNext, lerpCoeff);
    color = mix(colorTop, colorBot, y);
}
)";

struct Gradient
{
    std::string id;
    float duration;
    glm::vec4 colorTop;
    glm::vec4 colorBot;
};

void imguiGradientEdit(std::vector<Gradient>::iterator it, float* const time, bool current)
{
    const char* const title = current ? "current:" : "next:   ";
    ImGui::Text(title);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, {1.f, 1.f, 0.f, 1.f});
    ImGui::Text("%s", it->id.c_str());
    ImGui::PopStyleColor();

    if(current)
    {
        if(ImGui::InputFloat("duration(s)", &it->duration))
        {
            it->duration = std::max(0.f, it->duration);
            *time = 0.f;
        }
    }
    else
    {
        ImGui::Spacing();
        ImGui::Text("read-only");
        hppv::imguiPushDisabled(1.f);
    }

    ImGui::PushID(title);
    ImGui::ColorEdit4("color top", &it->colorTop.x, ImGuiColorEditFlags_AlphaBar);
    ImGui::ColorEdit4("color bot", &it->colorBot.x, ImGuiColorEditFlags_AlphaBar);
    ImGui::PopID();

    if(current == false)
    {
        hppv::imguiPopDisabled();
    }
}

void imguiNewSection()
{
    ImGui::Separator();
    ImGui::Spacing();
}

template<typename Vec, typename It>
auto getNext(Vec& vec, It it)
{
    return it == vec.end() - 1 ? vec.begin() : ++it;
}

class DayNight: public hppv::Scene
{
public:
    DayNight():
        sh_({hppv::Renderer::vInstancesSource, gradientSource}, "sh_"),
        font_(hppv::Font::Default(), 52)
    {
        properties_.maximize = true;

        {
            std::ifstream file(filename_);
            if(file.is_open())
            {
                std::string dum;
                Gradient g;

                while(std::getline(file, g.id)
                      >> g.duration
                      >> g.colorTop.r >> g.colorTop.g >> g.colorTop.b >> g.colorTop.a
                      >> g.colorBot.r >> g.colorBot.g >> g.colorBot.b >> g.colorBot.a
                      && std::getline(file, dum))
                {
                    gradients_.push_back(g);
                }
            }
        }

        if(gradients_.empty())
        {
            gradients_.push_back({"default", 5.f, {0.f, 0.f, 1.f, 0.f}, {1.f, 0.5f, 0.f, 0.f}});
        }

        current_ = gradients_.begin();

        std::random_device rd;
        std::mt19937 rng(rd());

        const auto pos = space_.pos;
        const auto size = space_.size;

        // we want to cover some range of aspect ratios
        constexpr auto offset = 50.f;

        std::uniform_real_distribution<float> distX(pos.x - offset, pos.x + size.x + offset);
        std::uniform_real_distribution<float> distY(pos.y - offset, pos.y + size.y + offset);

        for(auto& star: stars_)
        {
            star.pos = {distX(rng), distY(rng)};
        }
    }

    void processInput(const std::vector<hppv::Event>& events) override
    {
        for(const auto& event: events)
        {
            if(event.type == hppv::Event::Key && event.key.action == GLFW_PRESS
                                              && event.key.key == GLFW_KEY_ENTER)
            {
                hideEditor_ = !hideEditor_;

                hppv::Request r(hppv::Request::Cursor);
                r.cursor.mode = hideEditor_ ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL;
                hppv::App::request(r);
            }
        }
    }

    void render(hppv::Renderer& renderer) override;
    void editor();

private:
    const char* const filename_ = "data.txt";
    const hppv::Space space_ = {0.f, 0.f, 100.f, 100.f};
    hppv::Shader sh_;
    hppv::Font font_;
    std::vector<Gradient> gradients_;
    std::vector<Gradient>::iterator current_;
    hppv::Vertex stars_[500];
    float time_ = 0.f;
    int speed_ = 1;
    char textInputBuf_[256] = {'\0'};
    bool goToNext_ = true;
    bool hideEditor_ = false;
};

void DayNight::render(hppv::Renderer &renderer)
{
    time_ += frame_.time * speed_;

    if(time_ > current_->duration)
    {
        time_ = 0.f;

        if(goToNext_)
        {
            current_ = getNext(gradients_, current_);
        }
    }

    renderer.projection({0, 0, properties_.size});
    renderer.mode(hppv::RenderMode::Instances);
    renderer.shader(hppv::Render::Font);
    renderer.texture(font_.getTexture());
    {
        hppv::Text text(font_);
        text.text = "tranquil";
        text.color /= 2.f;
        text.pos = (properties_.size - glm::ivec2(text.getSize())) / 2;
        renderer.cache(text);
    }

    const auto projection = hppv::expandToMatchAspectRatio(space_, properties_.size);

    renderer.projection(projection);

    renderer.mode(hppv::RenderMode::Vertices);
    renderer.shader(hppv::Render::VerticesColor);
    renderer.primitive(GL_POINTS);
    renderer.cache(stars_, size(stars_));

    renderer.mode(hppv::RenderMode::Instances);
    renderer.shader(sh_);
    {
        const auto next = getNext(gradients_, current_);

        renderer.uniform1f("time", time_);
        renderer.uniform1f("duration", current_->duration);
        renderer.uniform4f("colorTopCurrent", current_->colorTop);
        renderer.uniform4f("colorBotCurrent", current_->colorBot);
        renderer.uniform4f("colorTopNext", next->colorTop);
        renderer.uniform4f("colorBotNext", next->colorBot);
        renderer.cache(hppv::Sprite(projection));
    }

    if(!hideEditor_)
    {
        editor();
    }
}

void DayNight::editor()
{
    ImGui::Begin("editor");

    ImGui::Text("Enter - hide / show");

    int idx = current_ - gradients_.begin();

    if(ImGui::ListBox("gradients", &idx,
                      [](void* data, int idx, const char** outText)
                      {
                          *outText = reinterpret_cast<Gradient*>(data)[idx].id.c_str(); return true;
                      },
                      reinterpret_cast<void*>(gradients_.data()), gradients_.size(), 10))
    {
        time_ = 0.f;
    }

    current_ = gradients_.begin() + idx;

    const bool disabled = speed_ && goToNext_;

    if(disabled)
    {
        hppv::imguiPushDisabled();
    }

    if(ImGui::InputText("change name", textInputBuf_, hppv::size(textInputBuf_), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        current_->id = textInputBuf_;
    }

    ImGui::NewLine();

    if(ImGui::Button("up"))
    {
        if(current_ > gradients_.begin())
        {
            std::swap(*current_, *(current_ - 1));
            --idx;
            current_ = gradients_.begin() + idx;
            time_ = 0.f;
        }
    }

    ImGui::SameLine();

    if(ImGui::Button("down"))
    {
        if(current_ < gradients_.end() - 1)
        {
            std::swap(*current_, *(current_ + 1));
            ++idx;
            current_ = gradients_.begin() + idx;
            time_ = 0.f;
        }
    }

    ImGui::SameLine();

    if(ImGui::Button("new"))
    {
        gradients_.insert(current_ + 1, *current_);
        ++idx;
        current_ = gradients_.begin() + idx;
        current_->id = "new";
        time_ = 0.f;
    }

    ImGui::SameLine(200);

    if(ImGui::Button("delete"))
    {
        if(gradients_.size() > 1)
        {
            gradients_.erase(current_);

            if(idx)
            {
                --idx;
            }

            current_ = gradients_.begin() + idx;
            time_ = 0.f;
        }
    }

    imguiNewSection();

    imguiGradientEdit(current_, &time_, true);
    imguiGradientEdit(getNext(gradients_, current_), &time_, false);

    if(disabled)
    {
        hppv::imguiPopDisabled();
    }

    imguiNewSection();

    // bug: data.txt gradient 6 - value can exceed the duration
    ImGui::SliderFloat("time(s)", &time_, 0.f, current_->duration);

    ImGui::Text("set to 0 or disable 'go to next'\nto enable modifiers");
    ImGui::InputInt("speed", &speed_);
    speed_ = std::max(0, speed_);

    ImGui::Checkbox("go to next", &goToNext_);

    imguiNewSection();

    if(ImGui::Button("save"))
    {
        std::ofstream file(filename_, file.trunc);
        if(file.is_open())
        {
            for(const auto& g: gradients_)
            {
                file << g.id << '\n'
                     << g.duration << '\n'
                     << g.colorTop.r << ' ' << g.colorTop.g << ' ' << g.colorTop.b << ' ' << g.colorTop.a << '\n'
                     << g.colorBot.r << ' ' << g.colorBot.g << ' ' << g.colorBot.b << ' ' << g.colorBot.a << '\n';
            }
        }
    }

    ImGui::SameLine(200);

    if(ImGui::Button("quit"))
    {
        hppv::App::request(hppv::Request::Quit);
    }

    ImGui::End();
}

RUN(DayNight)
