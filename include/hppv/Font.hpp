#pragma once

#include <map> // todo: replace with flat_map
#include <memory>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Texture.hpp"

namespace hppv
{

// github.com/libgdx/libgdx/wiki/Distance-field-fonts

struct Glyph
{
    glm::ivec4 texCoords;
    glm::ivec2 offset;
    int advance;
};

// todo: descent
class Font
{
public:
    Font(const std::string& filename);

    Texture& getTexture() {return *texture_;}
    Glyph getGlyph(int code) const;
    int getAscent() const {return ascent_;}
    int getDescent() const {return descent_;}
    int getLineHeight() const {return lineHeight_;}

private:
    std::unique_ptr<Texture> texture_;
    int ascent_;
    int descent_;
    int lineHeight_;

    std::map<int, Glyph> glyphs_;
};

} // namespace hppv
