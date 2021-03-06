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
#ifndef GRAPHICS_LAYER_RAW_IMAGE_HPP
#define GRAPHICS_LAYER_RAW_IMAGE_HPP


///////////////////////////////////
// Internal ShankBot headers
#include "utility/PixelFormat.hpp"
///////////////////////////////////

///////////////////////////////////
// STD C++
#include <vector>
#include <cstring>
///////////////////////////////////

namespace GraphicsLayer
{
    struct RawImage
    {
        public:
            RawImage(sb::utility::PixelFormat format, unsigned short width, unsigned short height, const unsigned char* data);
            RawImage(sb::utility::PixelFormat format, unsigned short width, unsigned short height, const std::vector<unsigned char>& data);

        private:
            size_t getSize() const;

        public:
            const sb::utility::PixelFormat format;
            const unsigned short width;
            const unsigned short height;
            const std::vector<unsigned char> pixels;
    };
}


#endif // GRAPHICS_LAYER_RAW_IMAGE_HPP
