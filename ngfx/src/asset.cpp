#include "asset.h"

namespace Asset
{
    namespace Texture
    {
        const uint count = 3;
        const AssetID box = 0;
        const AssetID wheel = 1;
        const AssetID wing = 2;

        sf::Texture list[count];
        const char* files[count] =
        {
            "assets/box.png",
            "assets/wheel.png",
            "assets/wing.png",
        };

        void init(void)
        {
            for (uint i = 0; i < count; i++)
            {
            list[i].loadFromFile(files[i]);
            list[i].setSmooth(true);
            }
        }
    }

    namespace Sprite
    {
        uint count = 0;
        sf::Sprite list[MAX_ENTITIES];

        // returns index in list
        uint add(AssetID texture)
        {
           sf::Vector2u size;
           sf::Texture *t = &Asset::Texture::list[texture];
           size = t->getSize();
           list[count].setTexture(*t);
           list[count].setOrigin(size.x * 0.5f, size.y * 0.5f);
           return count++;
        }

        void draw(uint index, cpVect position, float angle, sf::RenderWindow *window)
        {
            list[index].setPosition(position.x, position.y);
            list[index].setRotation(angle);
            window->draw(list[index]);
        }

        sf::Sprite * get(uint index)
        {
            return &list[index];
        }

    }
}
