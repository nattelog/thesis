#ifndef __ERR_h__
#define __ERR_h__

#define ENULL 1
#define EARGC 2
#define EJSON 3
#define EPTCL 4

#define GW_ERRNO_MAP(XX) \
    XX(ENULL, "null pointer") \
    XX(EARGC, "bad amount of arguments") \
    XX(EJSON, "cannot parse json") \
    XX(EPTCL, "bad json protocol") \

const char* gw_strerror(int err);

#endif
