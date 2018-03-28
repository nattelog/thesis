#include <stdlib.h>
#include <stdarg.h>
#include "test.h"

int main()
{
    int n_failed;
    SRunner* sr;

    sr = srunner_create(net_suite());
    srunner_add_suite(sr, protocol_suite());

    srunner_run_all(sr, CK_VERBOSE);
    n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (n_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
