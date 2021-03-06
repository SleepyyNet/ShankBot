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
#include "monitor/ShankBot.hpp"
#include "monitor/VersionControl.hpp"
#include "monitor/SequenceTree.hpp"
#include "monitor/SpriteObjectBindings.hpp"
#include "utility/file.hpp"
#include "tibiaassets/CatalogContent.hpp"
#include "tibiaassets/SpriteReader.hpp"
#include "tibiaassets/GraphicsResourceReader.hpp"
#include "tibiaassets/AppearancesReader.hpp"
#include "monitor/Constants.hpp"


#include "utility/utility.hpp"
#include "monitor/OutfitAddonMerger.hpp"
#include "monitor/CombatSquareSample.hpp"
#include "monitor/GlyphsFile.hpp"
using namespace GraphicsLayer;
using namespace sb::tibiaassets;
///////////////////////////////////

///////////////////////////////////
// STD C++
#include <iostream>
#include <memory>
#include <cassert>
///////////////////////////////////

///////////////////////////////////
// STD C
#include <unistd.h>
///////////////////////////////////

void ShankBot::initializeData(std::string clientDir, std::string versionControlDir)
{
    using namespace sb::utility;
    const std::string STORAGE_PATH = VersionControl::getPath(versionControlDir);
    const std::string SPRITE_COLOR_TREE_PATH = STORAGE_PATH + "/tree-sprite-color.bin";
    const std::string SPRITE_TRANSPARENCY_TREE_PATH = STORAGE_PATH + "/tree-sprite-transparency.bin";
    const std::string SPRITE_OBJECT_BINDINGS_PATH = STORAGE_PATH + "/sprite-object-bindings.bin";
    const std::string GLYPHS_PATH = STORAGE_PATH + "/glyphs.bin";
    const std::string SPRITE_INFO_PATH = STORAGE_PATH + "/sprite-info.bin";
    const std::string CATALOG_CONTENT_PATH = clientDir + "/packages/Tibia/assets/catalog-content.json";
    const std::string GRAPHICS_RESOURCES_PATH = clientDir + "/packages/Tibia/bin/graphics_resources.rcc";

    std::cout << "Initializing..." << std::endl;

    std::cout << "Reading catalog content... ";
    CatalogContent cat(CATALOG_CONTENT_PATH);
    std::cout << "Done" << std::endl;

    const std::list<CatalogContent::Appearances>& appearanceses = cat.getAppearances();
    assert(appearanceses.size() == 1);

    std::cout << "Reading appearances... ";
    AppearancesReader appearances(appearanceses.front().path);
    auto objects = std::make_unique<std::vector<Object>>(appearances.getObjects());
    std::cout << "Done" << std::endl;

    std::cout << "Reading graphics resource names...";
    auto graphicsResourceNames = std::make_unique<std::vector<std::string>>(GraphicsResourceReader::readNames(GRAPHICS_RESOURCES_PATH));
    std::cout << "Done" << std::endl;

    if(VersionControl::hasNewVersion(clientDir, versionControlDir))
    {
        std::cout << "Has new version" << std::endl;



        std::vector<std::vector<size_t>> colorSprites;
        std::vector<std::vector<size_t>> transparencySprites;
        std::vector<size_t> ids;

        std::cout << "Generating combat squares...";
        {
            std::vector<CombatSquareSample::CombatSquare> squares = CombatSquareSample::generateSamples();
            if(squares.size() >= Constants::COMBAT_SQUARE_ID_END - Constants::COMBAT_SQUARE_ID_START)
                THROW_RUNTIME_ERROR("There are not enough IDs allocated for combat squares.")
            for(size_t i = 0; i < squares.size(); i++)
            {
                const CombatSquareSample::CombatSquare& s = squares[i];
                colorSprites.push_back(rgbaToColorTreeSprite(s.pixels, s.width, s.height));
                transparencySprites.push_back(rgbaToTransparencyTreeSprite(s.pixels, s.width, s.height));
                ids.push_back(i + Constants::COMBAT_SQUARE_ID_START);
            }
        }
        std::cout << "Done" << std::endl;

        std::cout << "Loading graphics resources... ";
        std::unique_ptr<GraphicsResourceReader> gResourceReader = std::make_unique<GraphicsResourceReader>(GRAPHICS_RESOURCES_PATH);
        const std::vector<GraphicsResourceReader::GraphicsResource>& gResources = gResourceReader->getGraphicsResources();
        for(size_t i = 0; i < gResources.size(); i++)
        {
            const GraphicsResourceReader::GraphicsResource& gRes = gResources[i];
            const size_t ID = i + Constants::GRAPHICS_RESOURCE_ID_START;

            std::vector<size_t> colorTreeSprite = rgbaToColorTreeSprite(gRes.pixels, gRes.width, gRes.height);
            if(colorTreeSprite.size() > 0)
            {
                bool isDuplicate = false;
                for(const std::vector<size_t>& spr1 : colorSprites)
                {
                    if(spr1.size() == colorTreeSprite.size())
                    {
                        size_t j = 0;
                        while(j < spr1.size())
                        {
                            if(spr1[j] != colorTreeSprite[j])
                                break;

                            j++;
                        }

                        if(j == spr1.size())
                        {
                            isDuplicate = true;
                            break;
                        }
                    }
                }

                if(!isDuplicate)
                {
                    colorSprites.push_back(colorTreeSprite);
                    transparencySprites.push_back(rgbaToTransparencyTreeSprite(gRes.pixels, gRes.width, gRes.height));
                    ids.push_back(ID);
                }
            }
        }
        if(gResources.size() >= Constants::GRAPHICS_RESOURCE_ID_END - Constants::GRAPHICS_RESOURCE_ID_START)
            THROW_RUNTIME_ERROR("There are not enough IDs allocated for Tibia's graphics resources.");
        gResourceReader.reset();
        std::cout << "Done" << std::endl;

        std::cout << "Creating sprite object bindings... ";
        SpriteObjectBindings bindings(*objects);
        bindings.writeToBinaryFile(SPRITE_OBJECT_BINDINGS_PATH);
        std::cout << "Done" << std::endl;

        std::cout << "Loading sprites... ";
        OutfitAddonMerger merger;
        for(const Object& o : *objects)
            if(o.type == Object::Type::OUTFIT)
                merger.addOutfit(o);
        std::vector<std::vector<size_t>> mergeTransparencySprites;
        std::vector<unsigned int> mergeIds;
        SpriteReader(cat).forEachSprite([&](const SpriteReader::Sprite& spr)
        {
            size_t pixelWidth = spr.tileWidth * Constants::TILE_PIXEL_WIDTH;
            size_t pixelHeight = spr.tileHeight * Constants::TILE_PIXEL_HEIGHT;

            if(spr.id < Constants::SPRITE_ID_START || spr.id > Constants::SPRITE_ID_END)
                THROW_RUNTIME_ERROR("There are not enough IDs allocated for Tibia's sprites.");

            std::vector<std::shared_ptr<Sprite>> merges = merger.processSprite(spr.id, pixelWidth, pixelHeight, spr.pixels);
            for(const std::shared_ptr<Sprite>& m : merges)
            {
                bool isBlank = false;
                std::vector<size_t> transparency = rgbaToTransparencyTreeSprite(m->pixels, m->width, m->height, &isBlank);
                if(!isBlank)
                {
                    mergeTransparencySprites.push_back(transparency);
                    mergeIds.push_back(m->id);
                }
            }

            std::list<size_t> boundObjects = bindings.getObjects(spr.id);
            bool isMergeOnly = false;
            for(size_t objIndex : boundObjects)
            {
                const Object& o = (*objects)[objIndex];
                const Object::SpriteInfo& info = o.someInfos.front().spriteInfo;
                if(o.type == Object::Type::OUTFIT && (info.numAddons > 1 || info.numMounts > 1 || info.numBlendFrames > 1))
                    isMergeOnly = true;
                else
                {
                    isMergeOnly = false;
                    break;
                }
            }
            if(!isMergeOnly)
            {
                bool isBlank = false;
                std::vector<size_t> colorSprite = rgbaToColorTreeSprite(spr.pixels, pixelWidth, pixelHeight, &isBlank);
                if(!isBlank)
                {
                    colorSprites.push_back(colorSprite);
                    transparencySprites.push_back(rgbaToTransparencyTreeSprite(spr.pixels, pixelWidth, pixelHeight));
                    ids.push_back(spr.id);
                }
            }


            return true;
        });
        assert(merger.isEmpty());
        std::cout << "Done" << std::endl;


        std::cout << "Building colorTree... ";
        SequenceTree* colorTree = new SequenceTree(colorSprites, ids);
        colorTree->writeToBinaryFile(SPRITE_COLOR_TREE_PATH);
        delete colorTree;
        colorSprites.resize(0);
        std::cout << "Done " << std::endl;

        std::cout << "Building transparencyTree... ";
        transparencySprites.insert(transparencySprites.end(), mergeTransparencySprites.begin(), mergeTransparencySprites.end());
        ids.insert(ids.end(), mergeIds.begin(), mergeIds.end());
        mergeTransparencySprites.resize(0);
        mergeIds.resize(0);
        SequenceTree* transparencyTree = new SequenceTree(transparencySprites, ids);
        transparencyTree->writeToBinaryFile(SPRITE_TRANSPARENCY_TREE_PATH);
        delete transparencyTree;
        transparencySprites.resize(0);
        std::cout << "Done" << std::endl;

        std::cout << "Generating glyph samples... ";
        auto fontSample = std::make_unique<FontSample>("Verdana", 6.f, 10.f, FontSample::Style::BOLD | FontSample::Style::NORMAL);
        auto glyphs = std::make_unique<std::vector<FontSample::Glyph>>(fontSample->getGlyphs());
        fontSample = std::make_unique<FontSample>("Verdana", 13.f, 15.f, FontSample::Style::BOLD | FontSample::Style::NORMAL);
        glyphs->insert(glyphs->end(), fontSample->getGlyphs().begin(), fontSample->getGlyphs().end());
        GlyphsFile::write(*glyphs, GLYPHS_PATH);
        fontSample.reset();
        glyphs.reset();
        std::cout << "Done" << std::endl;

        VersionControl::checkout(versionControlDir);
    }

    std::cout << "Loading sprite object bindings... ";
    auto bindings = std::make_unique<SpriteObjectBindings>(SPRITE_OBJECT_BINDINGS_PATH);
    std::cout << "Done" << std::endl;

    std::cout << "Loading sprite infos... ";
    auto spriteInfo = std::make_unique<SpriteInfo>(cat.getSpriteSheets());
    std::cout << "Done" << std::endl;

    std::cout << "Loading trees... ";
    auto spriteColorTree = std::make_unique<SequenceTree>(SPRITE_COLOR_TREE_PATH);
    auto spriteTransparencyTree = std::make_unique<SequenceTree>(SPRITE_TRANSPARENCY_TREE_PATH);
    std::cout << "Done" << std::endl;

    std::cout << "Loading glyph samples... ";
    auto glyphs = std::make_unique<std::vector<FontSample::Glyph>>();
    GlyphsFile::read(*glyphs, GLYPHS_PATH);
    std::cout << "Done" << std::endl;

    mTibiaContext = std::make_unique<TibiaContext>
    (
        objects,
        bindings,
        spriteColorTree,
        spriteTransparencyTree,
        spriteInfo,
        graphicsResourceNames,
        glyphs
    );
}

ShankBot::ShankBot(std::string clientDir, std::string versionControlDir)
{
    srand(NULL);
    initializeData(clientDir, versionControlDir);

    std::cout << "Starting client" << std::endl;
    mTibiaClient = std::unique_ptr<TibiaClient>(new TibiaClient(clientDir, *mTibiaContext));
}

void ShankBot::run()
{
    while(mTibiaClient->isAlive())
    {
        mTibiaClient->update();
    }
}
