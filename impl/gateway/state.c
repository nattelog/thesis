/**
 * State machine implementation do describe and execute a reactive system.
 */

#include "stddef.h"
#include "stdlib.h"
#include "assert.h"
#include "log.h"
#include "state.h"

/**
 * Initializes the lookup table with NULL pointers.
 */
void lookup_init(state_lookup_t* lookup)
{
    for (int i = 0; i < LOOKUP_SIZE; ++i) {
        lookup->table[i] = NULL;
    }
}

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

/**
 * Returns the state corresponding to state_name in lookup.
 */
state_t* lookup_search(state_lookup_t* lookup, const char* state_name)
{
    log_verbose("lookup_search::lookup=%p, state_name=\"%s\"", lookup, state_name);

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

/**
 * Returns 1 if lookup has a state with name state_name. 0 otherwise.
 */
int lookup_has(state_lookup_t* lookup, const char* state_name)
{
    log_verbose("lookup_has::lookup=%p, state=%p", lookup, state);

    return lookup_search(lookup, state_name) != NULL;
}

/**
 * Inserts state into lookup if it isn't already inserted.
 */
void lookup_insert(state_lookup_t* lookup, state_t* state)
{
    log_verbose("lookup_insert::lookup=%p, state=%p", lookup, state);

    if (state == NULL) {
        log_error("cannot insert state: state is NULL");
        return;
    }

    if (lookup_has(lookup, state->name)) {
        log_error("cannot insert state \"%s\": already inserted", state->name);
        return;
    }

    const unsigned long key = hash(state->name) % LOOKUP_SIZE;
    state_lookup_slot_t* new_slot = malloc(sizeof(state_lookup_slot_t));
    state_lookup_slot_t* next_slot = lookup->table[key];

    new_slot->state = state;
    new_slot->next = next_slot;
    lookup->table[key] = new_slot;
}

/**
 * Releases memory allocated by slot. If there is a chain of slots, recursively
 * releases the tail first.
 */
void lookup_clear_slot(state_lookup_slot_t* slot)
{
    log_verbose("lookup_clear_slot::slot=%p", slot);

    if (slot->next != NULL) {
        lookup_clear_slot(slot->next);
    }

    free(slot);
}

/**
 * Releases memory allocated by lookup.
 */
void lookup_clear(state_lookup_t* lookup)
{
    log_verbose("lookup_clear::lookup=%p", lookup);

    for (int i = 0; i < LOOKUP_SIZE; ++i) {
        state_lookup_slot_t* slot = lookup->table[i];

        if (slot != NULL) {
            lookup_clear_slot(slot);
        }
    }
}

void lookup_print_slot(state_lookup_slot_t* slot)
{
    printf("%s", slot->state->name);

    if (slot->next != NULL) {
        printf(", ");
        lookup_print_slot(slot->next);
    }
}

void lookup_print(state_lookup_t* lookup)
{
    for (int i = 0; i < LOOKUP_SIZE; ++i) {
        state_lookup_slot_t* slot = lookup->table[i];

        if (slot != NULL) {
            printf("[%d]\t", i);
            lookup_print_slot(slot);
            printf("\n");
        }
    }
}

/**
 * Allocates and adds a new edge between from_state and to_state.
 */
void state_add_edge(const char* name, state_t* from_state, state_t* to_state) {
    log_verbose("state_add_edge::name=\"%s\", from_state=%p, to_state=%p", name, from_state, to_state);

    edge_t* new_edge = malloc(sizeof(edge_t));

    new_edge->name = name;
    new_edge->next_state = to_state;
    new_edge->next_edge = from_state->edges;
    from_state->edges = new_edge;
}

/**
 * Allocates and returns a new state.
 */
state_t* state_create(const char* name, state_callback callback)
{
    log_verbose("state_create::name=\"%s\", callback=%p", name, callback);

    state_t* new_state = malloc(sizeof(state_t));

    new_state->name = name;
    new_state->edges = NULL;
    new_state->callback = callback;

    return new_state;
}

/**
 * Builds an entire state machine as described by si and ei. Any states inside
 * lookup can be added as well.
 */
state_t* state_machine_build(
        const state_initializer_t* si,
        const size_t nsi,
        const edge_initializer_t* ei,
        const size_t nei,
        state_lookup_t* lookup)
{
    log_verbose("state_machine_build::si=%p, nsi=%zu, ei=%p, nei=%zu, lookup=%p", si, nsi, ei, nei, lookup);

    if (nsi > 0) {
        for (int i = 0; i < nsi; ++i) {
            const char* name = si[i].name;
            state_callback callback = si[i].callback;
            state_t* state = state_create(name, callback);

            lookup_insert(lookup, state);
        }

        for (int i = 0; i < nei; ++i) {
            const char* name = ei[i].name;
            const char* from = ei[i].from;
            const char* to = ei[i].to;
            state_t* from_state = lookup_search(lookup, from);
            state_t* to_state = lookup_search(lookup, to);

            state_add_edge(name, from_state, to_state);
        }

        state_t* first_state = lookup_search(lookup, si[0].name);

        return first_state;
    }

    return NULL;
}

/**
 * Runs the callback associated with start_state with the given payload.
 */
void state_machine_run(state_t* start_state, void* payload) {
    log_verbose("state_machine_run::start_state=%p, payload=%p", start_state, payload);

    (*start_state->callback)(start_state, payload);
}

/**
 * Runs the callback associated with the next state of the given state, determined by edge_name.
 */
void state_run_next(state_t* state, const char* edge_name, void* payload) {
    log_verbose("state_run_next::state=%p, edge_name=\"%s\", payload=%p", state, edge_name, payload);

    edge_t* current_edge = state->edges;

    while (current_edge != NULL && current_edge->name != edge_name) {
        current_edge = current_edge->next_edge;
    }

    if (current_edge != NULL && current_edge->name == edge_name) {
        state_t* next_state = current_edge->next_state;
        (*next_state->callback)(next_state, payload);
    } else {
        log_error("state %s has no edge %s", state->name, edge_name);
    }
}

void state_print_tree(state_lookup_t* lookup, state_t* parent, int indent)
{
    edge_t* edge = parent->edges;

    while (edge != NULL) {
        state_t* next_state = edge->next_state;

        printf("  %*s%s -> %s\n", indent, "", edge->name, next_state->name);

        if (!lookup_has(lookup, next_state->name)) {
            lookup_insert(lookup, next_state);
            state_print_tree(lookup, next_state, indent + 2);
        }

        edge = edge->next_edge;
    }
}

void state_print(state_t* state)
{
    state_lookup_t lookup;
    lookup_init(&lookup);

    lookup_insert(&lookup, state);
    printf("%s\n", state->name);
    state_print_tree(&lookup, state, 0);
    lookup_clear(&lookup);
}
