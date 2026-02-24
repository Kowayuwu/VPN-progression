#define ARENA_IMPLEMENTATION
#include "arena.h"

static Arena default_arena = {0};
static Arena temporary_arena = {0};
static Arena *context_arena = &default_arena;

void *context_alloc(size_t size)
{
    assert(context_arena);
    return arena_alloc(context_arena, size);
}

int main(void)
{
    // Allocate stuff in default_arena
    context_alloc(64);
    context_alloc(128);
    context_alloc(256);
    context_alloc(512);

    // Allocate stuff in temporary_arena;
    context_arena = &temporary_arena;
    context_alloc(64);
    context_alloc(128);
    context_alloc(256);
    context_alloc(512);

    // Deallocate everything at once
    arena_free(&default_arena);
    arena_free(&temporary_arena);
    return 0;
}