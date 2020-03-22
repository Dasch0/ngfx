//Simulation environment
#include "chipmunk/chipmunk.h"
#include "chipmunk/chipmunk_structs.h"
#include "chipmunk/cpBody.h"
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"
#include "zmq.h"
#include "env.h"
#include "asset.h"


// Gravity constant for this environment

static char buffer [500];
static cpVect nengoForce;

static Control_t ctrl;

static void
nengoPresolve(cpConstraint * motor, cpSpace * space)
{
    Control_t *c = (Control_t *) cpConstraintGetUserData(motor);

    // Calculate desired theta from force
    cpFloat theta = cpvtoangle(nengoForce) / 2.0f;

    // Calculate theta error
    cpFloat theta_err = theta - (cpvtoangle(cpvperp(cpBodyGetRotation(c->plant))));

    // Calculate rate & force to achieve theta (simple P controller for now)
    cpFloat rate = theta_err * 3;

    cpSimpleMotorSetRate(motor, ((int) c->in->x) ? c->in->x : rate);
    cpConstraintSetMaxForce(motor, 1000);
}

static void
ufoPresolve(cpConstraint * motor, cpSpace * space)
{
    Control_t *c = (Control_t *) cpConstraintGetUserData(motor);

    cpBodyApplyForceAtWorldPoint(c->plant, nengoForce * -10, cpBodyGetPosition(c->plant));
}


static void
motorPresolve(cpConstraint * motor, cpSpace *space)
{
    Control_t *c = (Control_t *) cpConstraintGetUserData(motor);

    // Compute positional term
    cpVect pos = cpBodyGetPosition(c->plant);
    cpVect pos_err = pos - cpv(500, 500);

    cpVect vel = cpBodyGetVelocity(c->plant);
    cpVect vel_err = cpv(vel.x, vel.y) * cpvlength(vel);

    // Calculate desired forces (simple PID controller for now)
    cpVect F = pos_err + vel_err * 1;

    // Calculate desired theta from force
    cpFloat theta = cpvtoangle(F) / 2;

    // Calculate theta error
    cpFloat theta_err = theta - (cpvtoangle(cpvperp(cpBodyGetRotation(c->plant))));

    // Calculate rate & force to achieve theta (simple P controller for now)
    cpFloat rate = theta_err * 3;

    cpSimpleMotorSetRate(motor, ((int) c->in->x) ? c->in->x : rate);
    cpConstraintSetMaxForce(motor, 1000);
}

//TODO: Break out into multiple files within the namespace
namespace nenv
{
    Table<cpBody, kMaxBodies> bodyTable;

    void
    envVelocityFunc(cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt)
    {
        cpVect f = cpvproject(cpv(kG, 0), cpvrperp(cpBodyGetRotation(body)));

        cpBodyApplyForceAtWorldPoint(body, f, cpBodyGetPosition(body));

        cpBodyUpdateVelocity(body, gravity, damping, dt);
    }

    struct Swimmer
    {//

        status createSkeleton(cpSpace *space, cpVect pos, Skeleton_t *s)
        {

        }

        static status bindSignals(Skeleton_t *s)
        {
            cpBodySetVelocityUpdateFunc(s->core, customVelocityFunc);
            cpConstraintSetPreSolveFunc(s->motor, customPresolveFunc);
            // TODO: improve status codes
            return 0;
        }

        static status bindCommands(Skeleton_t *s)
        {
            cpBodySetPositionUpdateFunc(s->core, customPositionFunc);
            // TODO: improve status codes
            return 0;
        }

        private:
        static void customVelocityFunc(cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt)
        {
            // TODO: replace this with actual handle dispenser
            env::handle h = (env::handle) ((uint64_t) body->userData);

            cpBodyUpdateVelocity(body, gravity, damping, dt);
        }
        static void customPresolveFunc(cpConstraint *motor, cpSpace *space)
        {
            // TODO: replace this with actual handle dispenser
            env::handle h = (env::handle) ((uint64_t) motor->userData);


        }
        static void customPositionFunc(cpBody *body, cpFloat dt)
        {
            // TODO: replace this with actual handle dispenser
            env::handle h = (env::handle) ((uint64_t) body->userData);
            cpBodyUpdatePosition(body, dt);


        }
        Swimmer();
        ~Swimmer();
    };

    void *alloc (size_t __nmemb, size_t __size)
    {
        // if caller is cpBodyAlloc, allocate to SOA
        if (cpBodyAlloc == (cpBody *(*)())__builtin_return_address(0))
            return (void *) (bodyTable.get(bodyTable.add()));

        // TODO: implement custom alloc for other callers
        else
            return calloc(__nmemb, __size);
    }
}

void envStep(cpSpace *space, void *responder, double dt, cpVect *goalPos, cpVect *goalVel)
{
    // Get input

    zmq_recv(responder, buffer, 100, 0);

    std::string vals = std::string(buffer);
    std::string::size_type sz;
    double x = std::stod(vals, &sz);
    double y = std::stod(vals.substr(sz));

    nengoForce = cpv(x,y);

    // simulate physics
    cpSpaceStep(space, dt);

    // Send Output
    cpVect pos = cpBodyGetPosition(ctrl.plant);
    cpVect vel = cpBodyGetVelocity(ctrl.plant);

    sprintf(buffer, "%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf", pos.x, pos.y, vel.x, vel.y, goalPos->x, goalPos->y, goalVel->x, goalVel->y);
    zmq_send(responder, buffer, strlen(buffer), 0);
}

cpBody * addSat(cpSpace *space, cpFloat size, cpFloat mass, cpVect pos, cpVect *input)
{
    cpVect verts[] = {
        cpv(-size,-size),
        cpv(-size, size),
        cpv( size, size),
        cpv( size,-size),
    };

    uint index;

    // Add rectangle, main body
    cpBody * rect = cpSpaceAddBody(space, cpBodyNew(mass, cpMomentForPoly(mass, 4, verts, cpvzero, cpFloat(0.0))));
    index = Asset::Sprite::add(Asset::Texture::box);
    cpBodySetUserData(rect, (void *) ((uint64_t) index));
    cpBodySetVelocityUpdateFunc(rect, planetGravityVelocityFunc);
    cpBodySetPosition(rect, pos);

    cpShape * rect_shape = cpSpaceAddShape(space, cpPolyShapeNew(rect, 4, verts, cpTransformIdentity, 0.0));

    // Add wheel
    cpBody * wheel = cpSpaceAddBody(space, cpBodyNew(mass, cpMomentForCircle(mass/2, size - 1, size - 2, cpvzero)));
    index = Asset::Sprite::add(Asset::Texture::wheel);
    cpBodySetUserData(wheel, (void *) ((uint64_t) index));

    cpBodySetPosition(wheel, pos);

    cpShape * wheel_shape = cpSpaceAddShape(space, cpCircleShapeNew(wheel, size - 1, cpvzero));
    cpShapeSetFilter(wheel_shape, CP_SHAPE_FILTER_NONE);

    // Connect wheel to rect and add motor
    cpSpaceAddConstraint(space, cpPivotJointNew(rect, wheel, pos));
    cpConstraint * motor = cpSpaceAddConstraint(space, cpSimpleMotorNew(rect, wheel, 0.0));

    // Connect I/O Channel to motor control
    ctrl.in = input;
    ctrl.plant = rect;
    cpConstraintSetUserData(motor, (cpDataPointer) &ctrl);

    // Set motor controller function
    cpConstraintSetPreSolveFunc(motor, nengoPresolve);

    cpBody *left_arm = cpSpaceAddBody(space, cpBodyNew(ARM_MASS, cpMomentForSegment(ARM_MASS, cpv(size/2, 0), cpv(-size/2, 0), ARM_RADIUS)));
    cpBodySetVelocityUpdateFunc(left_arm, planetGravityVelocityFunc);
    index = Asset::Sprite::add(Asset::Texture::wing);
    cpBodySetUserData(left_arm, (void *) ((uint64_t) index));
    cpVect left_pos= pos + cpv(size * 2, 0);
    cpBodySetPosition(left_arm, left_pos);
    cpShape * left_arm_shape = cpSpaceAddShape(space, cpSegmentShapeNew(left_arm, cpv(-size, 0),  cpv(size, 0), ARM_RADIUS));
    cpShapeSetFilter(left_arm_shape, CP_SHAPE_FILTER_NONE);

    cpBody *left_arm2 = cpSpaceAddBody(space, cpBodyNew(ARM_MASS, cpMomentForSegment(ARM_MASS, cpv(size/2, 0), cpv(-size/2, 0), ARM_RADIUS)));
    cpBodySetVelocityUpdateFunc(left_arm2, planetGravityVelocityFunc);
    index = Asset::Sprite::add(Asset::Texture::wing);
    cpBodySetUserData(left_arm2, (void *) ((uint64_t) index));
    cpVect left_pos2 = left_pos + cpv(size * 2, 0);
    cpBodySetPosition(left_arm2, left_pos2);
    cpShape * left_arm_shape2 = cpSpaceAddShape(space, cpSegmentShapeNew(left_arm2, cpv(-size, 0),  cpv(size, 0), ARM_RADIUS));
    cpShapeSetFilter(left_arm_shape2, CP_SHAPE_FILTER_NONE);

    cpBody *right_arm = cpSpaceAddBody(space, cpBodyNew(ARM_MASS, cpMomentForSegment(ARM_MASS, cpv(-size, 0), cpv(size, 0), ARM_RADIUS)));
    cpBodySetVelocityUpdateFunc(right_arm, planetGravityVelocityFunc);
    index = Asset::Sprite::add(Asset::Texture::wing);
    cpBodySetUserData(right_arm, (void *) ((uint64_t) index));
    cpVect right_pos = pos + cpv(-size * 2, 0);
    cpBodySetPosition(right_arm, right_pos);
    cpShape * right_arm_shape = cpSpaceAddShape(space, cpSegmentShapeNew(right_arm, cpv(-size, 0),  cpv(size, 0), ARM_RADIUS));
    cpShapeSetFilter(right_arm_shape, CP_SHAPE_FILTER_NONE);

    cpBody *right_arm2 = cpSpaceAddBody(space, cpBodyNew(ARM_MASS, cpMomentForSegment(ARM_MASS, cpv(-size, 0), cpv(size, 0), ARM_RADIUS)));
    cpBodySetVelocityUpdateFunc(right_arm2, planetGravityVelocityFunc);
    index = Asset::Sprite::add(Asset::Texture::wing);
    cpBodySetUserData(right_arm2, (void *) ((uint64_t) index));
    cpVect right_pos2 = right_pos + cpv(-size * 2, 0);
    cpBodySetPosition(right_arm2, right_pos2);
    cpShape * right_arm_shape2 = cpSpaceAddShape(space, cpSegmentShapeNew(right_arm2, cpv(-size, 0),  cpv(size, 0), ARM_RADIUS));
    cpShapeSetFilter(right_arm_shape2, CP_SHAPE_FILTER_NONE);

    cpSpaceAddConstraint(space, cpPivotJointNew2(rect, left_arm, cpv(-size, 0), cpv(size, 0)));
    cpSpaceAddConstraint(space, cpRotaryLimitJointNew(rect, left_arm, 0, 0));

    cpSpaceAddConstraint(space, cpPivotJointNew2(left_arm, left_arm2, cpv(-size, 0), cpv(size, 0)));
    cpSpaceAddConstraint(space, cpRotaryLimitJointNew(left_arm, left_arm2, 0, 0));

    cpSpaceAddConstraint(space, cpPivotJointNew2(rect, right_arm, cpv(size, 0), cpv(-size, 0)));
    cpSpaceAddConstraint(space, cpRotaryLimitJointNew(rect, right_arm, 0, 0));

    cpSpaceAddConstraint(space, cpPivotJointNew2(right_arm, right_arm2, cpv(size, 0), cpv(-size, 0)));
    cpSpaceAddConstraint(space, cpRotaryLimitJointNew(right_arm, right_arm2, 0, 0));

    return rect;
}

cpBody * addUfo(cpSpace *space, cpFloat size, cpFloat mass, cpVect pos, cpVect *input)
{
    cpVect verts[] = {
        cpv(-size,-size),
        cpv(-size, size),
        cpv( size, size),
        cpv( size,-size),
    };

    uint index;

    // Add rectangle, main body
    cpBody * rect = cpSpaceAddBody(space, cpBodyNew(mass, cpMomentForPoly(mass, 4, verts, cpvzero, cpFloat(0.0))));
    index = Asset::Sprite::add(Asset::Texture::box);
    cpBodySetUserData(rect, (void *) ((uint64_t) index));
    cpBodySetVelocityUpdateFunc(rect, planetGravityVelocityFunc);
    cpBodySetPosition(rect, pos);

    cpShape * rect_shape = cpSpaceAddShape(space, cpPolyShapeNew(rect, 4, verts, cpTransformIdentity, 0.0));

    // Add wheel
    cpBody * wheel = cpSpaceAddBody(space, cpBodyNew(mass, cpMomentForCircle(mass/2, size - 1, size - 2, cpvzero)));
    index = Asset::Sprite::add(Asset::Texture::wheel);
    cpBodySetUserData(wheel, (void *) ((uint64_t) index));

    cpBodySetPosition(wheel, pos);

    cpShape * wheel_shape = cpSpaceAddShape(space, cpCircleShapeNew(wheel, size - 1, cpvzero));
    cpShapeSetFilter(wheel_shape, CP_SHAPE_FILTER_NONE);

    // Connect wheel to rect and add motor
    cpSpaceAddConstraint(space, cpPivotJointNew(rect, wheel, pos));
    cpConstraint * motor = cpSpaceAddConstraint(space, cpSimpleMotorNew(rect, wheel, 0.0));

    // Connect I/O Channel to motor control
    ctrl.in = input;
    ctrl.plant = rect;
    cpConstraintSetUserData(motor, (cpDataPointer) &ctrl);

    // Set motor controller function
    cpConstraintSetPreSolveFunc(motor, ufoPresolve);

    return rect;
}
