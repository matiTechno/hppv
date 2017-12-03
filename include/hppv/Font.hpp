#pragma once

#include <map>
#include <string>
#include <fstream>

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
    Font() = default;

    // Angle Code font format - *.fnt
    // TrueType - *.ttf / *.otf (currently only the ASCII character set is loaded)

    // todo: move font loading to free functions / overload the constructor

    // bug?: hiero does not produce correct offset values

    explicit Font(const std::string& filename, int sizePx = 20);

    Texture& getTexture() {return texture_;}
    Glyph getGlyph(int code) const;
    int getAscent() const {return ascent_;}
    int getLineHeight() const {return lineHeight_;}

private:
    enum {TexSizeX = 256, Offset = 1};

    Texture texture_;
    std::map<int, Glyph> glyphs_;
    int ascent_;
    int lineHeight_;

    void loadFnt(const std::string& filename);
    void loadTrueType(const std::string& filename, int sizePx);
};

} // namespace hppv
