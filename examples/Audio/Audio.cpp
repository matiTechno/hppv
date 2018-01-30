#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>

#include <hppv/App.hpp>
#include <hppv/Scene.hpp>
#include <hppv/Deleter.hpp>
#include <hppv/imgui.h>
#include <hppv/Renderer.hpp>

#include "fmod/fmod.hpp"
#include "fmod/fmod_errors.h"

// returns true on success

bool check(const FMOD_RESULT r)
{
    if(r != FMOD_OK)
    {
        std::cout << "FMOD: error (" << r << ") " << FMOD_ErrorString(r) << std::endl;
        return false;
    }

    return true;
}

std::string getConfigFilename()
{
    return std::string(std::getenv("HOME")) + "/.Audio-volume";
}

class Audio: public hppv::Scene
{
public:
    Audio(const char* const filename)
    {
        properties_.maximize = true;

        if(check(FMOD::System_Create(&system_)) == false)
        {
            hppv::App::request(hppv::Request::Quit);
            return;
        }

        del_.set([system = system_]{check(system->release());});

        if(check(system_->init(512, FMOD_INIT_NORMAL, nullptr)) == false)
        {
            hppv::App::request(hppv::Request::Quit);
            return;
        }

        FMOD::Sound* sound;

        if(check(system_->createSound(filename, FMOD_LOOP_NORMAL | FMOD_CREATESTREAM, nullptr, &sound)) == false)
        {
            std::cout << filename << std::endl;
            hppv::App::request(hppv::Request::Quit);
            return;
        }

        check(sound->getName(name_, hppv::size(name_)));
        check(system_->playSound(sound, nullptr, false, &channel_));

        // todo?: FFT configuration?
        check(system_->createDSPByType(FMOD_DSP_TYPE_FFT, &dsp_));
        check(channel_->addDSP(1, dsp_));

        std::ifstream file(getConfigFilename());
        if(file.is_open())
        {
            float volume;
            if(file >> volume)
            {
                check(channel_->setVolume(volume));
            }
        }
    }

    ~Audio() override
    {
        float volume;
        if(check(channel_->getVolume(&volume)))
        {
            std::ofstream file(getConfigFilename(), file.trunc);
            if(file.is_open())
            {
                file << volume;
            }
        }
    }

    void render(hppv::Renderer& renderer) override
    {
        check(system_->update());

        FMOD_DSP_PARAMETER_FFT* fft;
        if(check(dsp_->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, reinterpret_cast<void**>(&fft),
                                        nullptr, nullptr, 0)))
        {
            const hppv::Space space(0.f, 0.f, 1.f, 1.f);

            renderer.projection(hppv::expandToMatchAspectRatio(space, properties_.size));

            for(auto channel = 0; channel < fft->numchannels; ++channel)
            {
                const auto len = fft->length / 2;

                for(auto i = 0; i < len; ++i)
                {
                    const auto a = fft->spectrum[channel][i];

                    hppv::Sprite s;
                    s.size = {space.size.x / len, a + 0.005f};
                    s.pos = {space.pos.x + s.size.x * i, space.pos.y + space.size.y - s.size.y - 0.05f};
                    s.color = {1.f, 0.5f, 0.f, 0.f};

                    renderer.cache(s);
                }
            }
        }

        ImGui::Begin("Audio");
        ImGui::Text("playing: %s", name_);

        float volume;
        check(channel_->getVolume(&volume));

        ImGui::PushItemWidth(150);

        if(ImGui::SliderFloat("volume", &volume, 0.f, 1.f))
        {
            check(channel_->setVolume(volume));
        }

        ImGui::PopItemWidth();
        ImGui::End();
    }

private:
    FMOD::System* system_;
    hppv::Deleter del_;
    FMOD::Channel* channel_;
    FMOD::DSP* dsp_;
    char name_[256];
};

int main(int argc, const char* const * const argv)
{
    if(argc > 2)
    {
        std::cout << "usage: Audio\n"
                     "       Audio <filename>" << std::endl;

        return 1;
    }

    hppv::App::InitParams p;
    p.window.title = "Audio";
    hppv::App app;
    if(!app.initialize(p)) return 1;
    app.pushScene<Audio>(argc == 2 ? argv[1] : "res/520387_Horizon_short.mp3");
    app.run();
    return 0;
}
