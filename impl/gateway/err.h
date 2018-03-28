#ifndef __ERR_h__
#define __ERR_h__

#define ENULL -1
#define EARGC -2
#define EJSON -3
#define EPTCL -4
#define ENFND -5
#define EBNDS -6

#define GW_ERRNO_MAP(XX) \
    XX(ENULL, "null pointer") \
    XX(EARGC, "bad amount of arguments") \
    XX(EJSON, "cannot parse json") \
    XX(EPTCL, "bad json protocol") \
    XX(ENFND, "not found") \
    XX(EBNDS, "out of bounds") \

const char* gw_strerror(int err);

#endif
