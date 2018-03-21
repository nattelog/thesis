#ifndef __STATE_h__
#define __STATE_h__

#include <stdlib.h>

#define LOOKUP_SIZE 20

typedef struct state_s state_t;
typedef struct edge_s edge_t;
typedef struct state_initializer_s state_initializer_t;
typedef struct edge_initializer_s edge_initializer_t;
typedef struct state_lookup_slot_s state_lookup_slot_t;
typedef struct state_lookup_s state_lookup_t;

typedef void (*state_callback)(state_t*, void*);

struct state_s {
    const char* name;
    edge_t* edges;
    state_callback callback;
};

struct edge_s {
    const char* name;
    state_t* next_state;
    edge_t* next_edge;
};

struct state_initializer_s {
    const char* name;
    state_callback callback;
};

struct edge_initializer_s {
    const char* name;
    const char* from;
    const char* to;
};

struct state_lookup_slot_s {
    state_t* state;
    state_lookup_slot_t* next;
};

struct state_lookup_s {
    state_lookup_slot_t* table[LOOKUP_SIZE];
};

void lookup_init(state_lookup_t*);

void lookup_insert_machine(state_lookup_t*, state_t*);

void lookup_clear(state_lookup_t*);

void lookup_print(state_lookup_t*);

void state_add_edge(const char*, state_t*, state_t*);

void state_copy(state_t*, state_t*);

state_t* state_machine_build(
        const state_initializer_t*,
        const size_t nsi,
        const edge_initializer_t*,
        const size_t nei,
        state_lookup_t*);

void state_machine_run(state_t*, void*);

void state_run_next(state_t*, const char*, void*);

void state_print(state_t*);

#endif
