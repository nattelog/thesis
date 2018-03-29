#include "log.h"
#include "test.h"
#include "err.h"
#include "protocol.h"

START_TEST(protocol_parse_success_test)
{
    protocol_value_t* value;
    char* buf = "{}";
    int r = protocol_parse(&value, buf);

    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);
}
END_TEST

START_TEST(protocol_parse_fail_test)
{
    protocol_value_t* value;
    char* buf = "{";
    int r = protocol_parse(&value, buf);

    ck_assert_int_eq(r, EJSON);
    protocol_free_parse(value);
}
END_TEST

START_TEST(protocol_has_key_test)
{
    int r;
    protocol_value_t* value;
    char* buf = "{\"foo\":\"bar\"}";

    r = protocol_parse(&value, buf);
    ck_assert_int_eq(r, 0);

    r = protocol_has_key(value, "foo");
    ck_assert_int_eq(r, 1);

    r = protocol_has_key(value, "bar");
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);
}
END_TEST

START_TEST(protocol_type_test)
{
    int r;
    protocol_value_t* value;
    char* strobj = "{\"foo\":\"bar\"}";
    char* strstr = "\"foo\"";
    char* strarr = "[]";
    char* strbol = "true";
    char* strint = "10";

    // is object
    r = protocol_parse(&value, strobj);
    ck_assert_int_eq(r, 0);
    r = protocol_is_object(value);
    ck_assert_int_eq(r, 1);
    protocol_free_parse(value);

    r = protocol_parse(&value, strstr);
    ck_assert_int_eq(r, 0);
    r = protocol_is_object(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strarr);
    ck_assert_int_eq(r, 0);
    r = protocol_is_object(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strbol);
    ck_assert_int_eq(r, 0);
    r = protocol_is_object(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strint);
    ck_assert_int_eq(r, 0);
    r = protocol_is_object(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    // is array
    r = protocol_parse(&value, strobj);
    ck_assert_int_eq(r, 0);
    r = protocol_is_array(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strstr);
    ck_assert_int_eq(r, 0);
    r = protocol_is_array(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strarr);
    ck_assert_int_eq(r, 0);
    r = protocol_is_array(value);
    ck_assert_int_eq(r, 1);
    protocol_free_parse(value);

    r = protocol_parse(&value, strbol);
    ck_assert_int_eq(r, 0);
    r = protocol_is_array(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strint);
    ck_assert_int_eq(r, 0);
    r = protocol_is_array(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    // is string
    r = protocol_parse(&value, strobj);
    ck_assert_int_eq(r, 0);
    r = protocol_is_string(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strstr);
    ck_assert_int_eq(r, 0);
    r = protocol_is_string(value);
    ck_assert_int_eq(r, 1);
    protocol_free_parse(value);

    r = protocol_parse(&value, strarr);
    ck_assert_int_eq(r, 0);
    r = protocol_is_string(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strbol);
    ck_assert_int_eq(r, 0);
    r = protocol_is_string(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strint);
    ck_assert_int_eq(r, 0);
    r = protocol_is_string(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    // is bool
    r = protocol_parse(&value, strobj);
    ck_assert_int_eq(r, 0);
    r = protocol_is_bool(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strstr);
    ck_assert_int_eq(r, 0);
    r = protocol_is_bool(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strarr);
    ck_assert_int_eq(r, 0);
    r = protocol_is_bool(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strbol);
    ck_assert_int_eq(r, 0);
    r = protocol_is_bool(value);
    ck_assert_int_eq(r, 1);
    protocol_free_parse(value);

    r = protocol_parse(&value, strint);
    ck_assert_int_eq(r, 0);
    r = protocol_is_bool(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    // is int
    r = protocol_parse(&value, strobj);
    ck_assert_int_eq(r, 0);
    r = protocol_is_int(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strstr);
    ck_assert_int_eq(r, 0);
    r = protocol_is_int(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strarr);
    ck_assert_int_eq(r, 0);
    r = protocol_is_int(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strbol);
    ck_assert_int_eq(r, 0);
    r = protocol_is_int(value);
    ck_assert_int_eq(r, 0);
    protocol_free_parse(value);

    r = protocol_parse(&value, strint);
    ck_assert_int_eq(r, 0);
    r = protocol_is_int(value);
    ck_assert_int_eq(r, 1);
    protocol_free_parse(value);
}
END_TEST

START_TEST(protocol_get_length_test)
{
    int r;
    protocol_value_t* value;
    char* strobj = "{\"a\":0, \"b\":0, \"c\":0}";
    char* strarr = "[0, 0, 0]";
    char* strstr = "\"foo\"";
    char* strbol = "true";

    r = protocol_parse(&value, strobj);
    ck_assert_int_eq(r, 0);
    r = protocol_get_length(value);
    ck_assert_int_eq(r, 3);
    protocol_free_parse(value);

    r = protocol_parse(&value, strarr);
    ck_assert_int_eq(r, 0);
    r = protocol_get_length(value);
    ck_assert_int_eq(r, 3);
    protocol_free_parse(value);

    r = protocol_parse(&value, strstr);
    ck_assert_int_eq(r, 0);
    r = protocol_get_length(value);
    ck_assert_int_eq(r, 3);
    protocol_free_parse(value);

    r = protocol_parse(&value, strbol);
    ck_assert_int_eq(r, 0);
    r = protocol_get_length(value);
    ck_assert_int_eq(r, EPTCL);
    protocol_free_parse(value);
}
END_TEST

START_TEST(protocol_get_at_test)
{
    int r;
    protocol_value_t* value;
    protocol_value_t* dest;
    char* strarr = "[\"a\", \"b\", \"c\"]";

    r = protocol_parse(&value, strarr);
    ck_assert_int_eq(r, 0);
    r = protocol_get_at(value, &dest, 0);
    ck_assert_int_eq(r, 0);
    r = protocol_is_string(dest);
    ck_assert_int_eq(r, 1);

    r = protocol_get_at(value, &dest, 3);
    ck_assert_int_eq(r, EBNDS);
    protocol_free_parse(value);
}
END_TEST

START_TEST(protocol_get_string_test)
{
    int r;
    protocol_value_t* value;
    protocol_value_t key_store;
    protocol_value_t* key = &key_store;
    char buf[3];
    char* strobj = "{\"foo\":\"bar\"}";

    r = protocol_parse(&value, strobj);
    ck_assert_int_eq(r, 0);
    r = protocol_get_key(value, &key, "foo");
    ck_assert_int_eq(r, 0);
    r = protocol_get_string(key, (char*) &buf);
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(buf, "bar");
    protocol_free_parse(value);

    // should be available after free
    ck_assert_str_eq(buf, "bar");
}
END_TEST

START_TEST(protocol_get_bool_test)
{
    int r;
    protocol_value_t* value;
    protocol_value_t key_store;
    protocol_value_t* key = &key_store;
    char* strobj = "{\"foo\":true}";

    r = protocol_parse(&value, strobj);
    ck_assert_int_eq(r, 0);
    r = protocol_get_key(value, &key, "foo");
    ck_assert_int_eq(r, 0);
    r = protocol_get_bool(key);
    ck_assert_int_eq(r, 1);
    protocol_free_parse(value);
}
END_TEST

START_TEST(protocol_get_int_test)
{
    int r;
    protocol_value_t* value;
    protocol_value_t key_store;
    protocol_value_t* key = &key_store;
    char* strobj = "{\"foo\":10}";

    r = protocol_parse(&value, strobj);
    ck_assert_int_eq(r, 0);
    r = protocol_get_key(value, &key, "foo");
    ck_assert_int_eq(r, 0);
    r = protocol_get_int(key);
    ck_assert_int_eq(r, 10);
    protocol_free_parse(value);
}
END_TEST

START_TEST(protocol_build_string_test)
{
    int r;
    protocol_value_t* value;
    char* str = "foo";
    char buf[3];

    r = protocol_build_string(&value, str);
    ck_assert_int_eq(r, 0);
    r = protocol_get_string(value, (char*) &buf);
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(buf, "foo");
    protocol_free_build(value);
}
END_TEST

START_TEST(protocol_build_int_test)
{
    int r;
    protocol_value_t* value;
    int i = 10;

    r = protocol_build_int(&value, i);
    ck_assert_int_eq(r, 0);
    r = protocol_get_int(value);
    ck_assert_int_eq(r, i);
    protocol_free_build(value);
}
END_TEST

START_TEST(protocol_build_array_test)
{
    int r;
    protocol_value_t* array;
    protocol_value_t* str;
    protocol_value_t* i;

    r = protocol_build_string(&str, "foo");
    ck_assert_int_eq(r, 0);
    r = protocol_build_int(&i, 10);
    ck_assert_int_eq(r, 0);
    r = protocol_build_array(&array, 2, str, i);
    ck_assert_int_eq(r, 0);
    r = protocol_get_length(array);
    ck_assert_int_eq(r, 2);

    protocol_free_build(array);
}
END_TEST

START_TEST(protocol_build_request_test)
{
    int r;
    protocol_value_t* req;
    protocol_value_t* str;
    protocol_value_t* i;
    protocol_value_t* args;

    r = protocol_build_string(&str, "foo");
    ck_assert_int_eq(r, 0);
    r = protocol_build_int(&i, 10);
    ck_assert_int_eq(r, 0);
    r = protocol_build_request(&req, "foo", 2, str, i);
    ck_assert_int_eq(r, 0);
    r = protocol_get_length(req);
    ck_assert_int_eq(r, 2);
    r = protocol_has_key(req, "method");
    ck_assert_int_eq(r, 1);
    r = protocol_has_key(req, "args");
    ck_assert_int_eq(r, 1);
    r = protocol_get_key(req, &args, "args");
    ck_assert_int_eq(r, 0);
    r = protocol_get_length(args);
    ck_assert_int_eq(r, 2);

    protocol_free_build(req);
}
END_TEST

START_TEST(protocol_build_response_success_test)
{
    int r;
    protocol_value_t* res;
    protocol_value_t* str;
    protocol_value_t* result;

    r = protocol_build_string(&str, "foo");
    ck_assert_int_eq(r, 0);
    r = protocol_build_response_success(&res, str);
    ck_assert_int_eq(r, 0);
    r = protocol_get_length(res);
    ck_assert_int_eq(r, 1);
    r = protocol_has_key(res, "result");
    ck_assert_int_eq(r, 1);
    r = protocol_get_key(res, &result, "result");
    ck_assert_int_eq(r, 0);
    r = protocol_is_string(result);
    ck_assert_int_eq(r, 1);

    protocol_free_build(res);
}
END_TEST

START_TEST(protocol_build_response_error_test)
{
    int r;
    protocol_value_t* res;
    protocol_value_t* err;
    char* name = "Error";
    char* msg = "An error occurred";

    r = protocol_build_response_error(&res, name, msg);
    ck_assert_int_eq(r, 0);
    r = protocol_get_length(res);
    ck_assert_int_eq(r, 1);
    r = protocol_has_key(res, "error");
    ck_assert_int_eq(r, 1);
    r = protocol_get_key(res, &err, "error");
    ck_assert_int_eq(r, 0);
    r = protocol_get_length(err);
    ck_assert_int_eq(r, 2);

    protocol_free_build(res);
}
END_TEST

START_TEST(protocol_get_response_error_test)
{
    int r;
    protocol_value_t* err;
    char* error_name = "Error";
    char* error_msg = "An error occurred";
    char name_result[28];
    char msg_result[28];

    r = protocol_build_response_error(&err, error_name, error_msg);
    ck_assert_int_eq(r, 0);
    r = protocol_get_response_error(err, (char*) &name_result, (char*) &msg_result);
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(name_result, error_name);
    ck_assert_str_eq(msg_result, error_msg);
}
END_TEST

START_TEST(protocol_to_json_test)
{
    int r;
    protocol_value_t* value;
    char* strobj = "{\"foo\":\"bar\"}";
    char buf[128];

    r = protocol_parse(&value, strobj);
    ck_assert_int_eq(r, 0);
    r = protocol_to_json(value, (char*) &buf);
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(buf, strobj);
}
END_TEST

Suite* protocol_suite()
{
    Suite* s = suite_create("protocol");
    TCase* tc = tcase_create("parse");

    tcase_add_test(tc, protocol_parse_success_test);
    tcase_add_test(tc, protocol_parse_fail_test);
    tcase_add_test(tc, protocol_has_key_test);
    tcase_add_test(tc, protocol_type_test);
    tcase_add_test(tc, protocol_get_length_test);
    tcase_add_test(tc, protocol_get_at_test);
    tcase_add_test(tc, protocol_get_string_test);
    tcase_add_test(tc, protocol_get_bool_test);
    tcase_add_test(tc, protocol_get_int_test);
    tcase_add_test(tc, protocol_build_string_test);
    tcase_add_test(tc, protocol_build_int_test);
    tcase_add_test(tc, protocol_build_array_test);
    tcase_add_test(tc, protocol_build_request_test);
    tcase_add_test(tc, protocol_build_response_success_test);
    tcase_add_test(tc, protocol_build_response_error_test);
    tcase_add_test(tc, protocol_get_response_error_test);
    tcase_add_test(tc, protocol_to_json_test);

    suite_add_tcase(s, tc);

    return s;
}
