/**
 * State machine implementation do describe and execute a reactive system.
 */

#include "stddef.h"
#include "stdlib.h"
#include "assert.h"
#include "log.h"
#include "state.h"

/**
 * djb2 hash from http://www.cse.yorku.ca/~oz/hash.html
 */
unsigned long hash(const char* str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

void lookup_insert(state_lookup_t* lookup, state_t* state)
{
    const unsigned long key = hash(state->name) % LOOKUP_SIZE;
    state_lookup_slot_t* new_slot = malloc(sizeof(state_lookup_slot_t));
    state_lookup_slot_t* next_slot = lookup->table[key];

    new_slot->state = state;
    new_slot->next = next_slot;
    lookup->table[key] = new_slot;
}

state_t* lookup_search(state_lookup_t* lookup, const char* state_name)
{
    const unsigned long key = hash(state_name) % LOOKUP_SIZE;
    state_lookup_slot_t* slot = lookup->table[key];

    while (slot != NULL && slot->state->name != state_name) {
        slot = slot->next;
    }

    if (slot == NULL) {
        return NULL;
    }

    return slot->state;
}

int lookup_has(state_lookup_t* lookup, state_t* state)
{
    return lookup_search(lookup, state->name) != NULL;
}

void lookup_clear_slot(state_lookup_slot_t* slot)
{
    if (slot->next != NULL) {
        lookup_clear_slot(slot->next);
    }

    free(slot);
}

void lookup_clear(state_lookup_t* lookup)
{
    for (int i = 0; i < LOOKUP_SIZE; ++i) {
        state_lookup_slot_t* slot = lookup->table[i];

        if (slot != NULL) {
            lookup_clear_slot(slot);
        }
    }
}

state_t* state_create(const char* name, state_callback callback)
{
    state_t* new_state = malloc(sizeof(state_t));

    new_state->name = name;
    new_state->edges = NULL;
    new_state->callback = callback;

    return new_state;
}

void state_add_edge(const char* name, state_t* from_state, state_t* to_state) {
    edge_t* new_edge = malloc(sizeof(edge_t));

    new_edge->name = name;
    new_edge->next_state = to_state;
    new_edge->next_edge = from_state->edges;
    from_state->edges = new_edge;
}

state_t* state_machine_build(
        const state_initializer_t* si,
        const size_t nsi,
        const edge_initializer_t* ei,
        const size_t nei)
{
    if (nsi > 0) {
        state_lookup_t lookup = { .table = { NULL } };

        for (int i = 0; i < nsi; ++i) {
            const char* name = si[i].name;
            state_callback callback = si[i].callback;
            state_t* state = state_create(name, callback);

            lookup_insert(&lookup, state);
        }

        for (int i = 0; i < nei; ++i) {
            const char* name = ei[i].name;
            const char* from = ei[i].from;
            const char* to = ei[i].to;
            state_t* from_state = lookup_search(&lookup, from);
            state_t* to_state = lookup_search(&lookup, to);

            state_add_edge(name, from_state, to_state);
        }

        state_t* first_state = lookup_search(&lookup, si[0].name);
        lookup_clear(&lookup);

        return first_state;
    }

    return NULL;
}

void state_machine_run(state_t* start_state, void* payload) {
    (*start_state->callback)(start_state, payload);
}

state_t* state_next(state_t* state, const char* edge_name, void* payload) {
    edge_t* current_edge = state->edges;

    while (current_edge != NULL && current_edge->name != edge_name) {
        current_edge = current_edge->next_edge;
    }

    if (current_edge != NULL && current_edge->name == edge_name) {
        state_t* next_state = current_edge->next_state;
        (*next_state->callback)(next_state, payload);
        return current_edge->next_state;
    }

    log_error("state %s has no edge %s", state->name, edge_name);
}

void state_print_tree(state_lookup_t* lookup, state_t* parent, int indent)
{
    edge_t* edge = parent->edges;

    while (edge != NULL) {
        state_t* next_state = edge->next_state;

        printf("  %*s%s -> %s\n", indent, "", edge->name, next_state->name);

        if (!lookup_has(lookup, next_state)) {
            lookup_insert(lookup, next_state);
            state_print_tree(lookup, next_state, indent + 2);
        }

        edge = edge->next_edge;
    }
}

void state_print(state_t* state)
{
    state_lookup_t lookup = { .table = { NULL } };

    lookup_insert(&lookup, state);
    printf("%s\n", state->name);
    state_print_tree(&lookup, state, 0);
    lookup_clear(&lookup);
}