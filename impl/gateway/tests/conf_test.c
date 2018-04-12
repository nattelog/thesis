#include "test.h"
#include "conf.h"
#include "log.h"

START_TEST(config_to_protocol_type_test)
{
    int r;
    config_data_t config;
    protocol_value_t* protocol;

    config.dispatcher = "serial";
    config.eventhandler = "serial";
    config.cpu = 0.5;
    config.io = 0.5;
    config.tp_size = 10;
    strcpy((char*) &config.logserver_address, "0.0.0.0");
    config.logserver_port = 5000;
    strcpy((char*) &config.nameservice_address, "0.0.0.0");
    config.nameservice_port = 5001;

    r = config_to_protocol_type(&config, &protocol);
    ck_assert_int_eq(r, 0);
    r = protocol_get_length(protocol);
    ck_assert_int_eq(r, 7);
    protocol_free_build(protocol);
}
END_TEST

Suite* conf_suite()
{
    Suite* s = suite_create("conf");
    TCase* tc = tcase_create("convert to protocol");

    tcase_add_test(tc, config_to_protocol_type_test);

    suite_add_tcase(s, tc);

    return s;
}
