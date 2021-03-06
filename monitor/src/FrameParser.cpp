// {SHANK_BOT_LICENSE_BEGIN}
/****************************************************************
****************************************************************
*
* ShankBot - Automation software for the MMORPG Tibia.
* Copyright (C) 2016-2017 Mikael Hernvall
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
* Contact:
*       mikael.hernvall@gmail.com
*
****************************************************************
****************************************************************/
// {SHANK_BOT_LICENSE_END}
///////////////////////////////////
// Internal ShankBot headers
#include "monitor/FrameParser.hpp"
#include "utility/utility.hpp"

#include "monitor/Constants.hpp"
#include "monitor/TibiaContext.hpp"
using namespace GraphicsLayer;
using namespace SharedMemoryProtocol;
using namespace sb::utility;
using namespace sb::tibiaassets;
///////////////////////////////////

///////////////////////////////////
// Qt
#include "QtGui/QImage"
#include "QtGui/QGuiApplication"
///////////////////////////////////

///////////////////////////////////
// STD C++
#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <cmath>
///////////////////////////////////


FrameParser::FrameParser(const TibiaContext& context)
: mContext(context)
, mTileCache(1 << 14)
{
}

FrameParser::Tile::Tile(size_t width, size_t height, Type type, const std::shared_ptr<GenericType>& data)
: mWidth(width)
, mHeight(height)
, mType(type)
, mData(data)
{
}

FrameParser::Tile::Tile()
{
}

FrameParser::Tile::Tile(Type type, const std::shared_ptr<GenericType>& data)
: mType(type)
, mData(data)
{
}

FrameParser::Tile::Type FrameParser::Tile::getType() const
{
    return mType;
}

size_t FrameParser::Tile::getWidth() const
{
    return mWidth;
}

size_t FrameParser::Tile::getHeight() const
{
    return mHeight;
}

void FrameParser::Tile::setWidth(size_t width)
{
    mWidth = width;
}

void FrameParser::Tile::setHeight(size_t height)
{
    mHeight = height;
}

GenericType* FrameParser::Tile::getData() const
{
    if(!mData.get())
        SB_THROW("Data is null.");

    return mData.get();
}


void FrameParser::copyGlyphs(unsigned int srcTexId, unsigned short srcX, unsigned short srcY, unsigned int tarTexId, unsigned short tarX, unsigned short tarY, unsigned short width, unsigned short height)
{
    typedef TileBufferCache<Tile>::AreaElement CachedGlyph;
    mTileCache.removeByArea(tarTexId, tarX, tarY, width, height);
    std::list<CachedGlyph> glyphs = mTileCache.getByArea(srcTexId, srcX, srcY, width, height);
    for(const CachedGlyph& glyph : glyphs)
    {
        assert(glyph.element.getType() == Tile::Type::GLYPH);
        mTileCache.set(tarTexId, tarX + glyph.x - srcX, tarY + glyph.y - srcY, glyph.element);
    }
}

unsigned int FrameParser::VertexBuffer::getVerticesOffset() const
{
    switch(vertexType)
    {
        case VertexType::TEXTURED:
        case VertexType::COLORED:
        {
            auto it = attribPointers.find(0);
            assert(it != attribPointers.end());
            return it->second.offset;
        }
        case VertexType::TEXTURED_NO_ORDER:
            return 0;
        default:
            SB_THROW("Unimplemented vertex type: ", (int)vertexType);
    }
}

unsigned int FrameParser::VertexBuffer::getOrdersOffset() const
{
    switch(vertexType)
    {
        case VertexType::TEXTURED:
        case VertexType::COLORED:
        {
            auto it = attribPointers.find(2);
            if(it == attribPointers.end())
            {
                return -1;
            }
            return it->second.offset;
        }

        default:
            SB_THROW("Unimplemented vertex type: ", (int)vertexType);
    }
}

void FrameParser::copyGlyphs(const DrawCall& drawCall)
{
    assert(drawCall.type == DrawCall::PrimitiveType::TRIANGLE_FAN);

    const VertexBuffer& buffer = mVertexBuffers[drawCall.bufferId];
    size_t bufferEnd = size_t((char*)buffer.data + buffer.numBytes);
    assert(buffer.vertexType == VertexBuffer::VertexType::TEXTURED_NO_ORDER || buffer.vertexType == VertexBuffer::VertexType::TEXTURED);

    Vertex* positions = (Vertex*)((char*)buffer.data + buffer.getVerticesOffset());
    auto texCoordsAttribPointer = buffer.attribPointers.find(1);
    assert(texCoordsAttribPointer != buffer.attribPointers.end());
    Vertex* texCoords = (Vertex*)((char*)positions + texCoordsAttribPointer->second.offset);

    assert(size_t(&texCoords[0]) < bufferEnd);
    assert(size_t(&texCoords[2]) < bufferEnd);
    const Vertex& topLeft = texCoords[0];
    const Vertex& botRight = texCoords[2];
    const Texture& srcTex = mTextures[drawCall.sourceTextureId];
    unsigned short topLeftX = topLeft.x * srcTex.width;
    unsigned short topLeftY = topLeft.y * srcTex.height;
    unsigned short width = botRight.x * srcTex.width - topLeftX;
    unsigned short height = botRight.y * srcTex.height - topLeftY;


    // NDC coords (x=[-1, 1], y=[-1, 1])
    assert(size_t(&positions[0]) < bufferEnd);
    const Vertex& offset = positions[0];
    const Texture& targetTex = mTextures[drawCall.targetTextureId];
    unsigned short offsetX = (targetTex.width / 2) * (offset.x + 1.f);
    unsigned short offsetY = (targetTex.height / 2) * (offset.y + 1.f);

    copyGlyphs(drawCall.sourceTextureId, topLeftX, topLeftY, drawCall.targetTextureId, offsetX, offsetY, width, height);
}

unsigned char FrameParser::getChar(unsigned textureId, unsigned short x, unsigned short y, unsigned short width, unsigned short height)
{
    Tile tile;
    if(!mTileCache.get(textureId, x, y, tile))
    {
        typedef TileBufferCache<Tile>::AreaElement CachedGlyph;
        unsigned short cacheX = (x < width / 2 ? 0 : x - width / 2);
        unsigned short cacheY = (y < height / 2 ? 0 : y - height / 2);
        std::list<CachedGlyph> glyphs = mTileCache.getByArea(textureId, cacheX, cacheY, width, height);
        assert(!glyphs.empty());

        if(glyphs.size() == 1)
        {
            tile = glyphs.front().element;
        }
        else
        {
            size_t minX = -1;
            const CachedGlyph* closestGlyph = nullptr;
            for(const CachedGlyph& g : glyphs)
            {
                int dX = g.x - x;
                if(dX < 0) dX = -dX;
                if(dX < minX)
                {
                    minX = dX;
                    closestGlyph = &g;
                }
            }

            assert(closestGlyph);
            tile = closestGlyph->element;
        }

        assert(tile.getType() == Tile::Type::GLYPH);
        mTileCache.set(textureId, x, y, tile);
    }

    assert(tile.getType() == Tile::Type::GLYPH);
    return GlyphData::fromTile(tile);
}

FrameParser::Tile FrameParser::getTile(unsigned int textureId, unsigned short x, unsigned short y)
{
    Tile tile;
    if(mTileCache.get(textureId, x, y, tile))
        return tile;

    bool isFound = false;
    unsigned short foundX = 0;
    unsigned short foundY = 0;
    mTileCache.forEach
    (
        textureId,
        [&isFound, &tile, y, x, &foundX, &foundY](const Tile& n, const size_t& nx, const size_t& ny)
        {
            if(x >= nx && x < nx + n.getWidth() && y >= ny)
            {
                if(y < ny + n.getHeight())
                {
                    isFound = true;
                    foundX = nx;
                    foundY = ny;
                    tile = n;
                }

                return false;
            }
            return true;
        },
        x, y,
        false
    );

    if(isFound)
    {
        assert(x >= foundX && y >= foundY);
        Tile recacheTile(tile);
        recacheTile.setWidth(tile.getWidth() - (x - foundX));
        recacheTile.setHeight(tile.getHeight() - (y - foundY));
        mTileCache.set(textureId, x, y, recacheTile);
        return tile;
    }

    return Tile();
}

FrameParser::TileNumber FrameParser::getTileNumber(const unsigned char* pixels, unsigned short width, unsigned short height, sb::utility::PixelFormat format) const
{
    static const size_t MAX_GLYPH_WIDTH = 9;
    static const size_t MIN_GLYPH_WIDTH = 8;
    static const size_t MAX_GLYPH_HEIGHT = 11;
    static const size_t MIN_GLYPH_HEIGHT = 10;

    if(width < MIN_GLYPH_WIDTH || height < MIN_GLYPH_HEIGHT || height > MAX_GLYPH_HEIGHT)
        return FrameParser::TileNumber();

    typedef sb::utility::PixelFormat Format;
    assert(format == Format::RGBA || format == Format::BGRA);
    std::unique_ptr<QImage> img;
    if(format == Format::RGBA)
        img.reset(new QImage(pixels, width, height, QImage::Format_RGBA8888));
    else
        img.reset(new QImage(QImage(pixels, width, height, QImage::Format_ARGB32).rgbSwapped()));

    const unsigned char* formattedPixels = img->constBits();

    size_t numPixelsCounted = 0;
    size_t rSum = 0;
    size_t gSum = 0;
    size_t bSum = 0;
    for(size_t i = 0; i < width * height * BYTES_PER_PIXEL_RGBA; i += BYTES_PER_PIXEL_RGBA)
    {
        if(formattedPixels[i + 3] == 255)
        {
            unsigned char r = formattedPixels[i];
            unsigned char g = formattedPixels[i + 1];
            unsigned char b = formattedPixels[i + 2];
            static const unsigned char PIXEL_SUM_THRESHOLD = 10;
            if(r + g + b >= PIXEL_SUM_THRESHOLD)
            {
                rSum += r;
                gSum += g;
                bSum += b;
                numPixelsCounted++;
            }
        }
    }
    float sum = rSum + gSum + bSum;
    float rPerc = float(rSum) / sum;
    float gPerc = float(gSum) / sum;
    float bPerc = float(bSum) / sum;

    FrameParser::TileNumber tileNumber;
    if(gPerc >= 0.95f)
    {
        tileNumber.type = FrameParser::TileNumber::Type::POISON_DAMAGE;
    }
    else if(rPerc >= 0.95f)
    {
        tileNumber.type = FrameParser::TileNumber::Type::PHYSICAL_DAMAGE;
    }
    else if(gPerc >= 0.38f && gPerc <= 0.40f && bPerc >= 0.38f && bPerc <= 0.40f)
    {
        tileNumber.type = FrameParser::TileNumber::Type::ICE_DAMAGE;
    }
    else if(rPerc >= 0.30f && rPerc <= 0.35f && gPerc >= 0.30f && gPerc <= 0.35f && bPerc >= 0.30f && bPerc <= 0.35f)
    {
        tileNumber.type = FrameParser::TileNumber::Type::XP_GAIN;
    }
    else if(rPerc >= 0.50f && rPerc <= 0.60f && gPerc >= 0.20f && gPerc <= 0.30f && bPerc >= 0.20f && bPerc <= 0.30f)
    {
        tileNumber.type = FrameParser::TileNumber::Type::HP_GAIN;
    }
    else if(rPerc >= 0.15f && rPerc <= 0.25f && gPerc >= 0.25f && gPerc <= 0.35f && bPerc >= 0.45f && bPerc <= 0.55f)
    {
        tileNumber.type = FrameParser::TileNumber::Type::MANA_GAIN;
    }
    else if(rPerc >= 0.58f&& rPerc <= 0.65f && gPerc >= 0.35f && gPerc <= 0.42f && bPerc <= 0.05f)
    {
        tileNumber.type = FrameParser::TileNumber::Type::FIRE_DAMAGE;
    }
    else if(rPerc >= 0.37f && rPerc <= 0.43f && gPerc >= 0.07f && gPerc <= 0.13f && bPerc >= 0.47f && bPerc <= 0.53f)
    {
        tileNumber.type = FrameParser::TileNumber::Type::ENERGY_DAMAGE;
    }

    if(tileNumber.type == FrameParser::TileNumber::Type::INVALID)
        return FrameParser::TileNumber();

    img.reset(new QImage(img->convertToFormat(QImage::Format_Grayscale8)));
    std::vector<unsigned char> grayPixels(width * height);
    for(size_t i = 0; i < height; i++)
        memcpy(&grayPixels[i * width], img->scanLine(i), width);

    numPixelsCounted = 0;
    sum = 0;
    static const unsigned char PIXEL_GRAY_THRESHOLD = 10;
    for(size_t i = 0; i < width * height; i++)
    {
        if(grayPixels[i] > PIXEL_GRAY_THRESHOLD)
        {
            sum += grayPixels[i];
            numPixelsCounted++;
        }
    }

    float averagePixelValue = float(sum) / float(numPixelsCounted);
    if(!std::isfinite(averagePixelValue))
        return FrameParser::TileNumber();

    static const float DESIRED_AVERAGE = 175.f;
    float modifier = DESIRED_AVERAGE / averagePixelValue;
    if(!std::isfinite(modifier))
        return FrameParser::TileNumber();

    for(size_t i = 0; i < width * height; i++)
        if(grayPixels[i] > PIXEL_GRAY_THRESHOLD)
            grayPixels[i] = std::min(int(grayPixels[i] * modifier), 255);


    if(width <= MAX_GLYPH_WIDTH)
    {
        std::list<unsigned char> topTen;
        unsigned char character = getChar(grayPixels.data(), width, height, &topTen);
        if(!isNumeric(character))
        {
            for(auto c : topTen)
            {
                if(isNumeric(c))
                {
                    character = c;
                    break;
                }
            }
        }
        if(!isNumeric(character))
            return FrameParser::TileNumber();

        tileNumber.number = character - '0';
        return tileNumber;
    }

    tileNumber.number = 0;
    unsigned int magnitude = 1;
    for(int x = width - 1;;)
    {
        if(x <= 0)
        {
            assert(x == 0);
            break;
        }

        int xRemaining = x + 1;
        size_t currWidth;
        if(xRemaining >= MAX_GLYPH_WIDTH)
        {
            currWidth = MAX_GLYPH_WIDTH;
        }
        else
        {
            assert(xRemaining == MIN_GLYPH_WIDTH);
            currWidth = MIN_GLYPH_WIDTH;
        }
        x -= currWidth - 1;
        std::vector<unsigned char> window(height * currWidth);
        for(size_t iSrc = x, iDest = 0; iSrc + currWidth <= height * width; iSrc += width, iDest += currWidth)
            memcpy(&window[iDest], &grayPixels[iSrc], currWidth);

        std::list<unsigned char> topTen;
        unsigned char character = getChar(window.data(), currWidth, height, &topTen);
        if(!isNumeric(character))
        {
            for(auto c : topTen)
            {
                if(isNumeric(c))
                {
                    character = c;
                    break;
                }
            }
        }
        if(!isNumeric(character))
            return FrameParser::TileNumber();

        unsigned int digit = character - '0';
        tileNumber.number += magnitude * (character - '0');
        magnitude *= 10;
    }


    return tileNumber;
}

unsigned char FrameParser::getChar(const unsigned char* pixels, unsigned short width, unsigned short height, std::list<unsigned char>* topTen) const
{
    size_t sum = 0;
    for(size_t i = 0; i < width * height; i++)
        sum += pixels[i];

    size_t minDiff = -1;
    std::list<std::pair<size_t, const FontSample::Glyph*>> diffs(10, {-1, nullptr});
    const FontSample::Glyph* mostSimilarGlyph = nullptr;
    for(const FontSample::Glyph& lhs : mContext.getGlyphs())
    {
        size_t diff = -1;
        #define SUM_EPSILON 2
        if(!(sum > lhs.sum * SUM_EPSILON || lhs.sum > sum * SUM_EPSILON))
        {
            if(lhs.width <= width && lhs.height <= height)
                diff = movingWindowMinDiff(lhs.data.data(), lhs.width, lhs.height, pixels, width, height, sum);
            else if(width <= lhs.width && height <= lhs.height)
                diff = movingWindowMinDiff(pixels, width, height, lhs.data.data(), lhs.width, lhs.height, lhs.sum);
        }

        if(topTen)
        {
            auto it = diffs.begin();
            while(it != diffs.end())
            {
                if(diff >= it->first)
                    break;

                it++;
            }

            if(it != diffs.begin())
            {
                diffs.insert(it, std::make_pair(diff, &lhs));
                diffs.pop_front();
            }
        }

        if(diff < minDiff)
        {

            minDiff = diff;
            mostSimilarGlyph = &lhs;
            if(minDiff == 0)
                break;
        }
    }

    if(topTen)
    {
        for(const auto& pair : diffs)
        {
            if(pair.second)
            {
                topTen->push_front(pair.second->character);
            }
        }
    }



    assert(mostSimilarGlyph);
    return mostSimilarGlyph->character;
}

void FrameParser::updateMiniMapPixels(const PixelData& pixelData, const unsigned char* pixels)
{
    assert(pixelData.format == sb::utility::PixelFormat::BGRA);

    auto it = mMiniMapBuffers.find(pixelData.targetTextureId);
    assert(it != mMiniMapBuffers.end());
    assert(it->second != nullptr);

    it->second->assign(pixels, pixels + pixelData.width * pixelData.height * BYTES_PER_PIXEL_RGBA);
}

void FrameParser::parseGlyphPixelData(const PixelData& pixelData, const unsigned char* pixels)
{
    mTileCache.removeByArea(pixelData.targetTextureId, pixelData.texX, pixelData.texY, pixelData.width, pixelData.height);


    const Texture& tex = mTextures[pixelData.targetTextureId];
    if(pixelData.width <= tex.height)
    {
        QImage* img;
        unsigned char* modifiedPixels = nullptr;
        switch(pixelData.format)
        {
            case sb::utility::PixelFormat::RGBA:
                img = new QImage(pixels, pixelData.width, pixelData.height, QImage::Format_RGBA8888);
                *img = img->convertToFormat(QImage::Format_Grayscale8);
                break;
            case sb::utility::PixelFormat::BGRA:
                modifiedPixels = bgraToRgba(pixels, pixelData.width, pixelData.height);
                img = new QImage(modifiedPixels, pixelData.width, pixelData.height, QImage::Format_RGBA8888);
                *img = img->convertToFormat(QImage::Format_Grayscale8);
                break;
            case sb::utility::PixelFormat::ALPHA:
                {
                    size_t remainder = pixelData.width % 4;
                    if(remainder == 0)
                        img = new QImage(pixels, pixelData.width, pixelData.height, QImage::Format_Grayscale8);
                    else
                    {
                        size_t paddedWidth = pixelData.width + 4 - remainder;
                        modifiedPixels = new unsigned char[paddedWidth * pixelData.height];
                        for(size_t iSrc = 0, iDest = 0; iSrc < pixelData.width * pixelData.height; iSrc += pixelData.width, iDest += paddedWidth)
                            memcpy(&modifiedPixels[iDest], &pixels[iSrc], pixelData.width);

                        img = new QImage(modifiedPixels, pixelData.width, pixelData.height, QImage::Format_Grayscale8);
                    }
                }
                break;

            default:
                SB_THROW("Wot");

        }

        if(tex.height == 68)
        {
            int argc = 0;
            QGuiApplication app(argc, nullptr);
            *img = img->scaledToWidth(16);
        }

        const size_t ROW_SIZE = img->width() * BYTES_PER_PIXEL_ALPHA;
        const size_t GLYPH_SIZE = ROW_SIZE * img->height();
        std::vector<unsigned char> formattedPixels(GLYPH_SIZE);
        for(size_t y = 0; y < img->height(); y++)
        {
            const unsigned char* row = img->constScanLine(y);
            memcpy(formattedPixels.data() + y * ROW_SIZE, row, ROW_SIZE);
        }

        unsigned char character = getChar(formattedPixels.data(), img->width(), img->height());
        Tile tile = GlyphData::createTile(pixelData.width, pixelData.height, character);
        mTileCache.set(pixelData.targetTextureId, pixelData.texX, pixelData.texY, tile);

        if(modifiedPixels)
            delete[] modifiedPixels;

        delete img;
    }
}

void FrameParser::parsePixelData(const PixelData& pixelData, const unsigned char* pixels)
{
    if(pixelData.messageType == Message::MessageType::SCREEN_PIXELS)
    {
        mCurrentFrame.screenPixels.reset(new RawImage(pixelData.format, pixelData.width, pixelData.height, pixels));
        return;
    }

    if(pixelData.targetTextureId == mTileBufferId)
    {
        updateTileBuffer(pixelData, pixels);
        return;
    }

    if(mMiniMapBuffers.find(pixelData.targetTextureId) != mMiniMapBuffers.end())
    {
        updateMiniMapPixels(pixelData, pixels);
        return;
    }

    if(mGlyphBufferIds.find(pixelData.targetTextureId) != mGlyphBufferIds.end())
    {
        parseGlyphPixelData(pixelData, pixels);
        return;
    }
}

void FrameParser::parseCopyTexture(const CopyTexture& copy)
{
    if(mGlyphBufferIds.find(copy.sourceTextureId) != mGlyphBufferIds.end())
    {
        if(mGlyphBufferIds.find(copy.targetTextureId) == mGlyphBufferIds.end())
        {
            SB_THROW("Glyph buffer copied to screen buffer. This should not happen.");
        }
        copyGlyphs(copy.sourceTextureId, copy.srcX, copy.srcY, copy.targetTextureId, copy.targetX, copy.targetY, copy.width, copy.height);
    }
}

void FrameParser::parseTextureData(const TextureData& textureData)
{
    mTileCache.clear(textureData.id);
    Texture& tex = mTextures[textureData.id];

    if(textureData.width == Constants::MINI_MAP_PIXEL_WIDTH && textureData.height == Constants::MINI_MAP_PIXEL_HEIGHT)
        mMiniMapBuffers[textureData.id].reset(new std::vector<unsigned char>());
    else
        mMiniMapBuffers.erase(textureData.id);

    tex.width = textureData.width;
    tex.height = textureData.height;

    if(textureData.width == 4096 && textureData.height == 4096)
        mTileBufferId = textureData.id;

    if(textureData.height == 16 || textureData.height == 68)
        mGlyphBufferIds.insert(textureData.id);
    else
        mGlyphBufferIds.erase(textureData.id);
}

void FrameParser::parseVertexBufferWrite(const VertexBufferWrite& bufferWrite, const char* bufferData)
{
    VertexBuffer& buffer = mVertexBuffers[bufferWrite.bufferId];
    if(buffer.numBytes != bufferWrite.numBytes)
    {
        if(buffer.data != nullptr)
            delete[] buffer.data;

        buffer.data = malloc(bufferWrite.numBytes);
    }

    memcpy(buffer.data, bufferData, bufferWrite.numBytes);
    buffer.numBytes = bufferWrite.numBytes;
}

void FrameParser::parseVertexAttribPointer(const VertexAttribPointer& attrib)
{
    VertexBuffer& buffer = mVertexBuffers[attrib.bufferId];
    buffer.attribPointers[attrib.index] = attrib;

    typedef VertexBuffer::VertexType Type;
    if(attrib.index == 0)
    {
        switch(attrib.stride)
        {
            case 0:
                buffer.vertexType = Type::TEXTURED_NO_ORDER;
                break;
            case sizeof(ColoredVertex):
                buffer.vertexType = Type::COLORED;
                break;
            case sizeof(TexturedVertex):
                buffer.vertexType = Type::TEXTURED;
                break;
            case 20:
                buffer.vertexType = Type::TEXT;
                break;
            default:
                SB_THROW("Unimplemented stride value:  ", attrib.stride, ".");
        }
    }
}

void FrameParser::parseGlyphDraw(const DrawCall& drawCall)
{
    if(drawCall.targetTextureId != 0)
    {
        if(mGlyphBufferIds.find(drawCall.targetTextureId) == mGlyphBufferIds.end())
        {
            SB_THROW("Target of glyph buffer draw copy is not a glyph buffer.");
        }
        copyGlyphs(drawCall);
        return;
    }

    assert(drawCall.type == DrawCall::PrimitiveType::TRIANGLE || drawCall.type == DrawCall::PrimitiveType::TRIANGLE_STRIP);

    const ShaderProgram& program = mShaderPrograms[drawCall.programId];
    TextDraw d;
    d.isOutlined = (program.uniform4fs.find(1) != program.uniform4fs.end() && program.uniform4fs.find(3) != program.uniform4fs.end());
    if(d.isOutlined)
    {
        const float* color = program.uniform4fs.find(1)->second;
        d.color = Color(color[0] * 255, color[1] * 255, color[2] * 255, color[3] * 255);
    }
    else
        d.color = drawCall.blendColor;
    d.transform = std::make_shared<Matrix<float, 4, 4>>(program.transform);
    d.glyphDraws = std::make_shared<std::vector<GlyphDraw>>();
    const unsigned short HALF_FRAME_WIDTH = mCurrentFrame.width / 2;
    const unsigned short HALF_FRAME_HEIGHT = mCurrentFrame.height / 2;

    if(drawCall.type == DrawCall::PrimitiveType::TRIANGLE)
    {
        const VertexBuffer& buffer = mVertexBuffers[drawCall.bufferId];
        assert(buffer.vertexType == VertexBuffer::VertexType::TEXTURED);

        TexturedVertex* vertices = (TexturedVertex*)((char*)buffer.data + buffer.getVerticesOffset());
        VertexAttribPointer::Order* orders = nullptr;
        if(drawCall.enabledVaos & (1 << 2) && buffer.getOrdersOffset() != -1)
        {
            orders = (VertexAttribPointer::Order*)((char*)buffer.data + buffer.getOrdersOffset());
        }
        size_t bufferEnd = size_t((char*)buffer.data + buffer.numBytes);
        const bool hasOrder = orders != nullptr;
        VertexAttribPointer::Index* indices = (VertexAttribPointer::Index*)((char*)buffer.data + drawCall.indicesOffset);
        const size_t numDraws = drawCall.numIndices / 6;
        d.glyphDraws->reserve(d.glyphDraws->capacity() + numDraws);
        for(size_t i = 0; i + 5 < drawCall.numIndices; i += 6)
        {
            assert(size_t(&indices[i]) < bufferEnd);
            assert(size_t(&indices[i + 2]) < bufferEnd);
            assert(size_t(&vertices[indices[i]]) < bufferEnd);
            assert(size_t(&vertices[indices[i + 2]]) < bufferEnd);
            const TexturedVertex& topLeft = vertices[indices[i]];
            const TexturedVertex& botRight = vertices[indices[i + 2]];

            GlyphDraw g;
            g.hasOrder = hasOrder;
            if(hasOrder)
            {
                assert(size_t(&orders[indices[i]]) < bufferEnd);
            }
            g.order = hasOrder ? orders[indices[i]] : -1337.f;
            g.drawCallId = mDrawCallId;
            g.isDepthTestEnabled = drawCall.isDepthTestEnabled;
            g.isDepthWriteEnabled = drawCall.isDepthWriteEnabled;
            g.topLeft.x = topLeft.x;
            g.topLeft.y = topLeft.y;
            g.botRight.x = botRight.x;
            g.botRight.y = botRight.y;
            g.character = getChar(drawCall.sourceTextureId, topLeft.texX, topLeft.texY, botRight.texX - topLeft.texX, botRight.texY - topLeft.texY);
            d.glyphDraws->push_back(g);
        }

        d.hasOrder = hasOrder;
        d.order = d.glyphDraws->empty() ? 0.f : d.glyphDraws->front().order;
        d.drawCallId = mDrawCallId;
        d.isDepthTestEnabled = drawCall.isDepthTestEnabled;
        d.isDepthWriteEnabled = drawCall.isDepthWriteEnabled;

        mDrawCallId++;
        mCurrentFrame.textDraws->push_back(d);
    }
    else // if TRIANGLE_STRIP
    {
        // Don't do anything. It's the same area being painted over and over
        // and it is only one triangle strip primitive. It might be some
        // whitespace at the start of each line in the chat. Anyway,
        // nothing interesting.
        const VertexBuffer& buffer = mVertexBuffers[drawCall.bufferId];
        assert(buffer.vertexType == VertexBuffer::VertexType::TEXTURED);
    }
}

void FrameParser::parseRectDraw(const DrawCall& drawCall)
{
    const VertexBuffer& buffer = mVertexBuffers[drawCall.bufferId];
    size_t bufferEnd = size_t((char*)buffer.data + buffer.numBytes);

    ColoredVertex* vertices = (ColoredVertex*)((char*)buffer.data + buffer.getVerticesOffset());
    VertexAttribPointer::Order* orders = (VertexAttribPointer::Order*)((char*)buffer.data + buffer.getOrdersOffset());
    assert(drawCall.enabledVaos & (1 << 2) && buffer.getOrdersOffset() != -1);
    VertexAttribPointer::Index* indices = (VertexAttribPointer::Index*)((char*)buffer.data + drawCall.indicesOffset);
    const ShaderProgram& program = mShaderPrograms[drawCall.programId];
    std::shared_ptr<Matrix<float, 4, 4>> transform = std::make_shared<Matrix<float, 4, 4>>(program.transform);
    if(drawCall.targetTextureId == 0)
    {
        if(drawCall.type == DrawCall::PrimitiveType::TRIANGLE_STRIP)
        {
            VertexAttribPointer::Index prevIndex = -1;
            for(size_t i = 0; i < drawCall.numIndices;)
            {
                float left = std::numeric_limits<float>::max();
                float right = std::numeric_limits<float>::min();
                float top = std::numeric_limits<float>::max();
                float bot = std::numeric_limits<float>::min();

                assert(size_t(&indices[i]) < bufferEnd);
                assert(size_t(&vertices[indices[i]]) < bufferEnd);
                const ColoredVertex& firstV = vertices[indices[i]];
                Color color(firstV.r, firstV.g, firstV.b, firstV.a);
                while(i < drawCall.numIndices && indices[i] != prevIndex)
                {
                    assert(size_t(&indices[i]) < bufferEnd);
                    assert(size_t(&vertices[indices[i]]) < bufferEnd);
                    const ColoredVertex& v = vertices[indices[i]];
                    if(color.packed != Color(v.r, v.g, v.b, v.a).packed)
                    {
                        assert(i >= 4);
                        i -= 2;
                        break;
                    }
                    if(v.x < left)
                        left = v.x;
                    else if(v.x > right)
                        right = v.x;

                    if(v.y < top)
                        top = v.y;
                    else if(v.y > bot)
                        bot = v.y;

                    prevIndex = indices[i];
                    i++;
                }

                mCurrentFrame.rectDraws->emplace_back();
                RectDraw& r = mCurrentFrame.rectDraws->back();
                r.topLeft.x = left;
                r.topLeft.y = top;
                r.botRight.x = right;
                r.botRight.y = bot;
                r.transform = transform;
                r.color = color;
                r.hasOrder = true;
                assert(size_t(&orders[indices[i]]) < bufferEnd);
                r.order = orders[indices[i]];
                r.drawCallId = mDrawCallId;
                r.isDepthTestEnabled = drawCall.isDepthTestEnabled;
                r.isDepthWriteEnabled = drawCall.isDepthWriteEnabled;

                i += 2;
            }
            mDrawCallId++;
        }

    }
}

void FrameParser::parseSpriteDraw(const DrawCall& drawCall)
{
    assert(drawCall.type == DrawCall::PrimitiveType::TRIANGLE_STRIP);
    mUnshadedViewBufferId = drawCall.targetTextureId;

    const VertexBuffer& buffer = mVertexBuffers[drawCall.bufferId];
    size_t bufferEnd = size_t((char*)buffer.data + buffer.numBytes);
    TexturedVertex* vertices = (TexturedVertex*)((char*)buffer.data + buffer.getVerticesOffset());
    VertexAttribPointer::Order* orders = (VertexAttribPointer::Order*)((char*)buffer.data + buffer.getOrdersOffset());
    assert(drawCall.enabledVaos & (1 << 2) && buffer.getOrdersOffset() != -1);
    VertexAttribPointer::Index* indices = (VertexAttribPointer::Index*)((char*)buffer.data + drawCall.indicesOffset);
    const Texture& tex = mTextures[drawCall.sourceTextureId];
    const size_t numDraws = drawCall.numIndices / 6;
    mCurrentFrame.spriteDraws->reserve(mCurrentFrame.spriteDraws->capacity() + numDraws);

    for(size_t i = 0; i < drawCall.numIndices; i += 6)
    {
        assert(size_t(&indices[i]) < bufferEnd);
        assert(size_t(&vertices[indices[i]]) < bufferEnd);
        const TexturedVertex& topLeft = vertices[indices[i]];

        size_t texX = topLeft.texX * tex.width;
        size_t texY = topLeft.texY * tex.height;
        Tile tile = getTile(drawCall.sourceTextureId, texX, texY);

        switch(tile.getType())
        {
            case Tile::Type::BLANK:
                break;

            case Tile::Type::SPRITE_OBJECT_PAIRINGS:
            {
                SpriteDraw draw;
                draw.drawCallId = mDrawCallId;
                draw.topLeft.x = topLeft.x;
                draw.topLeft.y = topLeft.y;
                draw.hasOrder = true;
                assert(size_t(&orders[indices[i]]) < bufferEnd);
                draw.order = orders[indices[i]];
                draw.isDepthTestEnabled = drawCall.isDepthTestEnabled;
                draw.isDepthWriteEnabled = drawCall.isDepthWriteEnabled;
                draw.pairings = SpriteObjectPairingsData::fromTile(tile);
                mCurrentFrame.spriteDraws->push_back(draw);
                break;
            }

            case Tile::Type::COMBAT_SQUARE:
            {
                CombatSquareSample::CombatSquare::Type type = CombatSquareData::fromTile(tile);
                break;
            }

            default:
                break;
        }
    }
    mDrawCallId++;
}

void FrameParser::parseGuiTileDraw(const DrawCall& drawCall)
{
    assert(drawCall.type == DrawCall::PrimitiveType::TRIANGLE || drawCall.type == DrawCall::PrimitiveType::TRIANGLE_STRIP);

    const VertexBuffer& buffer = mVertexBuffers[drawCall.bufferId];
    size_t bufferEnd = size_t((char*)buffer.data + buffer.numBytes);

    TexturedVertex* vertices = (TexturedVertex*)((char*)buffer.data + buffer.getVerticesOffset());
    VertexAttribPointer::Order* orders = (VertexAttribPointer::Order*)((char*)buffer.data + buffer.getOrdersOffset());
    assert(drawCall.enabledVaos & (1 << 2) && buffer.getOrdersOffset() != -1);
    VertexAttribPointer::Index* indices = (VertexAttribPointer::Index*)((char*)buffer.data + drawCall.indicesOffset);
    const Texture& tex = mTextures[drawCall.sourceTextureId];
    const unsigned short HALF_FRAME_WIDTH = mCurrentFrame.width / 2;
    const unsigned short HALF_FRAME_HEIGHT = mCurrentFrame.height / 2;
    const ShaderProgram& program = mShaderPrograms[drawCall.programId];
    const size_t numDraws = drawCall.numIndices / 6;
    mCurrentFrame.guiDraws->reserve(mCurrentFrame.guiDraws->capacity() + numDraws);
    std::shared_ptr<Matrix<float, 4, 4>> transform = std::make_shared<Matrix<float, 4, 4>>(program.transform);

    const size_t BOT_RIGHT_OFFSET = (drawCall.type == DrawCall::PrimitiveType::TRIANGLE ? 2 : 3);
    for(size_t i = 0; i < drawCall.numIndices; i += 6)
    {
        assert(i + BOT_RIGHT_OFFSET < drawCall.numIndices);
        assert(size_t(&indices[i]) < bufferEnd);
        assert(size_t(&indices[i + BOT_RIGHT_OFFSET]) < bufferEnd);
        assert(size_t(&vertices[indices[i]]) < bufferEnd);
        assert(size_t(&vertices[indices[i + BOT_RIGHT_OFFSET]]) < bufferEnd);
        TexturedVertex topLeft = vertices[indices[i]];
        TexturedVertex botRight = vertices[indices[i + BOT_RIGHT_OFFSET]];

        if(botRight.x < topLeft.x)
        {
            std::swap(topLeft, botRight);
        }

        assert(topLeft.x < botRight.x);
        assert(topLeft.y < botRight.y);
        size_t texX = std::min(topLeft.texX * tex.width, botRight.texX * tex.width);
        size_t texY = std::min(topLeft.texY * tex.height, botRight.texY * tex.height);

        Tile tile = getTile(drawCall.sourceTextureId, texX, texY);
        if(tile.getType() == Tile::Type::INVALID)
        {
            Vertex screenCoords;
            worldToScreenCoords(topLeft.x, topLeft.y, *transform, HALF_FRAME_WIDTH, HALF_FRAME_HEIGHT, screenCoords.x, screenCoords.y);

            size_t width = std::max(int(texX - topLeft.texX * tex.width), int(texX - botRight.texX * tex.width)) * 2;
            size_t height = std::max(int(texY - topLeft.texY * tex.height), int(texY - botRight.texY * tex.height)) * 2;
            size_t x = texX - width / 2;
            size_t y = texY - height / 2;
            if(x > texX)
            {
                x = 0;
            }
            if(y > texY)
            {
                y = 0;
            }
            auto area = mTileCache.getByArea(drawCall.sourceTextureId, x, y, width, height);

            std::stringstream sstream;
            for(const auto& e : area)
            {
                sstream << "\t" << e.x << "x" << e.y << " "
                                << e.element.getWidth() << "x" << e.element.getHeight()
                                << " (" << (int)e.element.getType() << ")" << std::endl;
            }


            SB_THROW("Cannot find GUI tile draw in any tilebuffer.", "\n",
                     "\tTexture ID: ", drawCall.sourceTextureId, " -> ", drawCall.targetTextureId, "\n",
                     "\tTexCoords: ", texX, "x", texY, "\n",
                     "\tTexSize: ", botRight.texX * tex.width - texX, "x", botRight.texY * tex.height - texY, "\n",
                     "\tDrawCoords: ", screenCoords.x, "x", screenCoords.y, "\n"
                     "Printing nearest cached tiles below:\n",
                     sstream.str(), "\n");
        }

        switch(tile.getType())
        {
            case Tile::Type::SPRITE_OBJECT_PAIRINGS:
            {
                SpriteDraw draw;
                draw.hasOrder = true;
                assert(size_t(&orders[indices[i]]) < bufferEnd);
                draw.order = orders[indices[i]];
                draw.drawCallId = mDrawCallId;
                draw.topLeft.x = topLeft.x;
                draw.topLeft.y = topLeft.y;
                draw.botRight.x = botRight.x;
                draw.botRight.y = botRight.y;
                draw.pairings = SpriteObjectPairingsData::fromTile(tile);
                draw.transform = transform;
                draw.isDepthTestEnabled = drawCall.isDepthTestEnabled;
                draw.isDepthWriteEnabled = drawCall.isDepthWriteEnabled;
                mCurrentFrame.guiSpriteDraws->push_back(draw);
                break;
            }
            case Tile::Type::GRAPHICS_RESOURCE_NAMES:
            {
                GuiDraw d;
                d.hasOrder = true;
                assert(size_t(&orders[indices[i]]) < bufferEnd);
                d.order = orders[indices[i]];
                d.drawCallId = mDrawCallId;
                d.topLeft.x = topLeft.x;
                d.topLeft.y = topLeft.y;
                d.botRight.x = botRight.x;
                d.botRight.y = botRight.y;
                d.name = GraphicsResourceNamesData::fromTile(tile).front();
                d.transform = transform;
                d.isDepthTestEnabled = drawCall.isDepthTestEnabled;
                d.isDepthWriteEnabled = drawCall.isDepthWriteEnabled;
                mCurrentFrame.guiDraws->push_back(d);
                break;
            }
            case Tile::Type::TILE_NUMBER:
            {
                const TileNumber& n = TileNumberData::fromTile(tile);
                break;
            }

            case Tile::Type::BLANK:
                break;

            default:
                SB_THROW("Invalid tile type: ", (int)tile.getType());
        }


    }
    mDrawCallId++;
}

void FrameParser::parseTileDraw(const DrawCall& drawCall)
{
    const VertexBuffer& buffer = mVertexBuffers[drawCall.bufferId];
    assert(buffer.vertexType == VertexBuffer::VertexType::TEXTURED);

    if(drawCall.targetTextureId != 0)
        parseSpriteDraw(drawCall);
    else
        parseGuiTileDraw(drawCall);
}

void FrameParser::parseUnshadedViewDraw(const DrawCall& drawCall)
{
    if(drawCall.targetTextureId != 0)
        SB_THROW("Light effects are enabled. They should not be enabled.");

    assert(drawCall.type == DrawCall::PrimitiveType::TRIANGLE_STRIP);
    assert(drawCall.targetTextureId == 0);

    const VertexBuffer& buffer = mVertexBuffers[drawCall.bufferId];
    size_t bufferEnd = size_t((char*)buffer.data + buffer.numBytes);
    assert(buffer.vertexType == VertexBuffer::VertexType::TEXTURED);

    assert(drawCall.numIndices == 4);

    TexturedVertex* vertices = (TexturedVertex*)((char*)buffer.data + buffer.getVerticesOffset());
    VertexAttribPointer::Index* indices = (VertexAttribPointer::Index*)((char*)buffer.data + drawCall.indicesOffset);

    assert(size_t(&indices[0]) < bufferEnd);
    assert(size_t(&vertices[indices[0]]) < bufferEnd);
    const TexturedVertex& topLeft = vertices[indices[0]];

    worldToScreenCoords
    (
        topLeft.x, topLeft.y,
        mShaderPrograms[drawCall.programId].transform,
        float(mCurrentFrame.width) / 2.f, float(mCurrentFrame.height) / 2.f,
        mCurrentFrame.viewX, mCurrentFrame.viewY
    );

    assert(size_t(&indices[4]) < bufferEnd);
    assert(size_t(&vertices[indices[4]]) < bufferEnd);
    const TexturedVertex& botRight = vertices[indices[4]];
    mCurrentFrame.viewWidth = botRight.x;
    mCurrentFrame.viewHeight = botRight.y;
}

void FrameParser::parseMiniMapDraw(const DrawCall& drawCall)
{
    assert(drawCall.type == DrawCall::PrimitiveType::TRIANGLE_STRIP);
    if(drawCall.targetTextureId != 0)
    {
        const Texture& tex = mTextures[drawCall.targetTextureId];
        std::cout << drawCall.targetTextureId << " " << tex.width << "x" << tex.height << std::endl;

        assert(tex.width == 480 && tex.height == 352);
        // TODO: Handle this
        return;
    }

    const VertexBuffer& buffer = mVertexBuffers[drawCall.bufferId];
    size_t bufferEnd = size_t((char*)buffer.data + buffer.numBytes);
    assert(buffer.vertexType == VertexBuffer::VertexType::TEXTURED);
    assert(drawCall.numIndices == 4);

    TexturedVertex* vertices = (TexturedVertex*)((char*)buffer.data + buffer.getVerticesOffset());
    VertexAttribPointer::Index* indices = (VertexAttribPointer::Index*)((char*)buffer.data + drawCall.indicesOffset);
    assert(size_t(&indices[0]) < bufferEnd);
    assert(size_t(&vertices[indices[0]]) < bufferEnd);
    const TexturedVertex& topLeft = vertices[indices[0]];
    mCurrentFrame.hasMiniMapMoved = true;
    mCurrentFrame.miniMapX = topLeft.x;
    mCurrentFrame.miniMapY = topLeft.y;

    worldToScreenCoords
    (
        topLeft.x, topLeft.y,
        mShaderPrograms[drawCall.programId].transform,
        float(mCurrentFrame.width) / 2.f, float(mCurrentFrame.height) / 2.f,
        mCurrentFrame.miniMapScreenX, mCurrentFrame.miniMapScreenY
    );

    assert(size_t(&indices[3]) < bufferEnd);
    assert(size_t(&vertices[indices[3]]) < bufferEnd);
    const TexturedVertex& botRight = vertices[indices[3]];
    Vertex screenBotRight;
    worldToScreenCoords
    (
        botRight.x, botRight.y,
        mShaderPrograms[drawCall.programId].transform,
        float(mCurrentFrame.width) / 2.f, float(mCurrentFrame.height) / 2.f,
        screenBotRight.x, screenBotRight.y
    );

    mCurrentFrame.miniMapScreenWidth = screenBotRight.x - mCurrentFrame.miniMapScreenX;
    mCurrentFrame.miniMapScreenHeight = screenBotRight.y - mCurrentFrame.miniMapScreenY;

    MiniMapDraw d;
    d.drawCallId;
    d.topLeft.x = topLeft.x;
    d.topLeft.y = topLeft.y;
    d.botRight.x = botRight.x;
    d.botRight.y = botRight.y;
    d.transform = std::make_shared<Matrix<float, 4, 4>>(mShaderPrograms[drawCall.programId].transform);

    auto pixelsIt = mMiniMapBuffers.find(drawCall.sourceTextureId);
    assert(pixelsIt != mMiniMapBuffers.end());
    d.pixels = pixelsIt->second;

    mCurrentFrame.miniMapDraws->push_back(d);
    mDrawCallId++;
}

void FrameParser::parseDrawCall(const DrawCall& drawCall)
{
    if(mVertexBuffers[drawCall.bufferId].vertexType == VertexBuffer::VertexType::COLORED)
    {
        parseRectDraw(drawCall);
        return;
    }

    if(mGlyphBufferIds.find(drawCall.sourceTextureId) != mGlyphBufferIds.end())
    {
        parseGlyphDraw(drawCall);
        return;
    }

    if(drawCall.sourceTextureId == mTileBufferId)
    {
        parseTileDraw(drawCall);
        return;
    }

    if(drawCall.sourceTextureId == mUnshadedViewBufferId)
    {
        parseUnshadedViewDraw(drawCall);
        return;
    }

    if(mMiniMapBuffers.find(drawCall.sourceTextureId) != mMiniMapBuffers.end())
    {
        parseMiniMapDraw(drawCall);
        return;
    }
}

void FrameParser::parseTransformationMatrix(const TransformationMatrix& transform)
{
    ShaderProgram& program = mShaderPrograms[transform.programId];
    memcpy(&program.transform, &transform.matrix, sizeof(transform.matrix));
}

void FrameParser::parseUniform4f(const Uniform4f& uniform)
{
    ShaderProgram& program = mShaderPrograms[uniform.programId];
    memcpy(program.uniform4fs[uniform.location], uniform.values, sizeof(uniform.values));
}

size_t numGlyphs = 0;
size_t frameId = 0;
std::list<GraphicsLayer::Frame> FrameParser::parse(const SharedMemoryProtocol::SharedMemorySegment* segment)
{
    std::list<Frame> frames;
    const char* DATA_END = segment->data + segment->size;
    for(const char* data = segment->data; data < DATA_END;)
    {
        const SharedMemoryProtocol::Frame& frame = *(SharedMemoryProtocol::Frame*)(data);
        mCurrentFrame = Frame();
        mCurrentFrame.guiDraws = std::make_shared<std::vector<GuiDraw>>();
        mCurrentFrame.spriteDraws = std::make_shared<std::vector<SpriteDraw>>();
        mCurrentFrame.textDraws = std::make_shared<std::vector<TextDraw>>();
        mCurrentFrame.rectDraws = std::make_shared<std::vector<RectDraw>>();
        mCurrentFrame.guiSpriteDraws = std::make_shared<std::vector<SpriteDraw>>();
        mCurrentFrame.fileIo = std::make_shared<std::vector<FileIo>>();
        mCurrentFrame.miniMapDraws = std::make_shared<std::vector<MiniMapDraw>>();
        mDrawCallId = 0;

        mCurrentFrame.width = frame.width;
        mCurrentFrame.height = frame.height;

        const char* FRAME_END = data + frame.size;
        data += sizeof(frame);
        while(data < FRAME_END)
        {
            const Message& message = *(Message*)(data);
            typedef Message::MessageType Type;
            switch(message.messageType)
            {
                case Type::PIXEL_DATA:
                case Type::SCREEN_PIXELS:
                {
                    const PixelData& pixelData = *(PixelData*)(data);
                    data += sizeof(pixelData);

                    unsigned char* pixels = (unsigned char*)data;
                    size_t pixelsSize = pixelData.width * pixelData.height * getBytesPerPixel(pixelData.format);

                    data += pixelsSize;
                    parsePixelData(pixelData, pixels);
                    break;
                }
                case Type::COPY_TEXTURE:
                {
                    const CopyTexture& copy = *(CopyTexture*)(data);
                    data += sizeof(copy);

                    parseCopyTexture(copy);
                    break;
                }

                case Type::TEXTURE_DATA:
                {
                    const TextureData& textureData = *(TextureData*)(data);
                    data += sizeof(textureData);

                    parseTextureData(textureData);
                    break;
                }

                case Type::VERTEX_BUFFER_WRITE:
                {
                    const VertexBufferWrite& bufferWrite = *(VertexBufferWrite*)(data);
                    data += sizeof(bufferWrite);

                    const char* bufferData = data;
                    data += bufferWrite.numBytes;

                    parseVertexBufferWrite(bufferWrite, bufferData);
                    break;
                }

                case Type::VERTEX_ATTRIB_POINTER:
                {
                    const VertexAttribPointer& attrib = *(VertexAttribPointer*)(data);
                    data += sizeof(attrib);

                    parseVertexAttribPointer(attrib);
                    break;
                }

                case Type::DRAW_CALL:
                {
                    const DrawCall& drawCall = *(DrawCall*)(data);
                    data += sizeof(drawCall);

                    parseDrawCall(drawCall);
                    break;
                }

                case Type::TRANSFORMATION_MATRIX:
                {
                    const TransformationMatrix& transform = *(TransformationMatrix*)(data);
                    data += sizeof(transform);

                    parseTransformationMatrix(transform);
                    break;
                }

                case Type::UNIFORM_4_F:
                {
                    const Uniform4f& uniform = *(Uniform4f*)(data);
                    data += sizeof(uniform);

                    parseUniform4f(uniform);
                    break;
                }

                case Type::FILE_IO:
                {
                    const SharedMemoryProtocol::FileIo& io = *(SharedMemoryProtocol::FileIo*)(data);
                    data += sizeof(io);

                    const char* pathData = data;
                    data += io.pathSize;

                    parseFileIo(io, pathData);
                    break;
                }

                default:
                    SB_THROW("Unimplemented message type: ", (int)message.messageType);
            }
        }
        assert(data == FRAME_END);
        frames.push_back(std::move(mCurrentFrame));
        mCurrentFrame = Frame();
    }

    return frames;
}

void FrameParser::parseFileIo(const SharedMemoryProtocol::FileIo& io, const char* data) const
{
    FileIo f;
    f.path.assign(data, io.pathSize);
    switch(io.type)
    {
        case SharedMemoryProtocol::FileIo::Type::READ:
            f.type = FileIo::Type::READ;
            break;

        case SharedMemoryProtocol::FileIo::Type::WRITE:
            f.type = FileIo::Type::WRITE;
            break;

        default:
            SB_THROW("Unimplemented FileIo type: ", (int)io.type);
    }

    mCurrentFrame.fileIo->push_back(f);
}

void FrameParser::updateTileBuffer(const PixelData& data, const unsigned char* pixels)
{
    typedef sb::utility::PixelFormat Format;
    std::vector<size_t> opaquePixels;
    bool isBlank = false;
    switch(data.format)
    {
        case Format::RGBA:
            opaquePixels = rgbaToColorTreeSprite(pixels, data.width, data.height, &isBlank);
            break;

        case Format::BGRA:
            opaquePixels = bgraToColorTreeSprite(pixels, data.width, data.height, &isBlank);
            break;

        case Format::ALPHA:
            return; // No reason to care about alpha.

        default:
            SB_THROW("Unimplemented pixel format for color tree.");
    }



    if(isBlank)
    {
        mTileCache.removeByArea(data.targetTextureId, data.texX, data.texY, data.width, data.height);
        Tile tile(data.width, data.height, Tile::Type::BLANK);
        mTileCache.set(data.targetTextureId, data.texX, data.texY, tile);
        return;
    }

    std::vector<size_t> transparency;
    switch(data.format)
    {
        case Format::RGBA:
        case Format::BGRA:
            // The alpha value of both formats are in the same place, which is the only
            // thing rgbaToTransparencyTreeSprite cares about. So it's OK to use it
            // for BGRA as well as RGBA.
            transparency = rgbaToTransparencyTreeSprite(pixels, data.width, data.height);
            break;

        case Format::ALPHA:
            return; // No reason to care about alpha.

        default:
            SB_THROW("Unimplemented pixel format for transparency tree.");
    }

    std::list<size_t> ids;
    if(mContext.getSpriteColorTree().find(opaquePixels, ids))
    {
        std::list<size_t> tIds;
        mContext.getSpriteTransparencyTree().find(transparency, tIds);

        std::list<size_t> matchingIds;
        for(size_t id : ids)
            if(std::find(tIds.begin(), tIds.end(), id) != tIds.end())
                matchingIds.push_back(id);

        if(matchingIds.empty())
            return;

        ids = matchingIds;
        ids.sort();
        ids.unique();
        size_t width = 0;
        size_t height = 0;
        std::list<SpriteDraw::SpriteObjectPairing> pairings;
        std::list<std::string> graphicsResourceNames;
        std::list<CombatSquareSample::CombatSquare::Type> combatSquares;
        for(size_t spriteId : ids)
        {
            if(spriteId >= Constants::SPRITE_ID_START && spriteId <= Constants::SPRITE_ID_END)
            {
                SpriteDraw::SpriteObjectPairing pairing;
                pairing.spriteId = spriteId;
                std::list<size_t> objects = mContext.getSpriteObjectBindings().getObjects(spriteId);
                if(!objects.empty())
                {
                    pairing.objects.swap(objects);
                    width = data.width;
                    height = data.height;
                    pairings.push_back(pairing);
                }
            }
            else if(spriteId >= Constants::GRAPHICS_RESOURCE_ID_START && spriteId <= Constants::GRAPHICS_RESOURCE_ID_END)
            {
                graphicsResourceNames.push_back(mContext.getGraphicsResourceNames()[spriteId - Constants::GRAPHICS_RESOURCE_ID_START]);
                width = data.width;
                height = data.height;
            }
            else if(spriteId >= Constants::COMBAT_SQUARE_ID_START && spriteId <= Constants::COMBAT_SQUARE_ID_END)
            {
                combatSquares.push_back(CombatSquareSample::getType(spriteId - Constants::COMBAT_SQUARE_ID_START));
                width = data.width;
                height = data.height;
            }
        }

        assert(!graphicsResourceNames.empty() + !pairings.empty() + !combatSquares.empty() <= 1);

        Tile tile;
        if(!graphicsResourceNames.empty())
        {
            assert(graphicsResourceNames.size() == 1);
            tile = GraphicsResourceNamesData::createTile(data.width, data.height, graphicsResourceNames);
        }
        else if(!pairings.empty())
        {
            tile = SpriteObjectPairingsData::createTile(data.width, data.height, pairings);
        }
        else if(!combatSquares.empty())
        {
            assert(combatSquares.size() == 1);
            tile = CombatSquareData::createTile(data.width, data.height, combatSquares.front());
        }
        else
            SB_THROW("This shouldn't happen. :-)");

        mTileCache.removeByArea(data.targetTextureId, data.texX, data.texY, data.width, data.height);
        mTileCache.set(data.targetTextureId, data.texX, data.texY, tile);
    }
    else
    {
        ids.clear();
        static size_t failCount = 0;
        if(!mContext.getSpriteTransparencyTree().find(transparency, ids))
        {
            bool isRecFail = true;

            QImage::Format f = data.format == Format::RGBA ? QImage::Format_RGBA8888 : QImage::Format_ARGB32;
            TileNumber n = getTileNumber(pixels, data.width, data.height, data.format);
            if(n.type != TileNumber::Type::INVALID)
            {
                isRecFail = false;
                Tile tile(TileNumberData::createTile(data.width, data.height, n));
                mTileCache.removeByArea(data.targetTextureId, data.texX, data.texY, data.width, data.height);
                mTileCache.set(data.targetTextureId, data.texX, data.texY, tile);
            }

            if(data.width != 1 && data.height != 1 && isRecFail)
            {
                QImage img(pixels, data.width, data.height, f);
                std::stringstream sstream;
                sstream << "recFail/" << failCount++ << "_" << data.texX << "x" << data.texY << "_" << data.width << "x" << data.height << ".png";
                img.save(QString::fromStdString(sstream.str()));

                std::cout << "Failed to recognize pixel data. A dump has been written to \"" << sstream.str() << "\"." << std::endl;
            }

            return;
        }

        // If we got here, we are either dealing with something using blend frames,
        // or a false positive. This means we will ONLY allow blend framed sprites.
        size_t width;
        size_t height;
        std::list<SpriteDraw::SpriteObjectPairing> pairings;
        for(size_t spriteId : ids)
        {
            if(spriteId >= Constants::SPRITE_ID_START && spriteId <= Constants::SPRITE_ID_END)
            {
                SpriteDraw::SpriteObjectPairing pairing;
                pairing.spriteId = spriteId;
                std::list<size_t> objects = mContext.getSpriteObjectBindings().getObjects(spriteId);
                if(!objects.empty())
                {
                    for(size_t object : objects)
                    {
                        for(const Object::SomeInfo& info : mContext.getObjects()[object].someInfos)
                            if(info.spriteInfo.numBlendFrames > 1)
                            {
                                pairing.objects.push_back(object);
                                break;
                            }
                    }

                    if(!pairing.objects.empty())
                    {
                        width = data.width;
                        height = data.height;
                        pairings.push_back(pairing);
                    }
                }
            }
        }

        if(!pairings.empty())
        {
            mTileCache.removeByArea(data.targetTextureId, data.texX, data.texY, data.width, data.height);
            Tile tile(SpriteObjectPairingsData::createTile(data.width, data.height, pairings));
            mTileCache.set(data.targetTextureId, data.texX, data.texY, tile);
        }
        else if(data.width != 1 && data.height != 1)
        {
            QImage::Format f = data.format == Format::RGBA ? QImage::Format_RGBA8888 : QImage::Format_ARGB32;
            QImage img(pixels, data.width, data.height, f);
            std::stringstream sstream;
            sstream << "recFail/" << failCount++ << "_" << data.texX << "x" << data.texY << "_" << data.width << "x" << data.height << ".png";
            img.save(QString::fromStdString(sstream.str()));
            std::cout << "Failed to recognize pixel data. A dump has been written to \"" << sstream.str() << "\"." << std::endl;
        }
    }
}
