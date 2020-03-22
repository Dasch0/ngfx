#ifndef __ENV_HEADER
#define __ENV_HEADER

#include "chipmunk/chipmunk.h"
#include "chipmunk/cpBody.h"
#include "SFML/Graphics.hpp"
#include "util.h"

// Assign custom allocator for chipmunk2d
#undef cpcalloc
#define cpcalloc env::alloc

typedef struct
{
    cpVect *in;
    cpBody *plant;
} Control_t;

void envStep(cpSpace *, void *, double, cpVect*, cpVect*);

cpBody * addSat(cpSpace *space, cpFloat size, cpFloat mass, cpVect pos, cpVect *input);

cpBody * addUfo(cpSpace *space, cpFloat size, cpFloat mass, cpVect pos, cpVect *input);

namespace nenv
{
    // Environment constants
    const uint16_t kMaxBodies = 1000;
    const cpFloat kG = 1;

    // Environement types
    typedef int32_t handle;
    typedef int16_t status;

    void envVelocityFunc(cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt);

    void *alloc (size_t __nmemb, size_t __size);

}

#endif //__ENV_HEADER
