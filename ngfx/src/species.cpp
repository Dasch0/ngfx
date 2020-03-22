#include "specie_h"
#include "env.h"
#include "asset.h"

namespace nenv
{
    namespace Swimmer
    {
        class Command
        {

        };

        class Signal
        {

        };

        class Skeleton
        {
        public:
            cpBody *_core;
            cpBody *_wheel;
            cpConstraint *_pivot;
            cpConstraint *_motor;

            status init(cpSpace *space, cpVect pos)
            {
                // Hardcoded parameters and locals
                // TODO: clean up hardcoded parameters
                cpVect coreVerts[] = {
                    cpv(-5,0),
                    cpv(5, 0),
                    cpv(0, 5),
                };
                cpFloat radius = 2.5;
                cpFloat mass = 5;
                uint64_t index;

                _core = cpSpaceAddBody(space, cpBodyNew(mass, cpMomentForPoly(mass, 4, coreVerts, cpvzero, 0.0)));
                index = Asset::Sprite::add(Asset::Texture::box);
                cpBodySetUserData(_core, (void *) index);
                cpBodySetVelocityUpdateFunc(_core, envVelocityFunc);
                cpBodySetPosition(_core, pos);
                cpShape * coreShape = cpSpaceAddShape(space, cpPolyShapeNew(_core, 3, coreVerts, cpTransformIdentity, 0.0));
                cpShapeSetFilter(coreShape, CP_SHAPE_FILTER_NONE);

                _wheel = cpSpaceAddBody(space, cpBodyNew(mass, cpMomentForPoly(mass, 4, coreVerts, cpvzero, 0.0)));
                index = Asset::Sprite::add(Asset::Texture::wheel);
                cpBodySetUserData(_wheel, (void *) index);
                cpBodySetVelocityUpdateFunc(_wheel, envVelocityFunc);
                cpBodySetPosition(_wheel, pos);
                cpShape * wheelShape = cpSpaceAddShape(space, cpCircleShapeNew(_wheel, 2.5, cpvzero));
                cpShapeSetFilter(wheelShape, CP_SHAPE_FILTER_NONE);

                _pivot = cpSpaceAddConstraint(space, cpPivotJointNew(_core, _wheel, pos));
                _motor = cpSpaceAddConstraint(space, cpSimpleMotorNew(_core, _wheel, 0.0));

                // TODO: improve status codes
                return 0;
            }

        };
    }

}
