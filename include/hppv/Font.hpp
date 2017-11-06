#pragma once

#include <map>
#include <string>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Texture.hpp"

namespace hppv
{

struct Glyph
{
    glm::ivec4 texRect;
    glm::ivec2 offset;
    int advance;
};

// github.com/libgdx/libgdx/wiki/Distance-field-fonts
// github.com/libgdx/libgdx/wiki/Hiero

class Font
{
public:
    // Angle Code font format
    explicit Font(const std::string& filename);

    Texture& getTexture() {return texture_;}
    Glyph getGlyph(int code) const;
    int getAscent() const {return ascent_;}
    int getLineHeight() const {return lineHeight_;}

private:
    Texture texture_;
    std::map<int, Glyph> glyphs_;
    int ascent_;
    int lineHeight_;
};

} // namespace hppv
