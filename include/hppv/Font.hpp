#pragma once

#include <map>
#include <string>
#include <fstream>
#include <string_view>

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

// bug?: hiero does not produce correct offset / texRect values?

class Font
{
public:
    Font() = default;

    // Angle Code font format - *.fnt

    // the ASCII character set is loaded by default
    // TrueType - *.ttf
    // OpenType - *.otf

    // todo:
    // * move font loading to free functions / overload the constructor
    // * replace additionalChars with something like imgui GlyphRanges

    explicit Font(const std::string& filename, int sizePx = 20, std::string_view additionalChars = "");

    Texture& getTexture() {return texture_;}

    Glyph getGlyph(int code) const;

    int getLineHeight() const {return lineHeight_;}

private:
    enum {TexSizeX = 256, Offset = 1};

    Texture texture_;
    // todo?: replace int with unsigned int?
    std::map<int, Glyph> glyphs_;
    int lineHeight_;

    void loadFnt(const std::string& filename);
    void loadTrueType(const std::string& filename, int sizePx, std::string_view additionalChars);
};

} // namespace hppv
