#include "err.h"

#define GW_STRERROR_GEN(name, msg) case name: return msg;
const char* gw_strerror(int err)
{
    switch (err) {
        GW_ERRNO_MAP(GW_STRERROR_GEN)
    }

    return "unknown error";
}
#undef GW_STRERROR_GEN
