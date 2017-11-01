#include <iostream>
#include <fstream>

#include <hppv/Font.hpp>

namespace hppv
{

struct Value
{
    std::string str;
    int value;
    std::size_t posNext;
};

Value getValue(const std::string& str, std::size_t pos, char prev = '=', char end = ' ')
{

    auto start = str.find(prev, pos) + 1;
    auto count = str.find(end, start) - start;
    auto substr = str.substr(start, count);

    return {substr, std::atoi(substr.c_str()), start + count};
}

Font::Font(const std::string& filename)
{
    std::ifstream file(filename);

    if(!file)
    {
        std::cout << "Font, could not open file: " << filename << std::endl;
        texture_ = Texture("");
        return;
    }

    std::string line;

    for(int i = 0; i < 2; ++i)
        std::getline(file, line);

    auto value = getValue(line, 0);
    lineHeight_ = value.value;

    value = getValue(line, value.posNext);
    ascent_ = value.value;

    std::getline(file, line);

    texture_ = Texture(getValue(line, 0, '"', '"').str);

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

Glyph Font::getGlyph(int code) const
{
    if(auto it = glyphs_.find(code); it != glyphs_.end())
        return it->second;
    else
        return glyphs_.find('?')->second;
}

} // namespace hppv
