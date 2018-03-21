#include <stdlib.h>
#include "log.h"
#include "uv.h"

void log_check_uv_r(int r, char* msg)
{
    if (r != 0) {
        log_error("%s: [%s(%d): %s]\n", msg, uv_err_name((r)), (int) r, uv_strerror((r)));
        exit(1);
    }
}
