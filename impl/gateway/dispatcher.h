#ifndef __DISPATCHER_h__
#define __DISPATCHER_h__

#include "net.h"
#include "protocol.h"

void dispatcher_serial(protocol_value_t* devices);

void dispatcher_cooperative(protocol_value_t* devices);

#endif
