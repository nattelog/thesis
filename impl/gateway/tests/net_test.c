#include "test.h"
#include "net.h"
#include "err.h"

START_TEST(request_init)
{
    int r = 0;
    net_request_t request;

    r = net_request_init(&request, "foo", 1, "bar");
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(request.method, "foo");
    ck_assert_int_eq(request.argc, 1);
    ck_assert_str_eq(request.argv[0], "bar");
}
END_TEST

START_TEST(request_init_no_args)
{
    int r = 0;
    net_request_t request;

    r = net_request_init(&request, "foo", 0);
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(request.method, "foo");
    ck_assert_int_eq(request.argc, 0);
}
END_TEST

START_TEST(parse_request)
{
    int r = 0;
    net_request_t request;
    char* buf = "{\"method\":\"foo\",\"args\":[\"bar\"]}";

    r = net_parse_request(&request, buf);
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(request.method, "foo");
    ck_assert_int_eq(request.argc, 0);
}
END_TEST

START_TEST(request_to_json)
{
    int r = 0;
    net_request_t request;
    char buf[256];

    r = net_request_init(&request, "foo", 1, "bar");
    ck_assert_int_eq(r, 0);

    r = net_request_to_json(&request, (char*) &buf);
    ck_assert_int_eq(r, 0);

    r = net_parse_request(&request, (char*) &buf);
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(request.method, "foo");
    ck_assert_int_eq(request.argc, 0);
}
END_TEST

START_TEST(response_success_init)
{
    int r = 0;
    net_response_t response;

    r = net_response_success_init(&response, "foo");
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(response.result, "foo");
    ck_assert_ptr_null(response.error_name);
    ck_assert_ptr_null(response.error_message);
}
END_TEST

START_TEST(response_error_init)
{
    int r = 0;
    net_response_t response;

    r = net_response_error_init(&response, "Error", "An error occurred");
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(response.error_name, "Error");
    ck_assert_str_eq(response.error_message, "An error occurred");
    ck_assert_ptr_null(response.result);
}
END_TEST

START_TEST(parse_successful_response)
{
    int r = 0;
    net_response_t response;
    char* buf = "{\"result\":\"foo\"}";

    r = net_parse_response(&response, buf);
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(response.result, "foo");
    ck_assert_ptr_null(response.error_name);
    ck_assert_ptr_null(response.error_message);
}
END_TEST

START_TEST(parse_error_response)
{
    int r = 0;
    net_response_t response;
    char* buf = "{\"error\":{\"name\":\"Error\", \"args\":[\"An error occurred\"]}}";

    r = net_parse_response(&response, buf);
    ck_assert_int_eq(r, 0);
    ck_assert_ptr_null(response.result);
    ck_assert_str_eq(response.error_name, "Error");
    ck_assert_str_eq(response.error_message, "An error occurred");
}
END_TEST

START_TEST(parse_corrupt_response)
{
    int r = 0;
    net_response_t response;
    char* buf = "{\"rror\":{\"name\":\"Error\", \"args\":[\"An error occurred\"]}";

    r = net_parse_response(&response, buf);
    ck_assert_int_eq(r, EJSON);
}
END_TEST

Suite* net_suite()
{
    Suite* s = suite_create("net");
    TCase* tc = tcase_create("json_parse");

    tcase_add_test(tc, request_init);
    tcase_add_test(tc, request_init_no_args);
    tcase_add_test(tc, parse_request);
    tcase_add_test(tc, request_to_json);
    tcase_add_test(tc, response_success_init);
    tcase_add_test(tc, response_error_init);
    tcase_add_test(tc, parse_successful_response);
    tcase_add_test(tc, parse_error_response);
    tcase_add_test(tc, parse_corrupt_response);

    suite_add_tcase(s, tc);

    return s;
}
