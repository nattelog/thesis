#include "test.h"
#include "log.h"
#include "state.h"

void __start(state_t* state, void* payload)
{
    state_run_next(state, "e1", payload);
    state_run_next(state, "e2", payload);
    state_run_next(state, "e3", payload);
}

void __end(state_t* state, void* payload)
{
    ck_assert_int_eq(*((int*) payload), 1);
}

START_TEST(state_machine_build_test)
{
    state_t* state;
    state_lookup_t lookup;

    const state_initializer_t si[] = {
        { .name = "start", .callback = __start },
        { .name = "end", .callback = __end }
    };
    const edge_initializer_t ei[] = {
        { .name = "e1", .from = "start", .to = "end" },
        { .name = "e2", .from = "start", .to = "end" },
        { .name = "e3", .from = "start", .to = "end" }
    };
    const int nsi = sizeof(si) / sizeof(si[0]);
    const int nei = sizeof(ei) / sizeof(ei[0]);

    lookup_init(&lookup);
    state = state_machine_build(si, nsi, ei, nei, &lookup);
    lookup_clear(&lookup);

    int i = 1;
    state_machine_run(state, &i);
}
END_TEST

void __start_cycle(state_t* state, void* payload)
{
    int* i = (int*) payload;

    if ((*i)++ == 10) {
        return;
    }

    state_run_next(state, "next", i);
}

void __end_cycle(state_t* state, void* payload)
{
    state_run_next(state, "next", payload);
}

START_TEST(state_machine_cycle_test)
{
    state_t* state;
    state_lookup_t lookup;

    const state_initializer_t si[] = {
        { .name = "start", .callback = __start_cycle },
        { .name = "end", .callback = __end_cycle }
    };
    const edge_initializer_t ei[] = {
        { .name = "next", .from = "start", .to = "end" },
        { .name = "next", .from = "end", .to = "start" },
    };
    const int nsi = sizeof(si) / sizeof(si[0]);
    const int nei = sizeof(ei) / sizeof(ei[0]);

    lookup_init(&lookup);
    state = state_machine_build(si, nsi, ei, nei, &lookup);
    lookup_clear(&lookup);

    int i = 0;
    state_machine_run(state, &i);
    ck_assert_int_eq(i, 11);
}
END_TEST

Suite* state_suite()
{
    Suite* s = suite_create("state");
    TCase* tc = tcase_create("machine build");

    tcase_add_test(tc, state_machine_build_test);
    tcase_add_test(tc, state_machine_cycle_test);

    suite_add_tcase(s, tc);

    return s;
}
