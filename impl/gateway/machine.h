#ifndef __MACHINE_h__
#define __MACHINE_h__

#include "state.h"

state_t* machine_tcp_request(state_lookup_t* lookup, state_callback done);

#endif
