#include <hppv/Scene.hpp>
#include <hppv/App.hpp>

namespace hppv
{

Scene::Scene():
    frame_(App::getFrame())
{}

void Scene::processInput(bool hasInput)
{
    (void)hasInput;
}

void Scene::update()
{}

void Scene::render(Renderer& renderer)
{
    (void)renderer;
}

} // namespace hppv
