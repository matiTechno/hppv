#pragma once

#include <map>

#include <glm/vec2.hpp>

#include "Texture.hpp"
#include "Rect.hpp"

namespace hppv
{

// github.com/libgdx/libgdx/wiki/Distance-field-fonts

struct Glyph
{
    Rect texRect;
    glm::ivec2 offset;
    int advance;
};

class Font
{
public:
    Font(const std::string& filename);
    Font(Texture texture, std::map<int, Glyph> glyphs, int ascent, int lineHeight);

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
