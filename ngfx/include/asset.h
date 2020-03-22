#ifndef __ASSET_HEADER
#define __ASSET_HEADER


#include "SFML/Graphics.hpp"
#include "chipmunk/chipmunk.h"

#define MAX_ENTITIES 100

namespace Asset
{
    typedef uint8_t AssetID;
    namespace Texture
    {
        extern const uint count;
        extern const AssetID box;
        extern const AssetID wheel;
        extern const AssetID wing;
        extern const AssetID background;

        extern const char* files[];

        extern sf::Texture list[];

        void init(void);
    }

    namespace Sprite
    {
        extern uint count;
        extern sf::Sprite list[MAX_ENTITIES];

        uint add(AssetID texture);
        void draw(uint index, cpVect position, double angle, sf::RenderWindow *window);
        sf::Sprite * get(uint index);
    }
}

#endif //__ASSET_HEADER
