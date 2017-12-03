#include <iostream>
#include <fstream>
#include <experimental/filesystem>
#include <vector>
#include <algorithm>

#include <hppv/Font.hpp>
#include <hppv/glad.h>

// imgui needs it
#define STB_RECT_PACK_IMPLEMENTATION
#include "imgui/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "imgui/stb_truetype.h"

namespace hppv
{

namespace fs = std::experimental::filesystem;

Font::Font(const std::string& filename, int sizePx)
{
    // replace find with regex (*.ext)?

    if(filename.find(".fnt") != std::string::npos)
    {
        loadFnt(filename);
    }
    else if(filename.find(".ttf") != std::string::npos ||
            filename.find(".otf") != std::string::npos)
    {
        loadTrueType(filename, sizePx);
    }
    else
    {
        std::cout << "Font: unsupported file format - " << filename << std::endl;
    }
}

Glyph Font::getGlyph(int code) const
{
    if(auto it = glyphs_.find(code); it != glyphs_.end())
        return it->second;

    if(auto it = glyphs_.find('?'); it != glyphs_.end())
        return it->second;

    return {};
}

void printFileOpenError(const std::string& filename)
{
    std::cout << "Font: could not open file = " << filename << std::endl;
}

struct Value
{
    int value;
    std::string str;
    std::size_t posNext;
};

Value getValue(const std::string& str, std::size_t pos, char prev = '=', char end = ' ')
{
    auto start = str.find(prev, pos) + 1;
    auto count = str.find(end, start) - start;
    auto substr = str.substr(start, count);
    return {std::atoi(substr.c_str()), substr, start + count};
}

void Font::loadFnt(const std::string& filename)
{
    std::ifstream file(filename);

    if(!file)
    {
        printFileOpenError(filename);
        return;
    }

    std::string line;

    for(int i = 0; i < 2; ++i)
    {
        std::getline(file, line);
    }

    auto value = getValue(line, 0);
    lineHeight_ = value.value;

    value = getValue(line, value.posNext);
    ascent_ = value.value;

    std::getline(file, line);

    texture_ = Texture(fs::path(filename).parent_path() /= getValue(line, 0, '"', '"').str);

    std::getline(file, line);

    auto numGlyphs = getValue(line, 0).value;

    for(int i = 0; i < numGlyphs; ++i)
    {
        std::getline(file, line);

        value = getValue(line, 0);
        auto& glyph = glyphs_[value.value];

        value = getValue(line, value.posNext);
        glyph.texRect.x = value.value;

        value = getValue(line, value.posNext);
        glyph.texRect.y = value.value;

        value = getValue(line, value.posNext);
        glyph.texRect.z = value.value;

        value = getValue(line, value.posNext);
        glyph.texRect.w = value.value;

        value = getValue(line, value.posNext);
        glyph.offset.x = value.value;

        value = getValue(line, value.posNext);
        glyph.offset.y = value.value;

        value = getValue(line, value.posNext);
        glyph.advance = value.value;
    }
}

void Font::loadTrueType(const std::string& filename, int sizePx)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if(!file)
    {
        printFileOpenError(filename);
        return;
    }

    std::vector<unsigned char> ttfData;

    {
        auto size = file.tellg();
        ttfData.resize(size);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(ttfData.data()), size);
    }

    stbtt_fontinfo fontInfo;

    if(stbtt_InitFont(&fontInfo, ttfData.data(), 0) == 0)
    {
        std::cout << "Font: stbtt_InitFont() failed - " << filename << std::endl;
        return;
    }

    auto scale = stbtt_ScaleForPixelHeight(&fontInfo, sizePx);

    {
        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

        ascent_ = ascent * scale;
        lineHeight_ = (ascent - descent + lineGap) * scale;
    }

    std::map<int, unsigned char*> bitmaps;
    int maxBitmapSizeY = 0;
    glm::ivec2 pos(0);

    for(int i = 32; i < 127; ++i)
    {
        int id = stbtt_FindGlyphIndex(&fontInfo, i);

        if(id == 0)
        {
            std::cout << "Font: stbtt_FindGlyphIndex() failed (" << i << ") - " << filename << std::endl;
            continue;
        }

        Glyph glyph;
        int dummy;
        stbtt_GetGlyphHMetrics(&fontInfo, id, &glyph.advance, &dummy);
        glyph.advance *= scale;

        int offsetY;

        bitmaps[i] = stbtt_GetGlyphBitmap(&fontInfo, scale, scale, id, &glyph.texRect.z, &glyph.texRect.w,
                                          &glyph.offset.x, &offsetY);

        glyph.offset.y = ascent_ + offsetY;

        if(pos.x + glyph.texRect.z > TexSizeX)
        {
            pos.x  = 0;
            pos.y += maxBitmapSizeY + Offset;
            maxBitmapSizeY = 0;
        }

        glyph.texRect.x = pos.x;
        glyph.texRect.y = pos.y;

        glyphs_.emplace(i, glyph);

        pos.x += glyph.texRect.z + Offset;
        maxBitmapSizeY = std::max(maxBitmapSizeY, glyph.texRect.w);
    }

    texture_ = hppv::Texture(GL_R8, {TexSizeX, pos.y + maxBitmapSizeY});
    texture_.bind();
    const auto texSize = texture_.getSize();

    std::vector<unsigned char> vec(texSize.x * texSize.y, 0);

    GLint unpackAlignment;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // clear the texture color
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texSize.x, texSize.y, GL_RED, GL_BYTE, vec.data());

    for(auto& glyph: glyphs_)
    {
        auto texRect = glyph.second.texRect;
        const auto* bitmap = bitmaps.at(glyph.first);

        // flip the bitmap vertically, so it plays nice with the Renderer framework
        for(int j = 0; j < texRect.w; ++j)
        {
            for(int i = 0; i < texRect.z; ++i)
            {
                vec[j * texRect.z + i] = bitmap[(texRect.w - j - 1) * texRect.z + i];
            }
        }

        // convert to the OpenGL coordinate system
        texRect.y = texSize.y - (texRect.y + texRect.w);

        glTexSubImage2D(GL_TEXTURE_2D, 0, texRect.x, texRect.y, texRect.z, texRect.w,
                        GL_RED, GL_UNSIGNED_BYTE, vec.data());
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);

    for(auto& bitmap: bitmaps)
    {
        stbtt_FreeBitmap(bitmap.second, nullptr);
    }
}

} // namespace hppv
