#ifndef __DISPATCHER_h__
#define __DISPATCHER_h__

#include <pthread.h>
#include "net.h"
#include "protocol.h"
#include "conf.h"

void dispatcher_serial(config_data_t* config, protocol_value_t* devices);

void dispatcher_cooperative(config_data_t* config, protocol_value_t* devices);

#endif
