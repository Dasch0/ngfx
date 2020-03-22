#ifndef __SPECIES_HEADER
#define __SPECIES_HEADER

#include <cstdint>
#include <assert.h>
#include "chipmunk/chipmunk.h"
#include "zmq.h"
#include "env.h"

namespace nenv
{
    template <class Command, class Signal, class Skeleton, int maxCount>
    class SpeciesTable
    {
    public:
        // TODO: Improve access control of members
        uint64_t _count;

        // Networking members
        void *_zmqContext;
        void *_sockets[maxCount];
        zmq_msg_t _inMsgs[maxCount];
        zmq_msg_t _outMsgs[maxCount];


        // Environment members
        cpSpace *_space;
        Command _commandTable[maxCount];
        Signal _signalTable[maxCount];
        Skeleton _skeletonTable[maxCount];


        SpeciesTable(void * zmqContext, cpSpace * space) :
            _count(0), _zmqContext(zmqContext), _space(space) {}

        handle add(void)
        {
            // TODO: Improve error handling
            assert(unlikely(_count>=maxCount));

            // Set up networking
            _sockets[_count] = zmq_socket(_zmqContext, ZMQ_REP);
            int ret = 0;
            ret |= zmq_bind(_sockets[_count], "tcp://*:5555");
            ret |= zmq_msg_init_data(_inMsgs[_count], _commandTable[_count], sizeof(_commandTable[_count]), NULL, NULL);
            ret |= zmq_msg_init_data(_outMsgs[_count], _signalTable[_count], sizeof(_signalTable[_count]), NULL, NULL);

            assert(unlikely(ret < 0));

            // Create physics components of org
            // TODO: Add enforced standard for init functions
            _skeletonTable[_count].init(_space, cpvzero);
            _commandTable[_count].init();
            _signalTable[_count].init();

            _count++;
            return _count;
        }

        status recv(void)
        {
            status ret = 0;
            for(int i = 0; i < _count; i++)
            {
                ret |= zmq_msg_recv(_inMsgs[i], _sockets[i], 0);
            }
            return ret;
        }

        status send(void)
        {
            status ret = 0;
            for(int i = 0; i < _count; i++)
            {
                ret |= zmq_msg_send(_outMsgs[i], _sockets[i], 0);
            }
            return ret;
        }

        ~SpeciesTable(void) {}
    };

    namespace Swimmer
    {

    }
}

#endif //__SPECIES_HEADER
