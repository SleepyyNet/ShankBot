///////////////////////////////////
// Internal ShankBot headers
#include "ShankBot.hpp"
#include "VersionControl.hpp"
#include "TibiaSpr.hpp"
#include "ImageTree.hpp"
#include "SpriteObjectBindings.hpp"
#include "fileUtility.hpp"
#include "TibiaDat.hpp"
#include "utility.hpp"
using namespace GraphicsLayer;
///////////////////////////////////

///////////////////////////////////
// STD C++
#include <iostream>
///////////////////////////////////

///////////////////////////////////
// STD C
#include <unistd.h>
///////////////////////////////////

void ShankBot::initializeData(std::string clientDir, std::string versionControlDir)
{
    std::string storagePath = VersionControl::getPath(versionControlDir);
    std::string spriteColorTreePath = storagePath + "/tree-sprite-color";
    std::string spriteTransparencyTreePath = storagePath + "/tree-sprite-transparency";
    std::string spriteObjectBindingsPath = storagePath + "/sprite-object-bindings";
    std::string datPath = clientDir + "/Tibia.dat";
    std::string sprPath = clientDir + "/Tibia.spr";
    std::string picPath = clientDir + "/Tibia.pic";

    if(VersionControl::hasNewVersion(clientDir, versionControlDir))
    {
        std::cout << "Has new version" << std::endl;

        TibiaSpr* spr = new TibiaSpr(sprPath);
        std::vector<std::vector<unsigned char>> sprites;
        for(auto pixels : spr->getSprites())
            sprites.push_back(rgbaToColorTreeSprite(pixels));

        ImageTree* spriteColor = new ImageTree(sprites, spr->getSpriteIds());
        spriteColor->writeToBinaryFile(spriteColorTreePath);
        delete spriteColor;

        sprites.clear();
        for(auto pixels : spr->getSprites())
            sprites.push_back(rgbaToTransparencyTreeSprite(pixels));

        ImageTree* spriteTransparency = new ImageTree(sprites, spr->getSpriteIds());
        spriteTransparency->writeToBinaryFile(spriteTransparencyTreePath);
        delete spriteTransparency;
        delete spr;

        TibiaDat dat(datPath);
        SpriteObjectBindings bindings(dat);
        bindings.writeToBinaryFile(spriteObjectBindingsPath);

        copyFile(sprPath, storagePath);
        copyFile(datPath, storagePath);
        copyFile(picPath, storagePath);
    }

    auto dat = std::make_unique<TibiaDat>(datPath);
    auto bindings = std::make_unique<SpriteObjectBindings>(*dat, spriteObjectBindingsPath);


    std::cout << "Constructing trees..." << std::endl;
    auto spriteColorTree = std::make_unique<ImageTree>(spriteColorTreePath);
    auto spriteTransparencyTree = std::make_unique<ImageTree>(spriteTransparencyTreePath);

    mTibiaContext = std::make_unique<TibiaContext>(dat, bindings, spriteColorTree, spriteTransparencyTree);
}

ShankBot::ShankBot(std::string clientDir, std::string versionControlDir)
{
    initializeData(clientDir, versionControlDir);

    std::cout << "Starting client" << std::endl;
    mTibiaClient = std::unique_ptr<TibiaClient>(new TibiaClient(clientDir, *mTibiaContext));
}

void ShankBot::run()
{
    while(true)
    {
        mTibiaClient->update();
        size_t ms = 40;
        usleep(ms * 1000);
    }
}
