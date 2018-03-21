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

/**
 * The state callback is run when the state is entered.
 */
typedef void (*state_callback)(state_t* state, void* payload);

struct state_s {
    const char* name;
    edge_t* edges;
    state_callback callback;
};

/**
 * The edge object represents an edge between two states. As a state can
 * consist of multiple edges, an edge has a link to the next edge in the chain.
 */
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

/**
 * The lookup object implements a hash table to store a set of states.
 */
struct state_lookup_s {
    state_lookup_slot_t* table[LOOKUP_SIZE];
};

void lookup_init(state_lookup_t* lookup);

void lookup_clear(state_lookup_t* lookup);

void lookup_print(state_lookup_t* lookup);

void state_add_edge(const char* name, state_t* from_state, state_t* to_state);

state_t* state_machine_build(
        const state_initializer_t* si,
        const size_t nsi,
        const edge_initializer_t* ei,
        const size_t nei,
        state_lookup_t* lookup);

void state_machine_run(state_t* start_state, void* payload);

void state_run_next(state_t* state, const char* edge_name, void* payload);

void state_print(state_t* state);

#endif
