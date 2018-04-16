#include "test.h"
#include "log.h"
#include "event_handler.h"

START_TEST(event_handler_fill_io_buffer_test)
{
    char* buf;

    event_handler_fill_io_buffer(0.01, &buf);
    ck_assert_int_eq(strlen(buf), 2684353);
    free(buf);
}
END_TEST

Suite* event_handler_suite()
{
    Suite*s = suite_create("event_handler");
    TCase* tc = tcase_create("fill io buffer");

    tcase_add_test(tc, event_handler_fill_io_buffer_test);

    suite_add_tcase(s, tc);

    return s;
}
