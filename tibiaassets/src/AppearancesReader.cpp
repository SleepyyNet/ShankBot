// {SHANK_BOT_LICENSE_BEGIN}
/****************************************************************
****************************************************************
*
* ShankBot - Automation software for the MMORPG Tibia.
* Copyright (C) 2016 Mikael Hernvall
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
#include "tibiaassets/AppearancesReader.hpp"
#include "utility/utility.hpp"
using namespace sb::tibiaassets;
using namespace sb::utility;
///////////////////////////////////

///////////////////////////////////
// STD C++
#include <sstream>
#include <iostream>
#include <fstream>
///////////////////////////////////

AppearancesReader::AppearancesReader(std::string path)
{
    readAppearances(path);
}

const std::vector<Object>& AppearancesReader::getObjects() const
{
    return mObjects;
}

void AppearancesReader::readAppearances(std::string path)
{
    std::ifstream file(path, std::ios::binary);

    file.seekg(0, file.end);
    size_t fileSize = file.tellg();
    file.seekg(0);

    while(file.tellg() < fileSize)
        mObjects.push_back(readObject(file));

    file.close();
}


Object::FrameInfo AppearancesReader::readFrameInfo(std::istream& stream) const
{
    Object::FrameInfo info;
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    while((int)stream.tellg() - streamStart < size)
    {
        FrameInfoOpCode op;
        readStream(op, stream);
        switch(op)
        {
            case FrameInfoOpCode::ANIMATION_DELAY_MIN:
                info.animationDelayMin = readTibiaSizeIndicator(stream);
                break;

            case FrameInfoOpCode::ANIMATION_DELAY_MAX:
                info.animationDelayMax = readTibiaSizeIndicator(stream);
                break;

            default:
                std::stringstream sstream;
                sstream << "Expected FrameInfo op code, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
                THROW_RUNTIME_ERROR(sstream.str());
                break;
        }
    }

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading FrameInfo starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return info;
}

Object::AnimationInfo AppearancesReader::readAnimationInfo(std::istream& stream) const
{
    Object::AnimationInfo info;
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    while((int)stream.tellg() - streamStart < size)
    {
        AnimationInfoOpCode op;
        readStream(op, stream);
        switch(op)
        {
            case AnimationInfoOpCode::UNKNOWN1:
                readBoolean(stream);
                break;

            case AnimationInfoOpCode::UNKNOWN2:
                readBoolean(stream);

                break;

            case AnimationInfoOpCode::UNKNOWN3:
                readTibiaSizeIndicator(stream);
                break;

            case AnimationInfoOpCode::UNKNOWN4:
                readBoolean(stream);
                break;

            case AnimationInfoOpCode::UNKNOWN5:
                readTibiaSizeIndicator(stream);
                break;

            case AnimationInfoOpCode::FRAME_INFO:
                info.frameInfos.push_back(readFrameInfo(stream));
                break;

            default:
                std::stringstream sstream;
                sstream << "Expected AnimationInfo op code, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
                THROW_RUNTIME_ERROR(sstream.str());
                break;
        }
    }

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading AnimationInfo starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return info;
}


Object::SpriteInfo AppearancesReader::readSpriteInfo(std::istream& stream) const
{
    Object::SpriteInfo info;
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    while((int)stream.tellg() - streamStart < size)
    {
        SpriteInfoOpCode op;
        readStream(op, stream);
        switch(op)
        {
            case SpriteInfoOpCode::NUM_DIRECTIONS:
                readStream(info.numDirections, stream);
                break;

            case SpriteInfoOpCode::NUM_ADDONS:
                readStream(info.numAddons, stream);

                break;

            case SpriteInfoOpCode::NUM_MOUNTS:
                readStream(info.numMounts, stream);

                break;

            case SpriteInfoOpCode::NUM_BLEND_FRAMES:
                readStream(info.numBlendFrames, stream);

                break;

            case SpriteInfoOpCode::UNKNOWN1:
                readStream(info.unknown1, stream);
                break;

            case SpriteInfoOpCode::UNKNOWN2:
                readTibiaSizeIndicator(stream);
                break;

            case SpriteInfoOpCode::ANIMATION_INFO:
                info.animationInfo = readAnimationInfo(stream);
                break;

            case SpriteInfoOpCode::SPRITE_ID:
                info.spriteIds.push_back(readTibiaSizeIndicator(stream));
                break;

            default:
                std::stringstream sstream;
                sstream << "Expected SpriteInfo op code, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
                THROW_RUNTIME_ERROR(sstream.str());
                break;
        }
    }

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading SpriteInfo starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return info;
}


Object::SomeInfo AppearancesReader::readSomeInfo(std::istream& stream) const
{
    Object::SomeInfo info;
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    while((int)stream.tellg() - streamStart < size)
    {
        SomeInfoOpCode op;
        readStream(op, stream);
        switch(op)
        {
            case SomeInfoOpCode::UNKNOWN1:
                readStream(info.unknown1, stream);
                break;

            case SomeInfoOpCode::ANIMATION_TYPE:
                readStream(info.animationType, stream);
                break;

            case SomeInfoOpCode::SPRITE_INFO:
                info.spriteInfo = readSpriteInfo(stream);
                break;

             default:
                std::stringstream sstream;
                sstream << "Expected SomeInfo op code, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
                THROW_RUNTIME_ERROR(sstream.str());
                break;
        }
    }

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading SomeInfo starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return info;
}


bool AppearancesReader::readBoolean(std::istream& stream) const
{
    unsigned char byte;
    readStream(byte, stream);
    if(byte != 0x00 && byte != 0x01)
    {
        std::stringstream sstream;
        sstream << "Expected 0x00 or 0x01 value when reading boolean, got '" << (int)byte << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return byte == 0x01;
}

Object::BodyRestriction AppearancesReader::readBodyRestriction(std::istream& stream) const
{
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    unsigned char op;
    readStream(op, stream);
    if(op != 0x08)
    {
        std::stringstream sstream;
        sstream << "Expected 0x08 op code when reading BodyRestriction, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    Object::BodyRestriction b;
    readStream(b, stream);

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading BodyRestriction starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return b;
}





unsigned short AppearancesReader::readWalkSpeed(std::istream& stream) const
{
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    GroundInfoOpCode op;
    readStream(op, stream);
    if(op != GroundInfoOpCode::WALK_SPEED)
    {
        std::stringstream sstream;
        sstream << "Expected WALK_SPEED op code when reading GroundInfo, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    unsigned short walkSpeed = readTibiaSizeIndicator(stream);

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading GroundInfo starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return walkSpeed;
}

Object::MiniMapColor AppearancesReader::readMinimapColor(std::istream& stream) const
{
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    MinimapInfoOpCode op;
    readStream(op, stream);
    if(op != MinimapInfoOpCode::COLOR)
    {
        std::stringstream sstream;
        sstream << "Expected COLOR op code when reading MinimapInfo, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    unsigned short color = readTibiaSizeIndicator(stream);
    if(color > 0xff)
        THROW_RUNTIME_ERROR(stringify("Minimap color value out of range: ", color));

    Object::MiniMapColor miniMapColor = (Object::MiniMapColor)color;
    typedef Object::MiniMapColor C;
    switch(miniMapColor)
    {
            case C::BLACK:
            case C::DARK_GREEN:
            case C::GREEN:
            case C::SLIME_GREEN:
            case C::BLUE:
            case C::LIGHT_BLUE:
            case C::DARK_GRAY:
            case C::BROWN:
            case C::LIGHT_BROWN:
            case C::GRAY:
            case C::LIGHT_GREEN:
            case C::LIGHT_TURQUOISE:
            case C::RED:
            case C::ORANGE:
            case C::BEIGE:
            case C::YELLOW:
            case C::WHITE:
                break;

            default:
                THROW_RUNTIME_ERROR(stringify("Unexpected mini map color: ", (int)miniMapColor));
    }

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading MinimapInfo starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return miniMapColor;
}

void AppearancesReader::readOffset(unsigned char& x, unsigned char& y, std::istream& stream) const
{
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    OffsetInfoOpCode op;
    readStream(op, stream);
    if(op != OffsetInfoOpCode::OFFSET_X)
    {
        std::stringstream sstream;
        sstream << "Expected OFFSET_X op code when reading OffsetInfo, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }
    readStream(x, stream);

    readStream(op, stream);
    if(op != OffsetInfoOpCode::OFFSET_Y)
    {
        std::stringstream sstream;
        sstream << "Expected OFFSET_Y op code when reading OffsetInfo, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }
    readStream(y, stream);

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading OffsetInfo starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }
}


void AppearancesReader::readLightInfo(unsigned char& distance, unsigned short& color, std::istream& stream) const
{
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    LightInfoOpCode op;
    readStream(op, stream);
    if(op != LightInfoOpCode::DISTANCE)
    {
        std::stringstream sstream;
        sstream << "Expected DISTANCE op code when reading LightInfo, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }
    readStream(distance, stream);

    readStream(op, stream);
    if(op != LightInfoOpCode::COLOR)
    {
        std::stringstream sstream;
        sstream << "Expected COLOR op code when reading LightInfo, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }
    color = readTibiaSizeIndicator(stream);

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading LightInfo starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }
}


Object::VocationRestriction AppearancesReader::readVocationRestriction(std::istream& stream) const
{
    Object::VocationRestriction b;
    readStream(b, stream);

    if(b >= Object::VocationRestriction::ENUM_END)
    {
        std::stringstream sstream;
        sstream << "Expected VocationRestriction op code, got '" << (int)b << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return b;
}


Object::ClassRestriction AppearancesReader::readClassRestriction(std::istream& stream) const
{
    Object::ClassRestriction b;
    readStream(b, stream);

    if(b <= Object::ClassRestriction::ENUM_BEGIN || b >= Object::ClassRestriction::ENUM_END)
    {
        std::stringstream sstream;
        sstream << "Expected ClassRestriction op code, got '" << (int)b << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return b;
}

std::string AppearancesReader::readString(std::istream& stream) const
{
    size_t size = readTibiaSizeIndicator(stream);

    std::string str;
    str.resize(size);
    for(size_t i = 0; i < size; i++)
        readStream(str[i], stream);

    return str;
}

Object::MarketInfo AppearancesReader::readMarketInfo(std::istream& stream) const
{
    Object::MarketInfo info;
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    while((int)stream.tellg() - streamStart < size)
    {
        MarketInfoOpCode op;
        readStream(op, stream);
        switch(op)
        {
            case MarketInfoOpCode::MARKET_ID_1:
                info.id1 = readTibiaSizeIndicator(stream);
                break;

            case MarketInfoOpCode::MARKET_ID_2:
                info.id2 = readTibiaSizeIndicator(stream);
                break;

            case MarketInfoOpCode::VOCATION_RESTRICTION:
                info.vocationRestriction = readVocationRestriction(stream);
                break;

            case MarketInfoOpCode::CLASS_RESTRICTION:
                info.classRestriction = readClassRestriction(stream);
                break;

            case MarketInfoOpCode::LEVEL_RESTRICTION:
                info.levelRestriction = readTibiaSizeIndicator(stream);
                break;

            case MarketInfoOpCode::NAME:
                info.name = readString(stream);
                if(info.name == "backpack")
                    int flerp = 0;
                break;

             default:
                std::stringstream sstream;
                sstream << "Expected MarketInfo op code, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
                THROW_RUNTIME_ERROR(sstream.str());
                break;
        }
    }

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading MarketInfo starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return info;
}

unsigned char AppearancesReader::readHeight(std::istream& stream) const
{
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    HeightInfoOpCode op;
    readStream(op, stream);
    if(op != HeightInfoOpCode::HEIGHT)
    {
        std::stringstream sstream;
        sstream << "Expected HEIGHT op code when reading HeightInfo, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    unsigned char height;
    readStream(height, stream);

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading HeightInfo starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return height;
}

void AppearancesReader::readItemUnknown15(std::istream& stream) const
{
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    unsigned char op;
    readStream(op, stream);
    if(op != 0x08)
    {
        std::stringstream sstream;
        sstream << "Expected 0x08 op code when reading ItemUnknown15, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    size_t unknown = readTibiaSizeIndicator(stream);

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading ItemUnknown15 starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }
}

unsigned short AppearancesReader::readMaxCharacters(std::istream& stream) const
{
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    unsigned char op;
    readStream(op, stream);
    if(op != 0x08)
    {
        std::stringstream sstream;
        sstream << "Expected 0x08 op code when reading MaxCharacters, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    unsigned short maxCharacters = readTibiaSizeIndicator(stream);

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading MaxCharacters starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return maxCharacters;
}

unsigned char AppearancesReader::readDefaultAction(std::istream& stream) const
{
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    unsigned char op;
    readStream(op, stream);
    if(op != 0x08)
    {
        std::stringstream sstream;
        sstream << "Expected 0x08 op code when reading DefaultAction, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    unsigned char defaultAction;
    readStream(defaultAction, stream);

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading DefaultAction starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return defaultAction;
}


Object::ItemInfo AppearancesReader::readItemInfo(std::istream& stream) const
{
    Object::ItemInfo info;
    size_t size = readTibiaSizeIndicator(stream);
    int streamStart = stream.tellg();

    while((int)stream.tellg() - streamStart < size)
    {
        ItemInfoOpCode op = (ItemInfoOpCode)readTibiaSizeIndicator(stream);
        switch(op)
        {
            case ItemInfoOpCode::UNKNOWN1: // 0x38 0x01
                info.unknown1 = readBoolean(stream);
                break;

            case ItemInfoOpCode::USABLE:
                info.isUsable = readBoolean(stream);
                break;

            case ItemInfoOpCode::BODY_RESTRICTION:
                info.bodyRestriction = readBodyRestriction(stream);
                break;

            case ItemInfoOpCode::IMMOVABLE:
                info.isMovable = !readBoolean(stream);
                break;

            case ItemInfoOpCode::STACKABLE:
                info.isStackable = readBoolean(stream);
                break;

            case ItemInfoOpCode::BLOCKING:
                info.isBlocking = readBoolean(stream);
                break;

            case ItemInfoOpCode::BLOCKING_PATH: // 0x80 0x01
                info.isPathBlocking = readBoolean(stream);
                break;

            case ItemInfoOpCode::BLOCKING_RANGED_ATTACK: // 0x78 0x01
                info.isBlockingRangedAttack = readBoolean(stream);
                break;

            case ItemInfoOpCode::PICKUPABLE: // 0x90 0x01 0x01
                info.isPickupable = readBoolean(stream);
                break;

            case ItemInfoOpCode::CONTAINER: // 0x28 0x01
                info.isContainer = readBoolean(stream);
                break;

            case ItemInfoOpCode::UNKNOWN6: // 0x40 0x01
                readBoolean(stream);
                break;

            case ItemInfoOpCode::TOP_ORDER_2: // 0x18 0x01
                readBoolean(stream);
                info.topOrder = 2;
                break;

            case ItemInfoOpCode::UNKNOWN9: // 0xe0 0x01 0x01 // UNKNOWN in old client as well (0x1b)
                readBoolean(stream);
                break;

            case ItemInfoOpCode::TOP_ORDER_3: // 0x20 0x01
                readBoolean(stream);
                info.topOrder = 3;
                break;

            case ItemInfoOpCode::TOP_ORDER_1: // 0x10 0x01
                readBoolean(stream);
                info.topOrder = 1;
                break;

            case ItemInfoOpCode::FLOOR_CHANGE: // 0xc8 0x01 0x01
                info.isFloorChange = readBoolean(stream);
                break;

            case ItemInfoOpCode::DEFAULT_ACTION: // 0x9a 0x02 0x02
                info.defaultAction = readDefaultAction(stream);
                break;

            case ItemInfoOpCode::TRANSPARENT_TILE: // 0x88 0x02 0x01
                info.isTransparent = readBoolean(stream);
                break;

            case ItemInfoOpCode::UNKNOWN15: // 0xfa 0x01
                readItemUnknown15(stream);
                break;

            case ItemInfoOpCode::FLUID: // 0x98 0x01 0x01
                info.isFluid = readBoolean(stream);
                break;

            case ItemInfoOpCode::HANGABLE: // 0xa0 0x01 0x01
                info.isHangable = readBoolean(stream);
                break;

            case ItemInfoOpCode::WALL_DIRECTION: // 0xaa 0x01 0x02
                info.isWall = true;
                stream.seekg(readTibiaSizeIndicator(stream), stream.cur);
                break;

            case ItemInfoOpCode::ROTATABLE: // 0xb0 0x01 0x01
                info.isRotatable = readBoolean(stream);
                break;

            case ItemInfoOpCode::UNKNOWN20: // 0xa8 0x02 0x01
                readBoolean(stream);
                break;

            case ItemInfoOpCode::FULL_TILE: // 0x80 0x02 0x01
                info.isItemDestroyer = readBoolean(stream);
                break;

            case ItemInfoOpCode::UNKNOWN22: // 0x88 0x01 0x01
                readBoolean(stream);
                break;

            case ItemInfoOpCode::WRITABLE:
                info.isReadable = true;
                info.isWritable = true;
                info.maxCharacters = readMaxCharacters(stream);
                break;

            case ItemInfoOpCode::READABLE:
                info.isReadable = true;
                info.maxCharacters = readMaxCharacters(stream);
                break;

            case ItemInfoOpCode::SPLASH:
                info.isSplash = readBoolean(stream);
                break;

            case ItemInfoOpCode::UNKNOWN26:
                readBoolean(stream);
                break;

            case ItemInfoOpCode::UNKNOWN27:
                readBoolean(stream);
                break;

            case ItemInfoOpCode::UNKNOWN28:
                readBoolean(stream);
                break;

            case ItemInfoOpCode::GROUND_INFO:
                info.walkSpeed = readWalkSpeed(stream);
                info.isGround = true;
                break;

            case ItemInfoOpCode::MINIMAP_INFO:
                info.minimapColor = readMinimapColor(stream);
                info.hasMinimapColor = true;
                break;

            case ItemInfoOpCode::OFFSET_INFO:
                readOffset(info.offsetX, info.offsetY, stream);
                break;

            case ItemInfoOpCode::LIGHT_INFO:
                readLightInfo(info.lightBrightness, info.lightColor, stream);
                break;

            case ItemInfoOpCode::MARKET_INFO:
                info.hasMarketInfo = true;
                info.marketInfo = readMarketInfo(stream);
                break;

            case ItemInfoOpCode::HEIGHT_INFO:
                info.height = readHeight(stream);
                break;

             default:
                std::stringstream sstream;
                sstream << "Expected ItemInfo op code, got '" << (int)op << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
                THROW_RUNTIME_ERROR(sstream.str());
                break;
        }
    }

    if((int)stream.tellg() - streamStart != size)
    {
        std::stringstream sstream;
        sstream << "Size error when reading ItemInfo starting at position " << streamStart << "." << std::endl
                << "Expected " << size << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return info;
}

Object AppearancesReader::readObject(std::istream& stream) const
{
    Object o;
    readStream(o.type, stream);

    typedef Object::Type Type;
    switch(o.type)
    {
        case Type::ITEM:
        case Type::OUTFIT:
        case Type::PROJECTILE:
        case Type::EFFECT:
            break;

         default:
            std::stringstream sstream;
            sstream << "Expected object type op code, got '" << (int)o.type << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
            THROW_RUNTIME_ERROR(sstream.str());
            break;
    }

    size_t objectSize = readTibiaSizeIndicator(stream);

    int streamStart = stream.tellg();

    ObjectOpCode op;
    readStream(op, stream);

    if(op != ObjectOpCode::ID)
    {
        std::stringstream sstream;
        sstream << "Expected Object ID op code, got " << (int)op << ", at position " << stream.tellg() << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    o.id = readTibiaSizeIndicator(stream);
    while((int)stream.tellg() - streamStart < objectSize)
    {
        readStream(op, stream);
        switch(op)
        {
            case ObjectOpCode::SOME_INFO:
                o.someInfos.push_back(readSomeInfo(stream));
                break;

            case ObjectOpCode::ITEM_INFO:
                o.itemInfo = readItemInfo(stream);
                break;

            default:
                std::stringstream sstream;
                sstream << "Expected an Object op code, got '" << (int)o.type << "', at position " << (int)stream.tellg() - 1 << "." << std::endl;
                THROW_RUNTIME_ERROR(sstream.str());
                break;
        }
    }

    if((int)stream.tellg() - streamStart != objectSize)
    {
        std::stringstream sstream;
        sstream << "Size error when reading Object starting at position " << streamStart << "." << std::endl
                << "Expected " << objectSize << " bytes, but read " << (int)stream.tellg() - streamStart << "." << std::endl;
        THROW_RUNTIME_ERROR(sstream.str());
    }

    return o;
}

